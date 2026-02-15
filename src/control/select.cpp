//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/select.hpp>

#include <wui/control/list.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/common/flag_helpers.hpp>

#include <boost/nowide/convert.hpp>
#include <utf8/utf8.h>

#include <algorithm>

namespace wui
{

static const int32_t select_horizontal_indent = 5;

select::select(std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : items_(),
    change_callback(),
    tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_control_sid(), my_plain_sid(),
    list_theme(make_custom_theme()),
    list_(std::make_shared<list>(list::tc, list_theme)),
    showed_(true), enabled_(true), active(false), topmost_(false),
    focused_(false),
    focusing_(true),
    left_shift(0),
    item_height_(32)
{
    update_list_theme();

    list_->set_mode(list::list_mode::simple_topmost);
    list_->set_draw_callback(std::bind(&select::draw_list_item, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    list_->set_item_height_callback([this](int32_t, int32_t& h) { h = item_height_; });
    list_->set_item_click_callback([this](list::click_button btn, int32_t n_item, int32_t, int32_t) { if (btn == list::click_button::left) activate_list_item(n_item); });
    list_->set_item_activate_callback(std::bind(&select::activate_list_item, this, std::placeholders::_1));
    list_->set_item_change_callback(std::bind(&select::change_list_item, this, std::placeholders::_1));
}

select::~select()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void select::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto control_pos = position();
    auto border_width = theme_dimension(tcn, tv_border_width, theme_);

    auto border_color = focused_
        ? theme_color(tcn, tv_focused_border, theme_)
        : (!active ? theme_color(tcn, tv_border, theme_) : theme_color(tcn, tv_hover_border, theme_));

    /// Draw the frame
    gr.draw_rect(control_pos,
        border_color,
        theme_color(tcn, tv_background, theme_),
        border_width,
        theme_dimension(tcn, tv_round, theme_));

    if (control_pos.width() < control_pos.height())
    {
        return;
    }

    /// Draw the button
    draw_arrow_down(gr, { control_pos.right - static_cast<int32_t>(control_pos.height() / 1.5),
            control_pos.top + static_cast<int32_t>(control_pos.height() / 2.1)});

    auto font_ = theme_font(tcn, tv_font, theme_);

    if (static_cast<int32_t>(items_.size()) <= list_->selected_item())
    {
        return;
    }

    auto text = items_[list_->selected_item()].text;

    auto text_size = measure_text(text, font_, &gr);

    truncate_line(text, gr, font_, control_pos.width() - control_pos.height());

    gr.draw_text({ control_pos.left + border_width + select_horizontal_indent,
        control_pos.top + (control_pos.height() - text_size.height()) / 2 },
        text,
        theme_color(tcn, tv_text, theme_),
        font_);
}

void select::draw_arrow_down(graphic &gr, rect pos)
{
    auto color = theme_color(tcn, !active ? tv_border : tv_focused_border, theme_);

    int w = 8, h = 4;

    for (int j = 0; j != h; ++j)
    {
        for (int i = 0; i != w; ++i)
        {
            gr.draw_pixel({ pos.left + j + i, pos.top + j }, color);
        }
        w -= 2;
    }
}

void select::select_up()
{
    if (!items_.empty() && list_->selected_item() > 0)
    {
        list_->select_item(list_->selected_item() - 1);
        redraw();
    }
}

void select::select_down()
{
    if (!items_.empty() && list_->selected_item() < static_cast<int32_t>(items_.size()) - 1)
    {
        list_->select_item(list_->selected_item() + 1);
        redraw();
    }
}

void select::show_list()
{
    list_->set_position({ position_.left, position_.top, position_.right, position_.top + item_height_ * static_cast<int32_t>(items_.size()) });
    auto pos = get_popup_position(parent_, position(), list_->position(), 0);
    
    list_->set_position(pos);
    list_->show();
    
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->set_focused(list_);
    }

    focused_ = true;
}

void select::receive_control_events(const event &ev)
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
                active = true;
                redraw();
            break;
            case mouse_event_type::leave:
                active = false;
                redraw();
            break;
            case mouse_event_type::wheel:
                if (ev.mouse_event_.wheel_delta > 0)
                {
                    select_up();
                }
                else
                {
                    select_down();
                }
                redraw();
            break;
            case mouse_event_type::left_up:
                if (!list_->showed())
                {
                    show_list();
                }
                else
                {
                    list_->hide();
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
                    case vk_up: case vk_nup:
                        select_up();
                    break;
                    case vk_down: case vk_ndown:
                        select_down();
                    break;
                    case vk_home: case vk_page_up:
                        if (!items_.empty())
                        {
                            list_->select_item(0);
                            redraw();
                        }
                    break;
                    case vk_end: case vk_nend: case vk_page_down: case vk_npage_down:
                        if (!items_.empty())
                        {
                            list_->select_item(static_cast<int32_t>(items_.size() - 1));
                            redraw();
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
                if (focusing_)
                {
                    focused_ = true;

                    redraw();
                }
            break;
            case internal_event_type::remove_focus:
                focused_ = false;
                redraw();
            break;
        }
    }
}

void select::receive_plain_events(const event &ev)
{
    if (!list_->showed())
    {
        return;
    }

    switch (ev.type)
    {
    case event_type::mouse:
        if (ev.mouse_event_.type == mouse_event_type::left_up &&
            !list_->position().in({ ev.mouse_event_.x, ev.mouse_event_.y, ev.mouse_event_.x, ev.mouse_event_.y }) &&
            !position().in({ ev.mouse_event_.x, ev.mouse_event_.y, ev.mouse_event_.x, ev.mouse_event_.y }))
        {
            list_->hide();
        }
        break;
    case event_type::keyboard:
        if (ev.keyboard_event_.type == keyboard_event_type::up)
        {
            if (ev.keyboard_event_.key[0] == vk_esc)
            {
                list_->hide();
            }
        }
        break;
    }
}

void select::update_list_theme()
{
    list_theme->load_theme(theme_ ? *theme_ : *get_default_theme());

    list_theme->set_color(list::tc, list::tv_background, theme_color(tcn, tv_background, theme_));
    list_theme->set_color(list::tc, list::tv_border, theme_color(tcn, tv_border, theme_));
    list_theme->set_color(list::tc, list::tv_focused_border, theme_color(tcn, tv_border, theme_));
    list_theme->set_dimension(list::tc, list::tv_border_width, theme_dimension(tcn, tv_border_width, theme_));
    list_theme->set_color(scroll::tc, scroll::tv_background, theme_color(tcn, tv_scrollbar, theme_));
    list_theme->set_color(scroll::tc, scroll::tv_slider, theme_color(tcn, tv_scrollbar_slider, theme_));
    list_theme->set_color(scroll::tc, scroll::tv_slider_acive, theme_color(tcn, tv_scrollbar_slider_acive, theme_));
    list_theme->set_dimension(list::tc, list::tv_round, theme_dimension(tcn, tv_round, theme_));
}

void select::set_position(rect position__)
{
    position_ = position__;
}

rect select::position() const
{
    return get_control_position(position_, parent_);
}

void select::set_parent(std::shared_ptr<window> window_)
{
    parent_ = window_;

    if (window_)
    {
        window_->add_control(list_, { 0 });
        list_->hide();

        my_control_sid = window_->subscribe(std::bind(&select::receive_control_events, this, std::placeholders::_1),
            wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::mouse, wui::event_type::keyboard),
            shared_from_this());

        my_plain_sid = window_->subscribe(std::bind(&select::receive_plain_events, this, std::placeholders::_1),
            wui::flags_map<wui::event_type>(2, wui::event_type::mouse, wui::event_type::keyboard));
    }
}

