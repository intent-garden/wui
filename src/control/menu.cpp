//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
//

#include <wui/control/menu.hpp>

#include <wui/window/window.hpp>

#include <wui/control/image.hpp>
#include <wui/control/list.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>
#include <wui/common/flag_helpers.hpp>

#include <algorithm>

#undef min
#undef max

namespace wui
{

int32_t calc_items_count(const menu_items_t &items)
{
    int32_t count = 0;
    for (auto &item : items)
    {
        ++count;
        if (!item.children.empty() && item.state == menu_item_state::expanded)
        {
            count += calc_items_count(item.children);
        }
    }
    return count;
}

menu_item *get_item(menu_items_t &items, int32_t n_item, int32_t pos = 0, int32_t level = 0)
{
    int32_t count = pos;
    for (auto &item : items)
    {
        if (count == n_item)
        {
            item.level = level;
            return &item;
        }

        if (!item.children.empty() && item.state == menu_item_state::expanded)
        {
            auto children_count = calc_items_count(item.children);
            if (n_item <= count + children_count)
            {
                return get_item(item.children, n_item - 1, count, level + 1);
            }
            else
            {
                count += children_count;
            }
        }

        ++count;
    }

    return nullptr;
}

menu::menu(std::string_view theme_control_name, std::shared_ptr<i_theme> theme__)
    : list_theme(make_custom_theme()),
    list_(std::make_shared<list>(list::tc, list_theme)),
    tcn(theme_control_name),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_subscriber_id(),
    activation_control(),
    indent(0), x(-1), y(-1),
    items(),
    max_text_width(0), max_hotkey_width(0),
    item_height_(32),
    showed_(false),
    size_updated(false)
{
    update_list_theme();

    list_->set_draw_callback(std::bind(&menu::draw_list_item, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    list_->set_item_height_callback([this](int32_t, int32_t& h) { h = item_height_; });
    list_->set_item_click_callback([this](list::click_button btn, int32_t n_item, int32_t, int32_t) { if (btn == list::click_button::left) activate_list_item(n_item); });
    list_->set_item_activate_callback(std::bind(&menu::activate_list_item, this, std::placeholders::_1));
    list_->set_mode(list::list_mode::auto_select);
    list_->hide();
}

menu::~menu()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(list_);
    }
}

void menu::draw(graphic &, rect )
{
}

void menu::set_position(rect position__)
{
    list_->set_position(position__);
}

rect menu::position() const
{
    return list_->position();
}

void menu::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;

    if (window)
    {
        window->add_control(list_, { 0 });

        my_subscriber_id = window->subscribe(std::bind(&menu::receive_event, this, std::placeholders::_1), 
            wui::flags_map<wui::event_type>(2, wui::event_type::mouse, wui::event_type::keyboard));
    }
}

std::weak_ptr<window> menu::parent() const
{
    return parent_;
}

void menu::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(list_);
        parent__->unsubscribe(my_subscriber_id);
    }

    parent_.reset();
}

void menu::update_list_theme()
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

    list_->update_theme(list_theme);
}

