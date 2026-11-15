//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/list.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/common/flag_helpers.hpp>

#include <algorithm>
#include <limits>

namespace wui
{

list::list(std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_control_sid(),
    showed_(true), enabled_(true), focused_(false), mouse_on_control(false), mouse_on_slider(false),
    columns_(),
    mode(list_mode::simple),
    item_count(0), selected_item_(0), active_item_(-1),
    mem_gr(),
    title_height(-1),
    scroll_area(0),
    vert_scroll(std::make_shared<scroll>(0, 0, orientation::vertical, std::bind(&list::on_scroll, this, std::placeholders::_1, std::placeholders::_2), scroll::tc, theme__)),
    draw_callback(),
    item_height_callback(),
    item_change_callback(),
    item_click_callback(),
    column_click_callback(),
    item_activate_callback(),
    scroll_callback()
{
}

list::~list()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

bool list::update_mem_gr()
{
    if (!mem_gr) return false;

    auto width = position_.width(), height = position_.height();

    auto current = mem_gr->max_size();

    if (current.width() < width || current.height() < height)
    {
        mem_gr->release();
        mem_gr->init({ 0, 0, width, height }, theme_color(tcn, tv_background, theme_));
    }

    return true;
}

void list::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto control_pos = position();

    bool ok = update_mem_gr();
    if (!ok) return;

    mem_gr->clear();

    calc_title_height(*mem_gr);

    draw_items(*mem_gr);

    draw_titles(*mem_gr);

    gr.draw_graphic({ control_pos.left,
            control_pos.top,
            control_pos.width(),
            control_pos.height() },
        *mem_gr, 0, 0);

    if ((mouse_on_control || focused_) && has_scrollbar())
    {
        vert_scroll->draw(gr, {});
    }

    auto border_color = focused_
        ? theme_color(tcn, tv_focused_border, theme_)
        : (!mouse_on_control ? theme_color(tcn, tv_border, theme_) : theme_color(tcn, tv_hover_border, theme_));

    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    gr.draw_rect(control_pos,
        border_color,
        make_color(0, 0, 0, 255), //{ theme_color(tcn, tv_background, theme_), 0 },
        border_width,
        theme_dimension(tcn, tv_round, theme_));
}

void list::receive_control_events(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    if (ev.type == event_type::mouse)
    {
        if (has_scrollbar())
        {
            if (vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y))
            {
                event sev = ev;
                if (!mouse_on_slider)
                {
                    mouse_on_slider = true;
                    
                    sev.mouse_event_.type = wui::mouse_event_type::enter;
                    vert_scroll->receive_control_events(sev);
                                        
                    return;
                }
                vert_scroll->receive_control_events(sev);                
                return;
            }
            else
            {
                if (mouse_on_slider)
                {
                    mouse_on_slider = false;
                    
                    event sev = ev;
                    sev.mouse_event_.type = wui::mouse_event_type::leave;
                    vert_scroll->receive_control_events(sev);

                    redraw();
                }
            }
        }

        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::enter:
                if (has_scrollbar())
                {
                    vert_scroll->show();
                }
                mouse_on_control = true;
                redraw();
            break;
            case mouse_event_type::leave:
                mouse_on_control = false;
                mouse_on_slider = false;
                redraw();
            break;
            case mouse_event_type::left_down:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    int32_t pos = 0, n = 0;
                    for (auto &c : columns_)
                    {
                        if (ev.mouse_event_.x >= pos && ev.mouse_event_.x < pos + c.width)
                        {
                            if (column_click_callback)
                            {
                                column_click_callback(n);
                            }
                            return;
                        }

                        pos += c.width;
                        ++n;
                    }
                    return;
                }
                else
                {
                    update_selected_item(ev.mouse_event_.y);
                    
                    if (item_click_callback)
                    {
                        item_click_callback(click_button::left, selected_item_, ev.mouse_event_.x, ev.mouse_event_.y);
                    }
                }
            break;
            case mouse_event_type::right_up:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    return;
                }

                update_selected_item(ev.mouse_event_.y);

                if (item_click_callback)
                {
                    item_click_callback(click_button::right, selected_item_, ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::left_double:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    return;
                }

                update_selected_item(ev.mouse_event_.y);
                
                if (selected_item_ != -1 && item_activate_callback)
                {
                    item_activate_callback(selected_item_);
                }
            break;
            case mouse_event_type::move:
            {
                if (mode == list_mode::simple || mode == list_mode::simple_topmost)
                {
                    update_active_item(ev.mouse_event_.y);
                }
                else if (mode == list_mode::auto_select)
                {
                    update_selected_item(ev.mouse_event_.y);
                }
            }
            break;
            case mouse_event_type::wheel:
                if (mode == list_mode::simple || mode == list_mode::simple_topmost)
                {
                    update_active_item(ev.mouse_event_.y);
                }
                else if (mode == list_mode::auto_select)
                {
                    update_selected_item(ev.mouse_event_.y);
                }

                if (ev.mouse_event_.wheel_delta > 0)
                {
                    vert_scroll->scroll_up();
                }
                else
                {
                    vert_scroll->scroll_down();
                }
            break;
            default: break;
        }
    }
    else if (ev.type == event_type::keyboard)
    {
        switch (ev.keyboard_event_.type)
        {
            case keyboard_event_type::down:
                active_item_ = -1;
                switch (ev.keyboard_event_.key[0])
                {
                    case vk_home: case vk_nhome:
                    {
                        if (selected_item_ != 0)
                        {
                            vert_scroll->set_scroll_pos(0);
                            selected_item_ = 0;
                            redraw();
                        }
                    }
                    break;
                    case vk_end: case vk_nend:
                    {
                        if (selected_item_ != item_count - 1)
                        {
                            vert_scroll->set_scroll_pos(scroll_area - position_.height());

                            selected_item_ = static_cast<int32_t>(item_count - 1);
                            redraw();
                        }
                    }
                    break;
                    case vk_up: case vk_nup:
                        if (selected_item_ != 0)
                        {
                            --selected_item_;
                            
                            auto selected_item_top = get_item_top(selected_item_);
                            if (selected_item_top < vert_scroll->get_scroll_pos())
                            {
                                vert_scroll->set_scroll_pos(selected_item_top);
                            }

                            redraw();
                            
                            if (item_change_callback)
                            {
                                item_change_callback(selected_item_);
                            }
                        }
                    break;
                    case vk_down: case vk_ndown:
                        if (selected_item_ != item_count - 1)
                        {
                            ++selected_item_;
                            
                            auto selected_item_bottom = get_item_top(selected_item_) + get_item_height(selected_item_);
                            if (selected_item_bottom > position_.height())
                            {
                                vert_scroll->set_scroll_pos(selected_item_bottom - position_.height() + title_height);
                            }
                            
                            redraw();

                            if (item_change_callback)
                            {
                                item_change_callback(selected_item_);
                            }
                        }
                    break;
                    case vk_page_up: case vk_npage_up:
                        if (selected_item_ != 0)
                        {
                            const int32_t diff_scroll = 10;

                            if (selected_item_ > diff_scroll)
                            {
                                selected_item_ -= diff_scroll;
                            }
                            else
                            {
                                selected_item_ = 0;
                            }

                            vert_scroll->set_scroll_pos(get_item_top(selected_item_));

                            redraw();

                            if (item_change_callback)
                            {
                                item_change_callback(selected_item_);
                            }
                        }
                    break;
                    case vk_page_down: case vk_npage_down:
                        if (selected_item_ != item_count - 1 && item_count != 0)
                        {
                            int32_t diff_scroll = 10;
                            if (diff_scroll > item_count)
                            {
                                diff_scroll = item_count;
                            }

                            if (selected_item_ < item_count - diff_scroll)
                            {
                                selected_item_ += diff_scroll;
                            }
                            else
                            {
                                selected_item_ = static_cast<int32_t>(item_count - 1);
                            }

                            vert_scroll->set_scroll_pos(get_item_top(selected_item_) + get_item_height(selected_item_) - position_.height() + title_height);
                                                        
                            redraw();

                            if (item_change_callback)
                            {
                                item_change_callback(selected_item_);
                            }
                        }
                    break;
                }
            break;
            case keyboard_event_type::up:
                if (ev.keyboard_event_.key[0] == vk_up || ev.keyboard_event_.key[0] == vk_down)
                {
                    //end_work();
                }
            break;
            default: break;
        }
    }
    else if (ev.type == event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                focused_ = true;

                if (has_scrollbar())
                {
                    vert_scroll->show();
                }

                redraw();
            break;
            case internal_event_type::remove_focus:
                focused_ = false;

                vert_scroll->hide();

                redraw();
            break;
            case internal_event_type::execute_focused:
                if (selected_item_ != -1 && item_activate_callback)
                {
                    item_activate_callback(selected_item_);
                }
            break;
        }
    }
}

