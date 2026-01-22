//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/control/scroll.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/common/flag_helpers.hpp>

#include <cmath>

namespace wui
{

scroll::scroll(int32_t area_, int32_t scroll_pos_,
    orientation orientation__,
    std::function<void(scroll_state, int32_t)> callback_,
    std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), enabled_(true), topmost_(false),
    area(area_),
    scroll_pos(static_cast<double>(scroll_pos_)),
    prev_scroll_pos(0.0),
    scroll_interval(1.0),
    orientation_(orientation__),
    callback(callback_),
    worker_action_(worker_action::undefined),
    worker(),
    worker_started(false),
    progress(0),
    scroll_view_(scroll_view::tiny),
    slider_scrolling(false),
    slider_click_pos(0),
    title_height(0)
{
}

scroll::~scroll()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void scroll::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    rect bar_rect = { 0 }, up_button_rect = { 0 }, down_button_rect = { 0 }, slider_rect = { 0 };
    calc_scrollbar_params(&bar_rect, &up_button_rect, &down_button_rect, &slider_rect);

    if (slider_rect.bottom == 0)
    {
        return;
    }

    gr.draw_rect(bar_rect, theme_color(tcn, tv_background, theme_));

    gr.draw_rect(up_button_rect, theme_color(tcn, tv_slider, theme_));
    if (scroll_view_ == scroll_view::full)
    {
        if (orientation_ == orientation::vertical)
            draw_arrow_up(gr, up_button_rect);
        else
            draw_arrow_left(gr, up_button_rect);
    }

    gr.draw_rect(down_button_rect, theme_color(tcn, tv_slider, theme_));
    if (scroll_view_ == scroll_view::full)
    {
        if (orientation_ == orientation::vertical)
            draw_arrow_down(gr, down_button_rect);
        else
            draw_arrow_right(gr, down_button_rect);
    }

    gr.draw_rect(slider_rect, theme_color(tcn, scroll_view_ == scroll_view::full ? tv_slider_acive : tv_slider, theme_));
}

void scroll::set_position(rect position__)
{
    position_ = position__;

    calc_scroll_interval();
}

rect scroll::position() const
{
    return get_control_position(position_, parent_);
}

void scroll::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;

    my_control_sid = window->subscribe(std::bind(&scroll::receive_control_events, this, std::placeholders::_1),
        wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::mouse, wui::event_type::keyboard),
        shared_from_this());

    my_plain_sid = window->subscribe(std::bind(&scroll::receive_plain_events, this, std::placeholders::_1), event_type::mouse);
}

std::weak_ptr<window> scroll::parent() const
{
    return parent_;
}

void scroll::clear_parent()
{
    end_work();

    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->unsubscribe(my_control_sid);
        my_control_sid.clear();

        parent__->unsubscribe(my_plain_sid);
        my_plain_sid.clear();
    }
    parent_.reset();
}

void scroll::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool scroll::topmost() const
{
    return topmost_;
}

bool scroll::focused() const
{
    return false;
}

bool scroll::focusing() const
{
    return false;
}

error scroll::get_error() const
{
    return {};
}

void scroll::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void scroll::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;
}

void scroll::show()
{
    showed_ = true;
}

void scroll::hide()
{
    showed_ = false;
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->redraw(position(), true);
    }
}

bool scroll::showed() const
{
    return showed_;
}

void scroll::enable()
{
    enabled_ = true;
}

void scroll::disable()
{
    enabled_ = false;
}

bool scroll::enabled() const
{
    return enabled_;
}

void scroll::set_area(int32_t area_)
{
    area = area_;

    calc_scroll_interval();
}

void scroll::set_scroll_pos(int32_t scroll_pos_)
{
    scroll_pos = scroll_pos_;
    if (scroll_pos < 0)
    {
        scroll_pos = 0;
    }

    if (callback && prev_scroll_pos != scroll_pos)
    {
        if (scroll_pos == 0)
        {
            callback(scroll_state::up_end, static_cast<int32_t>(0));
        }
        else if (scroll_pos >= area)
        {
            scroll_pos = area;
            callback(scroll_state::down_end, static_cast<int32_t>(area));
        }
        else
        {
            callback(scroll_state::moving, static_cast<int32_t>(scroll_pos));
        }
    }

    prev_scroll_pos = scroll_pos;
}

int32_t scroll::get_scroll_pos() const
{
    return static_cast<int32_t>(scroll_pos);
}

