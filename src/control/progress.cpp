//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
//

#include <wui/control/progress.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/event/event.hpp>

#include <wui/common/flag_helpers.hpp>

#include <boost/nowide/convert.hpp>

#include <cstring>

namespace wui
{

progress::progress(int32_t from_, int32_t to_, int32_t value_, orientation orientation__, std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_control_sid(),
    showed_(true), topmost_(false),
    from(from_),
    to(to_),
    value(value_),
    orientation_(orientation__),
    click_callback()
{
}

progress::~progress()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void progress::draw(graphic& gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto control_pos = position();

    gr.draw_rect(control_pos, theme_color(tcn, tv_background, theme_));

    // Checking if the mode is two-way
    bool bidirectional = (from < 0 && to > 0);

    if (bidirectional)
    {
        double half_px = (orientation_ == orientation::horizontal ? control_pos.width() : control_pos.height()) / 2.0;

        rect meter_rect = { 0 };
        color meter_color = theme_color(tcn, tv_meter, theme_);

        if (value > 0)
        {
            meter_color = theme_color(tcn, tv_meter_positive, theme_);
            // How much is the right/upper half filled (0..to )
            double ratio = static_cast<double>(value) / static_cast<double>(to);
            if (ratio > 1.0) ratio = 1.0;

            int32_t offset = static_cast<int32_t>(half_px * ratio);

            if (orientation_ == orientation::horizontal)
            {
                int32_t center_x = control_pos.left + static_cast<int32_t>(half_px);
                meter_rect = { center_x, control_pos.top, center_x + offset, control_pos.bottom };
            }
            else
            {
                int32_t center_y = control_pos.top + static_cast<int32_t>(half_px);
                // Up from the center (in screen coordinates, this is a decrease in Y)
                meter_rect = { control_pos.left, center_y - offset, control_pos.right, center_y };
            }
        }
        else if (value < 0)
        {
            meter_color = theme_color(tcn, tv_meter_negative, theme_);
            // How much is the left/bottom half filled (0..from)
            double ratio = static_cast<double>(value) / static_cast<double>(from); // from negative, value negative -> ratio positive
            if (ratio > 1.0) ratio = 1.0;

            int32_t offset = static_cast<int32_t>(half_px * ratio);

            if (orientation_ == orientation::horizontal)
            {
                int32_t center_x = control_pos.left + static_cast<int32_t>(half_px);
                meter_rect = { center_x - offset, control_pos.top, center_x, control_pos.bottom };
            }
            else
            {
                int32_t center_y = control_pos.top + static_cast<int32_t>(half_px);
                // Down from the center (in screen coordinates, this is an increase in Y)
                meter_rect = { control_pos.left, center_y, control_pos.right, center_y + offset };
            }
        }

        if (value != 0)
        {
            gr.draw_rect(meter_rect, meter_color);
        }
    }
    else
    {
        // Normal mode
        double total_px = orientation_ == orientation::horizontal ? control_pos.width() : control_pos.height();
        double range = static_cast<double>(to - from);
        if (range > 0)
        {
            double meter_px = (total_px * static_cast<double>(value - from)) / range;
            if (orientation_ == orientation::horizontal)
            {
                gr.draw_rect({ control_pos.left, control_pos.top, control_pos.left + static_cast<int32_t>(meter_px), control_pos.bottom },
                    theme_color(tcn, tv_meter, theme_));
            }
            else
            {
                gr.draw_rect({ control_pos.left, control_pos.bottom - static_cast<int32_t>(meter_px), control_pos.right, control_pos.bottom },
                    theme_color(tcn, tv_meter, theme_));
            }
        }
    }

    // Frame
    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    gr.draw_rect(control_pos,
        theme_color(tcn, tv_border, theme_),
        make_color(0, 0, 0, 255),
        border_width,
        theme_dimension(tcn, tv_round, theme_));
}

void progress::set_position(rect position__)
{
    position_ = position__;
}

rect progress::position() const
{
    return get_control_position(position_, parent_);
}

void progress::set_parent(std::shared_ptr<window> window)
{
    auto old_parent = parent_.lock();
    if (old_parent && !my_control_sid.empty())
    {
        old_parent->unsubscribe(my_control_sid);
        my_control_sid.clear();
    }
    
    parent_ = window;
    
    if (window && click_callback)
    {
        // Подписаться на события мыши для обработки кликов
        my_control_sid = window->subscribe(
            std::bind(&progress::receive_control_events, this, std::placeholders::_1),
            flags_map<event_type>(1, event_type::mouse),
            shared_from_this()
        );
    }
}

std::weak_ptr<window> progress::parent() const
{
    return parent_;
}

void progress::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->unsubscribe(my_control_sid);
    }
    parent_.reset();
    my_control_sid.clear();
}

