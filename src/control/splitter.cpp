//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/splitter.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

namespace wui
{

 splitter::splitter(splitter_orientation orientation_, std::function<void(int32_t, int32_t)> callback_, std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : orientation(orientation_),
    callback(callback_),
    margin_min(-1), margin_max(-1),
    tcn(theme_control_name),
    theme_(theme__),
     position_{ 0 },
    parent_(),
    my_control_sid(), my_plain_sid(),
    showed_(true), enabled_(true), active(false), topmost_(false),
    prev_pos{0}
{
}

splitter::~splitter()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void splitter::draw(graphic& gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    gr.draw_rect(position(), theme_color(tcn, active ? tv_active : tv_calm, theme_));
}

void splitter::receive_control_events(const event& ev)
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
            auto parent__ = parent_.lock();
            if (parent__)
            {
                if (orientation == splitter_orientation::vertical)
                {
                    set_cursor(parent__->context(), cursor::size_we);
                }
                else if (orientation == splitter_orientation::horizontal)
                {
                    set_cursor(parent__->context(), cursor::size_ns);
                }
            }
        }
        break;
        case mouse_event_type::leave:
        {
            if (!active)
            {
                auto parent__ = parent_.lock();
                if (parent__)
                {
                    set_cursor(parent__->context(), cursor::default_);
                }
            }
        }
        break;
        case mouse_event_type::left_down:
            active = true;
            redraw();
            break;
        }
    }
}

void splitter::receive_plain_events(const event& ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    switch (ev.mouse_event_.type)
    {
    case mouse_event_type::move:
        if (active)
        {
            auto pos = position_;

            if (orientation == splitter_orientation::vertical)
            {
                pos.put(ev.mouse_event_.x, pos.top);

                auto parent__ = parent_.lock();
                if (parent__ && parent__->parent().lock())
                {
                    auto pp = parent__->position();
                    pos.left -= pp.left;
                    pos.right -= pp.left;
                }
            }
            else if (orientation == splitter_orientation::horizontal)
            {
                pos.put(pos.left, ev.mouse_event_.y);

                auto parent__ = parent_.lock();
                if (parent__ && parent__->parent().lock())
                {
                    auto pp = parent__->position();
                    pos.top -= pp.top;
                    pos.bottom -= pp.top;
                }
            }

            if (orientation == splitter_orientation::vertical)
            {
                if (margin_min != -1 && pos.left <= margin_min)
                {
                    pos = { margin_min, pos.top, margin_min + pos.width(), pos.bottom };
                }
                else if (margin_max != -1 && pos.left >= margin_max)
                {
                    pos = { margin_max - pos.width(), pos.top, margin_max, pos.bottom };
                }
            }
            else if (orientation == splitter_orientation::horizontal)
            {
                if (margin_min != -1 && pos.top <= margin_min)
                {
                    pos = { pos.left, margin_min, pos.right, margin_min + pos.height() };
                }
                else if (margin_max != -1 && pos.bottom >= margin_max)
                {
                    pos = { pos.left, margin_max - pos.height(), pos.right, margin_max };
                }
            }

            if (pos == prev_pos) return;

            set_position(pos);

            if (callback)
            {
                callback(pos.left, pos.top);
                prev_pos = pos;
            }
        }
        break;
        case mouse_event_type::left_up:
            if (active)
            {
                active = false;
                redraw();

                if (!position().in(ev.mouse_event_.x, ev.mouse_event_.y))
                {
                    auto parent__ = parent_.lock();
                    if (parent__)
                    {
                        set_cursor(parent__->context(), cursor::default_);
                    }
                }
            }
        break;
    }
}

void splitter::set_position(rect position__)
{
    position_ = position__;
}

rect splitter::position() const
{
    return get_control_position(position_, parent_);
}

void splitter::set_parent(std::shared_ptr<window> window_)
{
    parent_ = window_;
    
    my_control_sid = window_->subscribe(std::bind(&splitter::receive_control_events, this, std::placeholders::_1), event_type::mouse, shared_from_this());
    my_plain_sid = window_->subscribe(std::bind(&splitter::receive_plain_events, this, std::placeholders::_1), event_type::mouse);
}

std::weak_ptr<window> splitter::parent() const
{
    return parent_;
}

void splitter::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->unsubscribe(my_control_sid);
        parent__->unsubscribe(my_plain_sid);
    }
    parent_.reset();
}

void splitter::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool splitter::topmost() const
{
    return topmost_;
}

bool splitter::focused() const
{
    return false;
}

bool splitter::focusing() const
{
    return false;
}

error splitter::get_error() const
{
    return {};
}

void splitter::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void splitter::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    redraw();
}

void splitter::show()
{
    if (!showed_)
    {
        showed_ = true;
        redraw();
    }
}

void splitter::hide()
{
    if (showed_)
    {
        showed_ = false;
        
        auto parent__ = parent_.lock();
        if (parent__)
        {
            parent__->redraw(position(), true);
        }
    }
}

bool splitter::showed() const
{
    return showed_;
}

void splitter::enable()
{
    enabled_ = true;
    redraw();
}

void splitter::disable()
{
    enabled_ = false;
    redraw();
}

bool splitter::enabled() const
{
    return enabled_;
}

void splitter::set_callback(std::function<void(int32_t, int32_t)> callback_)
{
    callback = callback_;
}

void splitter::set_margins(int32_t min_, int32_t max_)
{
    margin_min = min_;
    margin_max = max_;
}

void splitter::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            parent__->redraw(position());
        }
    }
}

}