std::weak_ptr<window> select::parent() const
{
    return parent_;
}

void select::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(list_);
        
        parent__->unsubscribe(my_control_sid);
        my_control_sid.clear();

        parent__->unsubscribe(my_plain_sid);
        my_plain_sid.clear();
    }
    parent_.reset();
}

void select::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool select::topmost() const
{
    return topmost_;
}

bool select::focused() const
{
    return enabled_ && showed_ && focused_;
}

bool select::focusing() const
{
    return enabled_ && showed_ && focusing_;
}

error select::get_error() const
{
    return {};
}

void select::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void select::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    update_list_theme();
}

void select::show()
{
    showed_ = true;
    redraw();
}

void select::hide()
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

bool select::showed() const
{
    return showed_;
}

void select::enable()
{
    enabled_ = true;
    redraw();
}

void select::disable()
{
    enabled_ = false;
    redraw();
}

bool select::enabled() const
{
    return enabled_;
}

void select::set_items(const select_items_t &items__)
{
    items_ = items__;
    list_->set_item_count(static_cast<int32_t>(items_.size()));
}

void select::update_item(const select_item &si)
{
    auto it = std::find(items_.begin(), items_.end(), si.id);
    if (it != items_.end())
    {
        *it = si;
    }
    list_->set_item_count(static_cast<int32_t>(items_.size()));
}

