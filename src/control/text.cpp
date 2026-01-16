//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
//

#include <wui/control/text.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <cstring>
#include <vector>
#include <sstream>

namespace wui
{

text::text(std::string_view text__,
    hori_alignment hori_alignment__, vert_alignment vert_alignment__,
    std::string_view theme_control_name,
    std::shared_ptr<i_theme> theme_)
    : tcn(theme_control_name),
    theme_(theme_),
    position_{ 0 },
    parent_(),
    showed_(true), topmost_(false),
    text_(text__),
    hori_alignment_(hori_alignment__), vert_alignment_(vert_alignment__)
{
}

text::~text()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void text::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    const auto space_coeff = 1.2;

    auto font_ = theme_font(tcn, tv_font, theme_);

    std::stringstream text__(text_);
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(text__, line, '\n'))
    {
        lines.push_back(line);
    }

    auto line_height = font_.size;

    auto control_pos = position();

    int32_t line_top = control_pos.top;
    switch (vert_alignment_)
    {
        case vert_alignment::top:
            // line_top = control_pos.top;
        break;
        case vert_alignment::center:
            line_top = control_pos.top + static_cast<int32_t>((control_pos.height() - line_height * lines.size() * space_coeff) / 2);
        break;
        case vert_alignment::bottom:
            line_top = control_pos.bottom - static_cast<int32_t>(line_height * lines.size() * space_coeff);
        break;
    }

    for (auto &line_text : lines)
    {
        truncate_line(line_text, gr, font_, control_pos.width());

        int32_t left = control_pos.left;

        switch (hori_alignment_)
        {
            case hori_alignment::left:
                // do nothing
            break;
            case hori_alignment::center:
            {
                auto line_width = measure_text(line_text, font_, &gr).width();
                left += ((control_pos.width() - line_width) / 2);
            }
            break;
            case hori_alignment::right:
            {
                auto line_width = measure_text(line_text, font_, &gr).width();
                left += (control_pos.width() - line_width);
            }
            break;
        }
        
        gr.draw_text({ left, line_top }, line_text, theme_color(tcn, tv_color, theme_), font_);

        line_top += static_cast<int32_t>(line_height * space_coeff);

        if (line_top + line_height > control_pos.bottom)
        {
            break;
        }
    }
}

void text::set_position(rect position__)
{
    position_ = position__;
}

rect text::position() const
{
    return get_control_position(position_, parent_);
}

void text::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;
}

std::weak_ptr<window> text::parent() const
{
    return parent_;
}

void text::clear_parent()
{
    parent_.reset();
}

void text::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool text::topmost() const
{
    return topmost_;
}

bool text::focused() const
{
    return false;
}

bool text::focusing() const
{
    return false;
}

error text::get_error() const
{
    return {};
}

void text::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void text::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    redraw();
}

void text::show()
{
    showed_ = true;
    redraw();
}

void text::hide()
{
    showed_ = false;
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->redraw(position(), true);
    }
}

bool text::showed() const
{
    return showed_;
}

void text::enable()
{
}

void text::disable()
{
}

bool text::enabled() const
{
    return true;
}

void text::set_text(std::string_view text__)
{
    text_ = text__;
    redraw();
}

std::string_view text::get_text() const
{
    return text_;
}

void text::set_alignment(hori_alignment hori_alignment__, vert_alignment vert_alignment__)
{
    hori_alignment_ = hori_alignment__;
    vert_alignment_ = vert_alignment__;

    redraw();
}

void text::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            parent__->redraw(position(), true);
        }
    }
}

}