void scroll::receive_control_events(const event& ev)
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
                if (scroll_view_ != scroll_view::full)
                {
                    scroll_view_ = scroll_view::full;
                    progress = 0;
                    start_work(worker_action::scrollbar_show);
                }
            }
            break;
            case mouse_event_type::leave:
                if (!slider_scrolling)
                {
                    scroll_view_ = scroll_view::tiny;
                }
            break;
            case mouse_event_type::left_down:
            {
                rect bar_rect = { 0 }, up_button_rect = { 0 }, down_button_rect = { 0 }, slider_rect = { 0 };
                calc_scrollbar_params(&bar_rect, &up_button_rect, &down_button_rect, &slider_rect);

                if (up_button_rect.in(ev.mouse_event_.x, ev.mouse_event_.y))
                {
                    start_work(worker_action::scroll_up);
                }
                else if (down_button_rect.in(ev.mouse_event_.x, ev.mouse_event_.y))
                {
                    start_work(worker_action::scroll_down);
                }
                else if (orientation_ == orientation::vertical && ev.mouse_event_.y < slider_rect.top)
                {
                    while (ev.mouse_event_.y < slider_rect.top)
                    {
                        scroll_up();
                        calc_scrollbar_params(&bar_rect, &up_button_rect, &down_button_rect, &slider_rect);
                    }
                }
                else if (orientation_ == orientation::vertical && ev.mouse_event_.y > slider_rect.bottom)
                {
                    while (ev.mouse_event_.y > slider_rect.bottom)
                    {
                        scroll_down();
                        calc_scrollbar_params(&bar_rect, &up_button_rect, &down_button_rect, &slider_rect);
                    }
                }
                else if (orientation_ == orientation::horizontal && ev.mouse_event_.x < slider_rect.left)
                {
                    while (ev.mouse_event_.x < slider_rect.left)
                    {
                        scroll_up();
                        calc_scrollbar_params(&bar_rect, &up_button_rect, &down_button_rect, &slider_rect);
                    }
                }
                else if (orientation_ == orientation::horizontal && ev.mouse_event_.x > slider_rect.right)
                {
                    while (ev.mouse_event_.x > slider_rect.right)
                    {
                        scroll_down();
                        calc_scrollbar_params(&bar_rect, &up_button_rect, &down_button_rect, &slider_rect);
                    }
                }
                else if (slider_rect.in(ev.mouse_event_.x, ev.mouse_event_.y))
                {
                    slider_scrolling = true;
                    slider_click_pos = orientation_ == orientation::vertical ? ev.mouse_event_.y : ev.mouse_event_.x;
                }
            }    
            break;
            case mouse_event_type::left_up:
            {
                slider_scrolling = false;
                end_work();
            }
            break;
            case mouse_event_type::move:
            {
                if (slider_scrolling)
                {
                    move_slider(orientation_ == orientation::vertical ? ev.mouse_event_.y : ev.mouse_event_.x);
                }
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
            default: break;
        }
    }
}

void scroll::receive_plain_events(const event& ev)
{
    if (ev.type == event_type::mouse)
    {
        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::move:
                if (slider_scrolling)
                {
                    move_slider(orientation_ == orientation::vertical ? ev.mouse_event_.y : ev.mouse_event_.x);
                }
            break;
            case mouse_event_type::left_up:
                end_work();

                slider_scrolling = false;

                scroll_view_ = scroll_view::tiny;
                if (callback) callback(scroll_state::relaxed, static_cast<int32_t>(0));
            break;
        }
    }
}

scroll_view scroll::get_scroll_view() const
{
    return scroll_view_;
}

void scroll::draw_arrow_up(graphic& gr, rect button_pos)
{
    auto color = theme_color(tcn, tv_slider_acive, theme_);

    int w = 8, h = 4;

    for (int j = 0; j != h; ++j)
    {
        for (int i = 0; i != w; ++i)
        {
            gr.draw_pixel({ button_pos.left + 3 + j + i, button_pos.top + 8 - j }, color);
        }
        w -= 2;
    }
}

void scroll::draw_arrow_left(graphic& gr, rect button_pos)
{
    auto color = theme_color(tcn, tv_slider_acive, theme_);

    int w = 4, h = 8;

    for (int j = 0; j != w; ++j)
    {
        for (int i = 0; i != h; ++i)
        {
            gr.draw_pixel({ button_pos.left + 8 - j, button_pos.top + 3 + i + j }, color);
        }
        h -= 2;
    }
}

void scroll::draw_arrow_down(graphic& gr, rect button_pos)
{
    auto color = theme_color(tcn, tv_slider_acive, theme_);

    int w = 8, h = 4;

    for (int j = 0; j != h; ++j)
    {
        for (int i = 0; i != w; ++i)
        {
            gr.draw_pixel({ button_pos.left + 3 + j + i, button_pos.top + 5 + j }, color);
        }
        w -= 2;
    }
}

