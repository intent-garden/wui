//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/tooltip.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

namespace wui
{

tooltip::tooltip(std::string_view text_, std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(false),
    text(text_)
{
}

tooltip::~tooltip()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void tooltip::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto control_position = position();

    gr.draw_rect(control_position,
        theme_color(tcn, tv_border, theme_),
        theme_color(tcn, tv_background, theme_),
        theme_dimension(tcn, tv_border_width, theme_),
        theme_dimension(tcn, tv_round, theme_));

    auto font_ = theme_font(tcn, tv_font, theme_);

    auto text_indent = theme_dimension(tcn, tv_text_indent, theme_);

    auto text_position = control_position;
    text_position.move(text_indent, text_indent);

    gr.draw_text(text_position, text, theme_color(tcn, tv_text, theme_), font_);
}

void tooltip::set_position(rect position__)
{
    position_ = position__;
}

rect tooltip::position() const
{
    return get_control_position(position_, parent_);
}

void tooltip::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;
}

std::weak_ptr<window> tooltip::parent() const
{
    return parent_;
}

void tooltip::clear_parent()
{
    parent_.reset();
}

void tooltip::set_topmost(bool)
{
}

bool tooltip::topmost() const
{
    return true;
}

bool tooltip::focused() const
{
    return false;
}

bool tooltip::focusing() const
{
    return false;
}

error tooltip::get_error() const
{
    return {};
}

void tooltip::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void tooltip::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    redraw();
}

void tooltip::show()
{
    if (showed_)
    {
        return;
    }

    showed_ = true;

    redraw();
}

void tooltip::hide()
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

bool tooltip::showed() const
{
    return showed_;
}

void tooltip::enable()
{
}

void tooltip::disable()
{
}

bool tooltip::enabled() const
{
    return true;
}

void tooltip::set_text(std::string_view text_)
{
    text = text_;

    if (showed_)
    {
        update_size();

        redraw();
    }
}

void tooltip::update_size()
{
    if (text.empty())
    {
        return;
    }

    auto font_ = theme_font(tcn, tv_font, theme_);

    auto old_position = position_;

    auto height = font_.size;
    auto position__ = measure_text(text, font_);

    auto text_indent = theme_dimension(tcn, tv_text_indent, theme_);
    position__.right += text_indent * 2;
    position__.bottom = height + (text_indent * 2);

    position__.move(old_position.left, old_position.top);

    set_position(position__);
}

void tooltip::show_on_control(i_control &control, int32_t indent)
{
    update_size();

    auto pos = get_popup_position(parent_, control.position(), position_, indent);
    
    set_position(pos);
    show();
}

void tooltip::redraw()
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

}
