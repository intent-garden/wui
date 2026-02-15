//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/slider.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/common/flag_helpers.hpp>

#include <cmath>

namespace wui
{

slider::slider(int32_t from_, int32_t to_, int32_t value_, std::function<void(int32_t)> change_callback_, slider_orientation orientation_, std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : orientation(orientation_),
    from(from_), to(to_), value(value_),
    centered_mode(from_ < 0 && to_ > 0), // Автоматически определяем centered режим
    change_callback(change_callback_),
    drag_end_callback(),
    tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_control_sid(), my_plain_sid(),
    showed_(true), enabled_(true), topmost_(false), active(false), focused_(false),
    slider_scrolling(false), mouse_on_control(false),
    slider_position({ 0 }),
    diff_size(0.)
{
}

slider::~slider()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void slider::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto control_pos = position();

    auto slider_width = theme_dimension(tcn, tv_slider_width, theme_);
    auto slider_height = theme_dimension(tcn, tv_slider_height, theme_);
    auto slider_round = theme_dimension(tcn, tv_slider_round, theme_);

    double total = (orientation == slider_orientation::horizontal ? control_pos.width() : control_pos.height()) - static_cast<double>(slider_width) / 2;
    double slider_pos;
    
    if (centered_mode && orientation == slider_orientation::vertical && from < 0 && to > 0)
    {
        // Для centered режима позиция вычисляется отдельно в вертикальном блоке
        slider_pos = 0; // Временное значение, будет пересчитано ниже
    }
    else
    {
        slider_pos = (total * static_cast<double>(value - from)) / static_cast<double>(to - from);
        if (slider_pos < static_cast<double>(slider_width) / 2)
        {
            slider_pos = static_cast<double>(slider_width) / 2;
        }
    }

    auto perform_color = theme_color(tcn, tv_perform, theme_);
    auto remain_color = theme_color(tcn, active || focused_ ? tv_active : tv_remain, theme_);

    auto slider_color = active || focused_ ? remain_color : perform_color;

    if (orientation == slider_orientation::horizontal)
    {
        auto center = control_pos.top + (control_pos.height() / 2) - 1;
        gr.draw_rect({ control_pos.left, center - 1, control_pos.left + static_cast<int32_t>(slider_pos), center + 1 }, perform_color);
        gr.draw_rect({ control_pos.left + static_cast<int32_t>(slider_pos), center - 1, control_pos.right, center + 1 }, remain_color);

        slider_position = { control_pos.left + static_cast<int32_t>(slider_pos) - (slider_width / 2),
                center - (slider_height / 2) + 1,
                control_pos.left + static_cast<int32_t>(slider_pos) + (slider_width / 2),
                center + (slider_height / 2) - 1 };

        gr.draw_rect(slider_position,
            slider_color,
            slider_color,
            1,
            slider_round);
    }
    else if (orientation == slider_orientation::vertical)
    {
        auto center_x = control_pos.left + (control_pos.width() / 2) - 1;
        
        if (centered_mode && from < 0 && to > 0)
        {
            // Режим с 0 посередине: нижняя часть = отрицательные, верхняя = положительные
            const double center_y = control_pos.top + control_pos.height() / 2.0; // Центр всего контрола
            
            // slider_y_pos - это позиция ВЕРХНЕГО края слайдера (top)
            double slider_y_pos;
            if (value < 0)
            {
                // Отрицательные значения: от bottom до center
                // При value == from: нижний край слайдера = bottom, верхний = bottom - slider_width
                // При value == 0: центр слайдера = center_y, верхний = center_y - slider_width/2
                const double neg_range = static_cast<double>(0 - from);
                const double neg_progress = static_cast<double>(value - from) / neg_range;
                // Позиция верхнего края: от (bottom - slider_width) до (center_y - slider_width/2)
                const double bottom_top = control_pos.bottom - slider_width; // Верхний край при value == from
                const double center_top = center_y - static_cast<double>(slider_width) / 2.0; // Верхний край при value == 0
                slider_y_pos = bottom_top - (bottom_top - center_top) * neg_progress;
            }
            else if (value > 0)
            {
                // Положительные значения: от center до top
                // При value == 0: центр слайдера = center_y, верхний = center_y - slider_width/2
                // При value == to: верхний край слайдера = top (ФИКС)
                if (value == to)
                {
                    // Верхний край слайдера точно на top контрола
                    slider_y_pos = static_cast<double>(control_pos.top);
                }
                else
                {
                    const double pos_range = static_cast<double>(to - 0);
                    const double pos_progress = static_cast<double>(value - 0) / pos_range;
                    // Позиция верхнего края: от (center_y - slider_width/2) до (top)
                    const double center_top = center_y - static_cast<double>(slider_width) / 2.0; // Верхний край при value == 0
                    const double top_limit = static_cast<double>(control_pos.top); // Верхний край при value == to
                    slider_y_pos = center_top - (center_top - top_limit) * pos_progress;
                }
            }
            else
            {
                // value == 0: точно в центре, верхний край = center_y - slider_width/2
                slider_y_pos = center_y - static_cast<double>(slider_width) / 2.0;
            }
            
            // Ограничить позицию
            if (slider_y_pos < static_cast<double>(control_pos.top))
            {
                slider_y_pos = static_cast<double>(control_pos.top);
            }
            if (slider_y_pos + slider_width > static_cast<double>(control_pos.bottom))
            {
                slider_y_pos = static_cast<double>(control_pos.bottom) - slider_width;
            }
            
            // Рисовать две части: нижняя (отрицательные) и верхняя (положительные)
            const int32_t center_y_int = static_cast<int32_t>(center_y);
            if (value < 0)
            {
                // Только нижняя часть активна
                gr.draw_rect({ center_x - 1, control_pos.bottom, center_x + 1, center_y_int }, perform_color);
                gr.draw_rect({ center_x - 1, center_y_int, center_x + 1, control_pos.top }, remain_color);
            }
            else if (value > 0)
            {
                // Только верхняя часть активна
                gr.draw_rect({ center_x - 1, center_y_int, center_x + 1, control_pos.top }, perform_color);
                gr.draw_rect({ center_x - 1, control_pos.bottom, center_x + 1, center_y_int }, remain_color);
            }
            else
            {
                // value == 0: обе части неактивны
                gr.draw_rect({ center_x - 1, control_pos.bottom, center_x + 1, control_pos.top }, remain_color);
            }
            
            slider_position = { center_x - (slider_height / 2) + 1,
                    static_cast<int32_t>(slider_y_pos),
                    center_x + (slider_height / 2) - 1,
                    static_cast<int32_t>(slider_y_pos) + slider_width };
        }
        else
        {
            // Обычный режим
            gr.draw_rect({ center_x - 1, control_pos.bottom, center_x + 1, control_pos.bottom - static_cast<int32_t>(slider_pos) }, perform_color);
            gr.draw_rect({ center_x - 1, control_pos.top, center_x + 1, control_pos.bottom - static_cast<int32_t>(slider_pos) }, remain_color);

            slider_position = { center_x - (slider_height / 2) + 1,
                    control_pos.bottom - static_cast<int32_t>(slider_pos) - (slider_width / 2),
                    center_x + (slider_height / 2) - 1,
                    control_pos.bottom - static_cast<int32_t>(slider_pos) + (slider_width / 2) };
        }

        gr.draw_rect(slider_position,
            slider_color,
            slider_color,
            1,
            slider_round);
    }
}

void slider::receive_control_events(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    if (ev.type == event_type::mouse)
    {
        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::enter:
            {
                mouse_on_control = true;
                if (!slider_scrolling)
                {
                    active = true;
                    redraw();
                }
            }
            break;
            case mouse_event_type::leave:
                mouse_on_control = false;
                if (!slider_scrolling)
                {
                    active = false;
                    redraw();
                }
            break;
            case mouse_event_type::left_down:
                if (slider_position.in(ev.mouse_event_.x, ev.mouse_event_.y))
                {
                    slider_scrolling = true;
                }
                else
                {
                    move_slider(ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::left_up:                
                active = false;
                slider_scrolling = false;
                if (drag_end_callback)
                {
                    drag_end_callback();
                }
            break;
            case mouse_event_type::move:
                if (slider_scrolling)
                {
                    move_slider(ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::wheel:
                if (ev.mouse_event_.wheel_delta > 0)
                {
                    scroll_up();
                }
                else
                {
                    scroll_down();
                }
            break;
        }
    }
    else if (ev.type == event_type::keyboard)
    {
        switch (ev.keyboard_event_.type)
        {
            case keyboard_event_type::down:
                switch (ev.keyboard_event_.key[0])
                {
                    case vk_end: case vk_page_up:
                    {
                        value = to;
                        redraw(true);
                        if (change_callback)
                        {
                            change_callback(value);
                        }
                    }
                    break;
                    case vk_home: case vk_page_down:
                    {
                        value = from;
                        redraw(true);
                        if (change_callback)
                        {
                            change_callback(value);
                        }
                    }
                    break;
                    case vk_up: case vk_right:
                        if (value != to)
                        {
                            scroll_up();
                        }
                    break;
                    case vk_down: case vk_left:
                        if (value != from)
                        {
                            scroll_down();
                        }
                    break;
                }
            break;
        }
    }
    else if (ev.type == event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                focused_ = true;
                redraw();
            break;
            case internal_event_type::remove_focus:
                focused_ = false;
                redraw();
            break;
        }
    }
}

void slider::receive_plain_events(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    if (!mouse_on_control && ev.type == event_type::mouse)
    {
        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::move:
                if (slider_scrolling)
                {
                    move_slider(ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::left_up:
                slider_scrolling = false;
            break;
        }
    }
}

void slider::set_position(rect position__)
{
    position_ = position__;

    calc_consts();
}

rect slider::position() const
{
    return get_control_position(position_, parent_);
}

void slider::set_parent(std::shared_ptr<window> window_)
{
    parent_ = window_;

    my_control_sid = window_->subscribe(std::bind(&slider::receive_control_events, this, std::placeholders::_1),
        wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::mouse, wui::event_type::keyboard),
        shared_from_this());

    my_plain_sid = window_->subscribe(std::bind(&slider::receive_plain_events, this, std::placeholders::_1), event_type::mouse);
}

std::weak_ptr<window> slider::parent() const
{
    return parent_;
}

void slider::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->unsubscribe(my_control_sid);
        parent__->unsubscribe(my_plain_sid);
    }
    parent_.reset();
}

void slider::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool slider::topmost() const
{
    return topmost_;
}

bool slider::focused() const
{
    return enabled_ && showed_ && focused_;
}

bool slider::focusing() const
{
    return enabled_ && showed_;
}

error slider::get_error() const
{
    return {};
}

void slider::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void slider::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    redraw();
}