void select::swap_items(int64_t first_item_id, int64_t second_item_id)
{
    auto first_it = std::find(items_.begin(), items_.end(), first_item_id);
    if (first_it != items_.end())
    {
        auto second_it = std::find(items_.begin(), items_.end(), second_item_id);
        if (second_it != items_.end())
        {
            std::swap(*first_it, *second_it);
        }
    }
    list_->set_item_count(static_cast<int32_t>(items_.size()));
}

void select::delete_item(int64_t id)
{
    auto it = std::find(items_.begin(), items_.end(), id);
    if (it != items_.end())
    {
        items_.erase(it);
    }
    list_->set_item_count(static_cast<int32_t>(items_.size()));
}

void select::set_item_height(int32_t item_height__)
{
    item_height_ = item_height__;
}

void select::select_item_number(int32_t index)
{
    list_->select_item(index);
    redraw();
}

void select::select_item_id(int64_t id)
{
    for (int32_t i = 0; i != static_cast<int32_t>(items_.size()); ++i)
    {
        if (items_[i].id == id)
        {
            select_item_number(i);
            break;
        }
    }
}

select_item select::selected_item() const
{
    auto item_number = list_->selected_item();

    select_item result;
    result.id = -1;

    if (item_number != -1 && item_number < static_cast<int32_t>(items_.size()))
    {
        result = items_[item_number];
    }

    return result;
}

const select_items_t &select::items() const
{
    return items_;
}

void select::set_change_callback(std::function<void(int32_t, int64_t)> change_callback_)
{
    change_callback = change_callback_;
}

void select::redraw()
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

void select::draw_list_item(graphic &gr, int32_t n_item, rect item_rect, list::item_state state)
{
    if (static_cast<int32_t>(items_.size()) <= n_item)
    {
        return;
    }

    auto item = items_[n_item];

    if (state == wui::list::item_state::active)
    {
        gr.draw_rect(item_rect, wui::theme_color(tcn, tv_active_item));
    }
    else if (state == wui::list::item_state::selected)
    {
        gr.draw_rect(item_rect, wui::theme_color(tcn, tv_selected_item));
    }

    auto text_color = theme_color(tcn, tv_text);
    auto font = theme_font(tcn, tv_font);

    auto text = items_[n_item].text;

    auto text_size = measure_text(text, font, &gr);

    truncate_line(text, gr, font, item_rect.width() - item_rect.height());

    auto text_height = text_size.height();
    gr.draw_text({ item_rect.left + select_horizontal_indent, item_rect.top + (item_rect.height() - text_height) / 2 }, text, text_color, font);
}

void select::activate_list_item(int32_t n_item)
{
    (void)n_item; // Unused in this implementation
    list_->hide();
    redraw();
}

void select::change_list_item(int32_t n_item)
{
    if (change_callback)
    {
        change_callback(n_item, n_item < static_cast<int32_t>(items_.size()) ? items_[n_item].id : -1);
    }
}

}
