//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/control/panel.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

namespace wui
{

panel::panel(std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), topmost_(false),
    draw_callback()
{
}

panel::panel(std::function<void(graphic&)> draw_callback_, std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), topmost_(false),
    draw_callback(draw_callback_)
{
}

panel::~panel()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void panel::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    gr.draw_rect(position(), theme_color(tcn, tv_background, theme_));

    if (draw_callback)
    {
        draw_callback(gr);
    }
}

void panel::set_position(rect position__)
{
    position_ = position__;
}

rect panel::position() const
{
    return get_control_position(position_, parent_);
}

void panel::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;
}

std::weak_ptr<window> panel::parent() const
{
    return parent_;
}

void panel::clear_parent()
{
    parent_.reset();
}

void panel::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool panel::topmost() const
{
    return topmost_;
}

bool panel::focused() const
{
    return false;
}

bool panel::focusing() const
{
    return false;
}

error panel::get_error() const
{
    return {};
}

void panel::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void panel::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    redraw();
}

void panel::show()
{
    showed_ = true;
    redraw();
}

void panel::hide()
{
    showed_ = false;
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->redraw(position(), true);
    }
}

bool panel::showed() const
{
    return showed_;
}

void panel::enable()
{
}

void panel::disable()
{
}

bool panel::enabled() const
{
    return true;
}

void panel::redraw()
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