void slider::show()
{
    if (!showed_)
    {
        showed_ = true;
        redraw();
    }
}

void slider::hide()
{
    if (showed_)
    {
        showed_ = false;
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_slider_height, theme_));
            parent__->redraw(pos, true);
        }
    }
}

bool slider::showed() const
{
    return showed_;
}

void slider::enable()
{
    enabled_ = true;
    redraw();
}

void slider::disable()
{
    enabled_ = false;
    redraw();
}

bool slider::enabled() const
{
    return enabled_;
}

void slider::set_range(int32_t from_, int32_t to_)
{
    from = from_;
    to = to_;
    
    // Автоматически определяем centered режим
    if (orientation == slider_orientation::vertical && from_ < 0 && to_ > 0)
    {
        centered_mode = true;
    }
    else
    {
        centered_mode = false;
    }

    calc_consts();

    redraw();
}

void slider::set_value(int32_t value_)
{
    value = value_;
    redraw();
}

int32_t slider::get_value() const
{
    return value;
}

void slider::set_callback(std::function<void(int32_t)> change_callback_)
{
    change_callback = change_callback_;
}

void slider::set_drag_end_callback(std::function<void()> drag_end_callback_)
{
    drag_end_callback = drag_end_callback_;
}

void slider::set_centered_mode(bool centered)
{
    centered_mode = centered;
    calc_consts();
    redraw();
}