void scroll::draw_arrow_right(graphic& gr, rect button_pos)
{
    auto color = theme_color(tcn, tv_slider_acive, theme_);

    int w = 4, h = 8;

    for (int j = 0; j != w; ++j)
    {
        for (int i = 0; i != h; ++i)
        {
            gr.draw_pixel({ button_pos.left + 5 + j, button_pos.top + 3 + i + j }, color);
        }
        h -= 2;
    }
}

void scroll::move_slider(int32_t v)
{
    if (scroll_interval < 0 || v == slider_click_pos)
    {
        return;
    }

    double delta = v - slider_click_pos;
    slider_click_pos = v;

    scroll_pos += delta * scroll_interval;

    if (scroll_pos < 0)
    {
        scroll_pos = 0;
    }

    if (scroll_pos > area)
    {
        scroll_pos = area;
    }

    if (callback && prev_scroll_pos != scroll_pos)
    {
        if (scroll_pos == 0)
        {
            callback(scroll_state::up_end, static_cast<int32_t>(scroll_pos));
        }
        else if (scroll_pos >= area)
        {
            callback(scroll_state::down_end, static_cast<int32_t>(scroll_pos));
        }
        else
        {
            callback(scroll_state::moving, static_cast<int32_t>(scroll_pos));
        }
    }

    prev_scroll_pos = scroll_pos;
}

void scroll::scroll_up()
{
    if (scroll_pos == 0 || scroll_interval < 0)
    {
        worker_started = false;
        return;
    }

    scroll_pos -= scroll_interval * 10;
    if (scroll_pos < 0)
    {
        scroll_pos = 0;
    }

    if (callback && prev_scroll_pos != scroll_pos)
    {
        callback(scroll_pos == 0 ? scroll_state::up_end : scroll_state::moving,
            static_cast<int32_t>(scroll_pos));
    }

    prev_scroll_pos = scroll_pos;
}

void scroll::scroll_down()
{
    if (scroll_interval < 0 || scroll_pos == area)
    {
        worker_started = false;
        return;
    }

    if (area > scroll_pos)
    {
        scroll_pos += scroll_interval * 10;
        
        if (scroll_pos > area) scroll_pos = area;
    }

    if (callback && prev_scroll_pos != scroll_pos)
    {
        callback(scroll_pos == area ? scroll_state::down_end : scroll_state::moving,
            static_cast<int32_t>(scroll_pos));
    }
    
    prev_scroll_pos = scroll_pos;
}

void scroll::calc_scrollbar_params(rect* bar_rect, rect* up_button_rect, rect* down_button_rect, rect* slider_rect)
{
    if (orientation_ == orientation::vertical)
    {
        calc_vert_scrollbar_params(bar_rect, up_button_rect, down_button_rect, slider_rect);
    }
    else
    {
        calc_hor_scrollbar_params(bar_rect, up_button_rect, down_button_rect, slider_rect);
    }
}

void scroll::calc_scroll_interval()
{
    if (area == 0. || position_.is_null())
    {
        return;
    }

    if (orientation_ == orientation::vertical && position_.height() > full_scrollbar_size * 2)
    {
        scroll_interval = (area + position_.height()) / (position_.height() - full_scrollbar_size * 2);
    }
    else if (orientation_ == orientation::horizontal && position_.width() > full_scrollbar_size * 2)
    {
        scroll_interval = (area + position_.width()) / (position_.width() - full_scrollbar_size * 2);
    } // 2 - is two buttons (up / down)
}

void scroll::calc_vert_scrollbar_params(rect* bar_rect, rect* up_button_rect, rect* down_button_rect, rect* slider_rect)
{
    int32_t scrollbar_width = 0;
    if (scroll_view_ == scroll_view::tiny)
    {
        scrollbar_width = tiny_scrollbar_size;
    }
    else if (scroll_view_ == scroll_view::full)
    {
        scrollbar_width = full_scrollbar_size;
    }
    else
    {
        return;
    }

    const int32_t SB_WIDTH = scrollbar_width,
        SB_HEIGHT = full_scrollbar_size, SB_SILDER_MIN_HEIGHT = 5,
        SB_BUTTON_WIDTH = SB_WIDTH, SB_BUTTON_HEIGHT = SB_HEIGHT;

    auto control_pos = get_control_position(position_, parent_);

    double client_height = control_pos.height() - (SB_HEIGHT * 2);

    if (scroll_interval < 0)
    {
        return;
    }

    if (bar_rect)
    {
        *bar_rect = { control_pos.right - SB_WIDTH, control_pos.top, control_pos.right, control_pos.bottom };
    }

    if (up_button_rect && scroll_view_ == scroll_view::full)
    {
        *up_button_rect = { control_pos.right - SB_BUTTON_WIDTH, control_pos.top, control_pos.right, control_pos.top + SB_BUTTON_HEIGHT };
    }

    if (down_button_rect && scroll_view_ == scroll_view::full)
    {
        *down_button_rect = { control_pos.right - SB_BUTTON_WIDTH, control_pos.bottom - SB_BUTTON_HEIGHT, control_pos.right, control_pos.bottom };
    }

    if (slider_rect)
    {
        auto slider_top = control_pos.top + static_cast<int32_t>(std::round(scroll_pos / scroll_interval));
        auto slider_height = static_cast<int32_t>(std::round(client_height / scroll_interval));

        if (slider_height < SB_SILDER_MIN_HEIGHT)
        {
            slider_height = SB_SILDER_MIN_HEIGHT;
        }

        *slider_rect = { control_pos.right - SB_BUTTON_WIDTH,
            SB_HEIGHT + slider_top,
            control_pos.right,
            SB_HEIGHT + slider_top + slider_height };

        if (scroll_pos == area)
        {
            slider_rect->move(0, control_pos.bottom - SB_BUTTON_HEIGHT - slider_rect->bottom);
        }
    }
}