void progress::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool progress::topmost() const
{
    return topmost_;
}

bool progress::focused() const
{
    return false;
}

bool progress::focusing() const
{
    return false;
}

error progress::get_error() const
{
    return {};
}

void progress::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void progress::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    redraw();
}

void progress::show()
{
    showed_ = true;
    redraw();
}

void progress::hide()
{
    showed_ = false;
    auto parent__ = parent_.lock();
    if (parent__)
    {
        auto pos = position();
        pos.widen(theme_dimension(tcn, tv_border_width, theme_));
        parent__->redraw(pos, true);
    }
}

bool progress::showed() const
{
    return showed_;
}

void progress::enable()
{
}

void progress::disable()
{
}

bool progress::enabled() const
{
    return true;
}

void progress::set_range(int32_t from_, int32_t to_)
{
    from = from_;
    to = to_;

    redraw();
}

void progress::set_value(int32_t value_)
{
    value = value_;

    redraw();
}

void progress::set_click_callback(std::function<void(int32_t)> click_callback_)
{
    click_callback = click_callback_;
    
    // Если parent уже установлен и callback задан, подписаться на события
    auto parent__ = parent_.lock();
    if (parent__ && click_callback && my_control_sid.empty())
    {
        my_control_sid = parent__->subscribe(
            std::bind(&progress::receive_control_events, this, std::placeholders::_1),
            flags_map<event_type>(1, event_type::mouse),
            shared_from_this()
        );
    }
    else if (!click_callback && !my_control_sid.empty())
    {
        // Если callback удален, отписаться от событий
        if (parent__)
        {
            parent__->unsubscribe(my_control_sid);
        }
        my_control_sid.clear();
    }
}

void progress::redraw()
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

void progress::receive_control_events(const event& ev)
{
    if (!showed_ || !click_callback)
    {
        return;
    }

    if (ev.type == event_type::mouse && ev.mouse_event_.type == mouse_event_type::left_down)
    {
        auto control_pos = position();
        if (!control_pos.in(ev.mouse_event_.x, ev.mouse_event_.y))
        {
            return;
        }

        int32_t new_value = value;
        bool bidirectional = (from < 0 && to > 0);

        // Determining the relative position of the click (from 0.0 to 1.0)
        double ratio = 0.0;
        if (orientation_ == orientation::horizontal)
        {
            ratio = static_cast<double>(ev.mouse_event_.x - control_pos.left) / control_pos.width();
        }
        else
        {
            // For vertical: 0.0 - bottom (from), 1.0 - top (to)
            ratio = 1.0 - (static_cast<double>(ev.mouse_event_.y - control_pos.top) / control_pos.height());
        }

        if (bidirectional)
        {
            if (ratio > 0.49 && ratio < 0.51) // The dead zone in the center is for pure zero
            {
                new_value = 0;
            }
            else if (ratio >= 0.5)
            {
                // Правая/верхняя половина: отображаем [0.5, 1.0] в [0, to]
                double pos_ratio = (ratio - 0.5) * 2.0;
                new_value = static_cast<int32_t>(pos_ratio * to);
            }
            else
            {
                // Правая/верхняя половина: отображаем [0.5, 1.0] в [0, to]
                // With ratio = 0 it should be from, with ratio = 0.5 it should be 0
                double neg_ratio = ratio * 2.0;
                new_value = static_cast<int32_t>(from * (1.0 - neg_ratio));
            }
        }
        else
        {
            // Normal linear mode
            new_value = static_cast<int32_t>(from + ratio * (to - from));
        }

        // We limit the values just in case
        if (new_value < from) new_value = from;
        if (new_value > to) new_value = to;

        click_callback(new_value);
    }
}

}