void list::set_position(rect position__)
{
    auto height = position__.height();
    bool height_changed = height != position_.height();
    
    position_ = position__;

    auto border_width = theme_dimension(tcn, tv_border_width, theme_) / 2;

    vert_scroll->set_position({ position_.right - 14 - border_width,
        position_.top + border_width,
        position_.right - border_width,
        position_.bottom - border_width });
        
    if (height_changed) update_scroll_area();
}

rect list::position() const
{
    return get_control_position(position_, parent_);
}

void list::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;

    my_control_sid = window->subscribe(std::bind(&list::receive_control_events, this, std::placeholders::_1),
        wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::mouse, wui::event_type::keyboard),
        shared_from_this());

    window->add_control(vert_scroll, { 0 });

    /// Create memory dc for inner content
    if (mem_gr) mem_gr.reset();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        mem_gr = std::make_unique<graphic>(parent__->context());
    }
}

std::weak_ptr<window> list::parent() const
{
    return parent_;
}

void list::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(vert_scroll);

        parent__->unsubscribe(my_control_sid);
        my_control_sid.clear();
    }

    parent_.reset();

    mem_gr.reset();
}

void list::set_topmost(bool yes)
{
    mode = yes ? list_mode::simple_topmost : list_mode::simple;
}

bool list::topmost() const
{
    return mode == list_mode::auto_select || mode == list_mode::simple_topmost;
}