void slider::redraw(bool clear)
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_slider_height, theme_));
            parent__->redraw(pos, clear);
        }
    }
}

void slider::calc_consts()
{
    diff_size = static_cast<double>(to - from) / static_cast<double>(orientation == slider_orientation::horizontal ? position_.width() : position_.height());
}

void slider::move_slider(int32_t x, int32_t y)
{
    if (orientation == slider_orientation::horizontal)
    {
        value = from + static_cast<int32_t>((x - position().left) * diff_size);
    }
    else
    {
        if (centered_mode && from < 0 && to > 0)
        {
            // Режим с 0 посередине - обратное преобразование от draw()
            // В draw(): value -> y_pos (центр слайдера)
            // Здесь: y (позиция клика) -> value
            auto control_pos = position();
            auto slider_width = theme_dimension(tcn, tv_slider_width, theme_);
            const double total_height = control_pos.height() - static_cast<double>(slider_width);
            const double center_y = control_pos.top + control_pos.height() / 2.0; // Центр всего контрола
            const double half_height = total_height / 2.0;
            
            // Доступная область для центра слайдера
            const double top_limit = control_pos.top + static_cast<double>(slider_width) / 2.0;
            const double bottom_limit = control_pos.bottom - static_cast<double>(slider_width) / 2.0;
            
            // Ограничить y в доступных пределах (y это центр слайдера)
            double slider_center_y = (std::max)(top_limit, (std::min)(bottom_limit, static_cast<double>(y)));
            
            if (slider_center_y >= center_y)
            {
                // Нижняя часть: отрицательные значения
                // В draw(): slider_center_y = bottom - (half_height) * neg_progress - slider_width/2
                // где neg_progress = (value - from) / (0 - from)
                // Решаем обратно: slider_center_y - bottom + slider_width/2 = -(half_height) * neg_progress
                // neg_progress = (bottom - slider_width/2 - slider_center_y) / half_height
                const double bottom_center = bottom_limit; // bottom - slider_width/2
                
                // ФИКС: Если слайдер в самом низу (около bottom_limit), устанавливаем from
                const double epsilon = 2.0; // Небольшая погрешность в пикселях
                if (std::abs(slider_center_y - bottom_center) < epsilon)
                {
                    value = from; // Минимальное значение (например, -7)
                }
                else
                {
                    const double neg_progress = (bottom_center - slider_center_y) / half_height;
                    const double neg_range = static_cast<double>(0 - from); // например, 7
                    // neg_progress = (value - from) / neg_range
                    // value = from + neg_range * neg_progress
                    value = static_cast<int32_t>(from + neg_range * (std::max)(0.0, (std::min)(1.0, neg_progress))); // от from (-7) до 0
                    // ФИКС: Если получилось 0, но мы близко к центру, оставляем 0
                    if (value > 0) value = 0;
                }
            }
            else
            {
                // Верхняя часть: положительные значения
                // В draw(): slider_center_y = center_y - (half_height) * pos_progress - slider_width/2
                // где pos_progress = value / to
                // Решаем обратно: slider_center_y - center_y + slider_width/2 = -(half_height) * pos_progress
                // pos_progress = (center_y - slider_width/2 - slider_center_y) / half_height
                const double center_center = center_y - static_cast<double>(slider_width) / 2.0;
                
                // ФИКС: Если слайдер в самом верху (около top_limit), устанавливаем to
                const double epsilon = 2.0; // Небольшая погрешность в пикселях
                if (std::abs(slider_center_y - top_limit) < epsilon)
                {
                    value = to; // Максимальное значение (например, +7)
                }
                else
                {
                    const double pos_progress = (center_center - slider_center_y) / half_height;
                    const double pos_range = static_cast<double>(to - 0); // например, 7
                    // pos_progress = value / pos_range
                    // value = pos_range * pos_progress
                    value = static_cast<int32_t>(pos_range * (std::max)(0.0, (std::min)(1.0, pos_progress))); // от 0 до to (+7)
                    // ФИКС: Если значение близко к to (в пределах погрешности округления), устанавливаем точно to
                    if (value < to && pos_progress >= 0.95) // Если прогресс >= 95%, считаем что достигли максимума
                    {
                        value = to;
                    }
                }
            }
        }
        else
        {
            // Обычный режим
            value = from + static_cast<int32_t>((position().bottom - y) * diff_size);
        }
    }

    if (value > to)
    {
        value = to;
    }
    else if (value < from)
    {
        value = from;
    }

    redraw(true);

    if (change_callback)
    {
        change_callback(value);
    }
}

void slider::scroll_up()
{
    if (value == to)
    {
        return;
    }

    int32_t step = static_cast<int32_t>(round(diff_size));
    if (step == 0) {
        step = 1; // Минимум 1 шаг
    }
    value += step;
    if (value > to)
    {
        value = to;
    }

    redraw(true);

    if (change_callback)
    {
        change_callback(value);
    }
}

void slider::scroll_down()
{
    if (value == from)
    {
        return;
    }

    int32_t step = static_cast<int32_t>(round(diff_size));
    if (step == 0) {
        step = 1; // Минимум 1 шаг
    }
    value -= step;
    if (value < from)
    {
        value = from;
    }

    redraw(true);

    if (change_callback)
    {
        change_callback(value);
    }
}

}