void scroll::calc_hor_scrollbar_params(rect* bar_rect, rect* up_button_rect, rect* down_button_rect, rect* slider_rect)
{
    int32_t scrollbar_height = 0;
    if (scroll_view_ == scroll_view::tiny)
    {
        scrollbar_height = tiny_scrollbar_size;
    }
    else if (scroll_view_ == scroll_view::full)
    {
        scrollbar_height = full_scrollbar_size;
    }
    else
    {
        return;
    }

    const int32_t SB_WIDTH = full_scrollbar_size,
        SB_SILDER_MIN_WIDTH = 5,
        SB_BUTTON_WIDTH = SB_WIDTH, SB_BUTTON_HEIGHT = SB_WIDTH;

    auto control_pos = get_control_position(position_, parent_);

    double client_width = control_pos.width() - (SB_WIDTH * 2);

    if (scroll_interval < 0)
    {
        return;
    }

    if (bar_rect)
    {
        *bar_rect = { control_pos.left, control_pos.bottom - scrollbar_height, control_pos.right, control_pos.bottom };
    }

    if (up_button_rect && scroll_view_ == scroll_view::full)
    {
        *up_button_rect = { control_pos.left, control_pos.bottom - SB_BUTTON_HEIGHT, control_pos.left + SB_BUTTON_WIDTH, control_pos.bottom };
    }

    if (down_button_rect && scroll_view_ == scroll_view::full)
    {
        *down_button_rect = { control_pos.right - SB_BUTTON_WIDTH, control_pos.bottom - SB_BUTTON_HEIGHT, control_pos.right, control_pos.bottom };
    }

    if (slider_rect)
    {
        auto slider_left = control_pos.left + static_cast<int32_t>(std::round(scroll_pos / scroll_interval));
        auto slider_width = static_cast<int32_t>(std::round(client_width / scroll_interval));

        if (slider_width < SB_SILDER_MIN_WIDTH)
        {
            slider_width = SB_SILDER_MIN_WIDTH;
        }

        *slider_rect = { SB_WIDTH + slider_left,
            control_pos.bottom - scrollbar_height,
            SB_WIDTH + slider_left + slider_width,
            control_pos.bottom };

        if (scroll_pos == area)
        {
            slider_rect->move(control_pos.right - SB_BUTTON_HEIGHT - slider_rect->right, 0);
        }
    }
}

void scroll::start_work(worker_action action)
{
    worker_action_ = action;

    if (!worker_started)
    {
        worker_started = true;
        if (worker.joinable()) worker.join();
        worker = std::thread(std::bind(&scroll::work, this));
    }
}

void scroll::work()
{
    while (worker_started)
    {
        switch (worker_action_)
        {
        case worker_action::scroll_up:
            scroll_up();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        break;
        case worker_action::scroll_down:
            scroll_down();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        break;
        case worker_action::scrollbar_show:
            if (progress < full_scrollbar_size)
            {
                progress += 4;

                auto parent__ = parent_.lock();
                if (parent__)
                {
                    auto control_pos = position();
                    if (orientation_ == orientation::vertical)
                        parent__->redraw({ control_pos.right - progress, control_pos.top, control_pos.right, control_pos.bottom });
                    else
                        parent__->redraw({ control_pos.left, control_pos.bottom - progress, control_pos.right, control_pos.bottom });
                }
            }
            else
            {
                worker_started = false;
                if (callback) callback(scroll_state::activated, static_cast<int32_t>(0));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        break;
        default: break;
        }
    }
}

void scroll::end_work()
{
    worker_started = false;
    if (worker.joinable()) worker.join();
}

}