void menu::receive_event(const event &ev)
{
    if (!list_->showed())
    {
        return;
    }

    switch (ev.type)
    {
        case event_type::mouse:
        {
            auto mouse_x = ev.mouse_event_.x, mouse_y = ev.mouse_event_.y;

            bool mouse_btn_up = ev.mouse_event_.type == mouse_event_type::left_up ||
                ev.mouse_event_.type == mouse_event_type::right_up;

            bool outside_list = !list_->position().in({ mouse_x, mouse_y, mouse_x, mouse_y });

            bool outside_activation =
                !activation_control ||
                !activation_control->position().in({ mouse_x, mouse_y, mouse_x, mouse_y });

            if (mouse_btn_up && outside_list && outside_activation)
            {
                list_->hide();
            }
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

void menu::set_topmost(bool)
{
}

bool menu::topmost() const
{
    return true;
}

bool menu::focused() const
{
    return false;
}

bool menu::focusing() const
{
    return false;
}

error menu::get_error() const
{
    return {};
}

void menu::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void menu::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    update_list_theme();

    size_updated = false;
}

void menu::show()
{
    if (showed_)
    {
        return;
    }

    showed_ = true;

    list_->show();
}

void menu::hide()
{
    showed_ = false;

    list_->hide();
}

bool menu::showed() const
{
    return showed_;
}

void menu::enable()
{
}

void menu::disable()
{
}

bool menu::enabled() const
{
    return true;
}

void menu::set_items(const menu_items_t &items_)
{
    items = items_;
    list_->set_item_count(calc_items_count(items));
    size_updated = false;
}

void menu::update_item(const menu_item &mi)
{
    auto it = std::find(items.begin(), items.end(), mi.id);
    if (it != items.end())
    {
        *it = mi;
    }
    list_->set_item_count(calc_items_count(items));
}

void menu::swap_items(int32_t first_item_id, int32_t second_item_id)
{
    auto first_it = std::find(items.begin(), items.end(), first_item_id);
    if (first_it != items.end())
    {
        auto second_it = std::find(items.begin(), items.end(), second_item_id);
        if (second_it != items.end())
        {
            std::swap(*first_it, *second_it);
        }
    }
    list_->set_item_count(calc_items_count(items));
}

void menu::delete_item(int32_t id)
{
    auto it = std::find(items.begin(), items.end(), id);
    if (it != items.end())
    {
        items.erase(it);
        size_updated = false;
    }
    list_->set_item_count(calc_items_count(items));
}

void menu::set_item_height(int32_t item_height__)
{
    item_height_ = item_height__;
    size_updated = false;
}

void menu::update_size()
{
    if (size_updated || items.empty())
    {
        return;
    }
    
    auto font_ = theme_font(tcn, tv_font, theme_);

    max_text_width = 0, max_hotkey_width = 0;

    auto items_count = calc_items_count(items);
    for (int i = 0; i != items_count; ++i)
    {
        auto *item = get_item(items, i);
        if (!item)
        {
            continue;
        }

        auto text_width = measure_text(item->text, font_).right;
        auto hotkey_width = measure_text(item->hotkey, font_).right;
        if (hotkey_width != 0)
        {
            hotkey_width += item_height_;
        }
        if (hotkey_width > max_hotkey_width)
        {
            max_hotkey_width = hotkey_width;
        }
        
        auto width = (item->level * item_height_) + text_width + max_hotkey_width + (item_height_ * 3);
        if (width > max_text_width)
        {
            max_text_width = width;
        }
    }

    int32_t height = item_height_ * items_count;

    position_ = { 0, 0, max_text_width, height };

    size_updated = true;
}

void menu::show_on_control(std::shared_ptr<i_control> control, int32_t indent_, int32_t x_, int32_t y_)
{
    activation_control = control;
    indent = indent_;
    x = x_;
    y = y_;

    rect prev_pos = list_->position();
    update_size();

    auto base_pos = control ? control->position() : rect{ 0 };

    if (x != -1)
    {
        base_pos = { x, base_pos.top, base_pos.right, base_pos.bottom };
        if (base_pos.right < x)
        {
            base_pos.right = x;
        }
    }
    if (y != -1)
    {
        base_pos = { base_pos.left, y, base_pos.right, base_pos.bottom };
        if (base_pos.bottom < y)
        {
            base_pos.bottom = y;
        }
    }

    auto pos = get_popup_position(parent_, base_pos, position_, indent);

    list_->set_position(pos);
    list_->show();
    
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->set_focused(list_);
        rect redraw_pos = { std::min(prev_pos.left, pos.left),
            std::min(prev_pos.top, pos.top),
            std::max(prev_pos.right, pos.right),
            std::max(prev_pos.bottom, pos.bottom) };
        redraw_pos.widen(theme_dimension(tcn, tv_border_width));
        parent__->redraw(redraw_pos, true);
    }
}

void menu::show_on_point(int32_t x_, int32_t y_)
{
    show_on_control(nullptr, 0, x_, y_);
}

void menu::draw_arrow_down(graphic &gr, rect pos, bool expanded)
{
    auto color = theme_color(tcn, !expanded ? tv_text : tv_scrollbar_slider_acive, theme_);

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

void menu::draw_list_item(graphic &gr, int32_t n_item, rect item_rect, list::item_state state)
{
    auto item = get_item(items, n_item);
    if (!item)
    {
        return;
    }

    auto border_width = theme_dimension(tcn, tv_border_width);

    if (state == list::item_state::selected)
    {
        gr.draw_rect({ item_rect.left, item_rect.top, item_rect.right, item_rect.bottom }, theme_color(tcn, tv_selected_item));
    }
    
    if (item->image_)
    {
        auto img_size = static_cast<int32_t>(item_rect.height() * 0.9);

        auto indent_ = static_cast<int32_t>((item_rect.height() - img_size) / 2);

        rect img_rect = { item_rect.left + indent_,
            item_rect.top + indent_,
            item_rect.left + img_size + indent_,
            item_rect.top + img_size + indent_ };

        item->image_->set_position(img_rect);
        item->image_->draw(gr, { 0 });
    }

    auto text_color = item->state != menu_item_state::disabled ? theme_color(tcn, tv_text) : theme_color(tcn, tv_disabled_text);
    auto font = theme_font(tcn, tv_font);

    auto text_height = font.size;
    
    gr.draw_text({ item_rect.left + item_rect.height() + item_rect.height() * item->level, item_rect.top + (item_rect.height() - text_height) / 2 }, item->text, text_color, font);

    if (!item->hotkey.empty())
    {
        gr.draw_text({ item_rect.right - max_hotkey_width, item_rect.top + (item_rect.height() - text_height) / 2 }, item->hotkey, text_color, font);
    }

    if (!item->children.empty())
    {
        auto height = item_rect.height();

        auto left = item_rect.right - item_rect.height() + (height - 8) / 2,
            top = item_rect.top + (height - 4) / 2;

        draw_arrow_down(gr, { left, top }, item->state == menu_item_state::expanded);
    }

    if (item->state == menu_item_state::separator && item_rect.bottom <= list_->position().bottom - border_width)
    {
        gr.draw_line({ item_rect.left, item_rect.bottom, item_rect.right, item_rect.bottom }, text_color);
    }
}

void menu::activate_list_item(int32_t n_item)
{
    auto item = get_item(items, n_item);
    if (!item)
    {
        return;
    }

    if (!item->children.empty())
    {
        if (item->state != menu_item_state::expanded)
        {
            item->prev_state = item->state;
            item->state = menu_item_state::expanded;
        }
        else
        {
            item->state = item->prev_state;
        }
        size_updated = false;
        list_->set_item_count(calc_items_count(items));
        show_on_control(activation_control, indent, x, y);
    }
    
    if (item->children.empty() && item->state != menu_item_state::disabled)
    {
        list_->hide();
    }

    if (item->click_callback && item->state != menu_item_state::disabled)
    {
        item->click_callback(n_item);
    }
}

}