bool list::focused() const
{
    return enabled_ && showed_ && focused_;
}

bool list::focusing() const
{
    return enabled_ && showed_;
}

error list::get_error() const
{
    return {};
}

void list::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void list::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    if (mem_gr)
    {
        mem_gr->set_background_color(theme_color(tcn, tv_background, theme__));
    }
}

void list::show()
{
    if (!showed_)
    {
        showed_ = true;

        vert_scroll->show();

        redraw();
    }
}

void list::hide()
{
    if (showed_)
    {
        showed_ = false;

        vert_scroll->hide();
        
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_border_width, theme_));
            parent__->redraw(pos, true);
        }
    }
}

bool list::showed() const
{
    return showed_;
}

void list::enable()
{
    enabled_ = true;
    redraw();
}

void list::disable()
{
    enabled_ = false;
    redraw();
}

bool list::enabled() const
{
    return enabled_;
}

void list::update_columns(const std::vector<column> &columns__)
{
    columns_ = columns__;
    title_height = -1;
    redraw();
}

const std::vector<list::column> &list::columns()
{
    return columns_;
}

void list::set_mode(list_mode mode_)
{
    mode = mode_;
}

void list::select_item(int32_t n_item)
{
    selected_item_ = n_item;

    redraw();

    if (item_change_callback)
    {
        item_change_callback(selected_item_);
    }
}

int32_t list::selected_item() const
{
    return selected_item_;
}

void list::set_column_width(int32_t n_column, int32_t width)
{
    if (static_cast<int32_t>(columns_.size()) > n_column)
    {
        columns_[n_column].width = width;
        redraw();
    }
}

int32_t list::get_item_height(int32_t n_item) const
{
    if (n_item < 0 || n_item >= item_count) return -1;
    int32_t height = -1;
    if (item_height_callback)
    {
        item_height_callback(n_item, height);
    }
    return height;
}

void list::set_item_count(int32_t count)
{
    item_count = std::max(0, count);

    update_scroll_area();

    redraw();
}

int32_t list::get_item_count() const
{
    return item_count;
}

void list::make_selected_visible()
{
    auto area = position_.height();
    if (area <= 0)
    {
        return;
    }

    auto scroll_pos = vert_scroll->get_scroll_pos();
    
    auto selected_top = get_item_top(selected_item_);
    auto selected_bottom = selected_top + get_item_height(selected_item_);

    auto visible_top = scroll_pos,
         visible_bottom = scroll_pos + area;
        
    if (selected_top < visible_top || selected_bottom > visible_bottom)
    {
        vert_scroll->set_scroll_pos(selected_top);
    }
}

void list::scroll_to_start()
{
    vert_scroll->set_scroll_pos(0);
    redraw();
}

void list::scroll_to_end()
{
    vert_scroll->set_scroll_pos(scroll_area);
    redraw();
}

int32_t list::get_item_top(int32_t n_item) const
{
    if (n_item < 0)
    {
        return 0;
    }

    int64_t top = 0;
    const auto end = std::min(n_item, item_count);
    for (int32_t i = 0; i < end; ++i)
        top += std::max(0, get_item_height(i));
    return static_cast<int32_t>(std::min<int64_t>(top, std::numeric_limits<int32_t>::max()));
}

void list::set_draw_callback(std::function<void(graphic&, int32_t, rect, item_state state)> draw_callback_)
{
    draw_callback = draw_callback_;
}

void list::set_item_height_callback(std::function<void(int32_t, int32_t&)> item_height_callback_)
{
    item_height_callback = item_height_callback_;
}

void list::set_item_click_callback(std::function<void(click_button, int32_t, int32_t, int32_t)> item_click_callback_)
{
    item_click_callback = item_click_callback_;
}

void list::set_item_change_callback(std::function<void(int32_t)> item_change_callback_)
{
    item_change_callback = item_change_callback_;
}

void list::set_item_activate_callback(std::function<void(int32_t)> item_activate_callback_)
{
    item_activate_callback = item_activate_callback_;
}

void list::set_column_click_callback(std::function<void(int32_t)> column_click_callback_)
{
    column_click_callback = column_click_callback_;
}

void list::set_scroll_callback(std::function<void(scroll_state, int32_t)> scroll_callback_)
{
    scroll_callback = scroll_callback_;
}

void list::on_scroll(scroll_state ss, int32_t v)
{
    redraw();

    if (scroll_callback)
    {
        scroll_callback(ss, v);
    }
}

void list::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_border_width, theme_));
            parent__->redraw(pos);
        }
    }
}

void list::redraw_item(int32_t item)
{
    if (showed_)
    {
        auto control_pos = position();

        auto scroll_pos = vert_scroll->get_scroll_pos();

        auto top = control_pos.top + get_item_top(item) + title_height - scroll_pos;

        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto height = get_item_height(item);
            if (height != -1)
            {
                parent__->redraw({ control_pos.left, top, control_pos.right, top + height + 1 });
            }
        }
    }
}

void list::calc_title_height(graphic &gr_)
{
    (void)gr_; // Unused in this implementation
    auto font = theme_font(tcn, tv_font, theme_);
    auto text_indent = 5;

    if (title_height == -1)
    {
        if (columns_.empty())
        {
            title_height = 0;
            return;
        }

        title_height = font.size + text_indent * 2;
    }
}

void list::draw_titles(graphic &gr_)
{
    auto font = theme_font(tcn, tv_font, theme_);
    auto text_indent = 5;

    auto title_color = theme_color(tcn, tv_title, theme_);
    auto title_text_color = theme_color(tcn, tv_title_text, theme_);
    
    int32_t left = 0;

    gr_.draw_rect({ left, 0, position_.width() - 1, title_height }, title_color);

    for (auto &c : columns_)
    {
        gr_.draw_rect({ left, 0, left + c.width - 1, title_height }, title_color);
        gr_.draw_text({ left + text_indent, text_indent, 0, 0 }, c.caption, title_text_color, font);
        
        left += c.width + 1;
    }
}

void list::draw_items(graphic &gr_)
{
    if (!draw_callback || position_.height() <= 0 || position_.width() <= 0) return;

    // Accumulate heights once. Calling get_item_top for every candidate row
    // repeatedly remeasures the entire prefix and becomes quadratic on history.
    const auto scroll_pos = vert_scroll->get_scroll_pos();
    const auto header = std::max(0, title_height);
    int64_t top = static_cast<int64_t>(header) - scroll_pos;
    const int32_t right = position_.width() - (has_scrollbar()
        ? (vert_scroll->get_scroll_view() == scroll_view::full ? 14 : 3) : 0);
    for (int32_t item = 0; item < item_count && top < position_.height(); ++item)
    {
        const auto height = std::max(0, get_item_height(item));
        const auto bottom = top + height;
        if (height > 0 && bottom > header)
        {
            rect item_rect{0, static_cast<int32_t>(top), right,
                static_cast<int32_t>(std::min<int64_t>(bottom, std::numeric_limits<int32_t>::max()))};
            item_state state = item_state::normal;
            if (item == active_item_) state = item_state::active;
            if (item == selected_item_) state = item_state::selected;
            draw_callback(gr_, item, item_rect, state);
        }
        top = bottom;
    }
}

bool list::has_scrollbar()
{
    return scroll_area > 0;
}

int32_t list::item_at(int32_t y) const
{
    const int64_t offset = static_cast<int64_t>(y) - position().top
        - std::max(0, title_height) + vert_scroll->get_scroll_pos();
    if (offset < 0) return -1;
    int64_t top = 0;
    for (int32_t item = 0; item < item_count; ++item)
    {
        const auto height = std::max(0, get_item_height(item));
        const auto bottom = top + height;
        if (height > 0 && offset < bottom) return item;
        top = bottom;
    }
    return -1;
}

void list::update_selected_item(int32_t y)
{
    const auto item = item_at(y);
    if (item != selected_item_)
    {
        const auto old_selected = selected_item_;
        selected_item_ = item;
        redraw_item(old_selected);
        redraw_item(selected_item_);
        if (item_change_callback) item_change_callback(selected_item_);
    }
}

void list::update_active_item(int32_t y)
{
    const auto item = item_at(y);
    if (active_item_ != item)
    {
        active_item_ = item;
        redraw();
    }
}

void list::update_scroll_area()
{
    const int64_t total = static_cast<int64_t>(std::max(0, title_height)) + get_item_top(item_count);
    scroll_area = static_cast<int32_t>(std::min<int64_t>(
        std::max<int64_t>(0, total - position_.height()), std::numeric_limits<int32_t>::max()));
    if (scroll_area < 0)
    {
        scroll_area = 0;
    }

    vert_scroll->set_area(scroll_area);

    if (vert_scroll->get_scroll_pos() > scroll_area)
    {
        vert_scroll->set_scroll_pos(scroll_area);
    }
}

}
