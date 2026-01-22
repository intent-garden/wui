//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/control/i_control.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/event/event.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/color.hpp>

#include <wui/control/list.hpp>

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace wui
{

class image;

struct menu_item;

typedef std::vector<menu_item> menu_items_t;

enum menu_item_state
{
    normal,
    separator,
    expanded,
    disabled
};

struct menu_item
{
    int32_t id;

    menu_item_state state;

    std::string text;

    std::string hotkey;

    std::shared_ptr<image> image_;

    menu_items_t children;

    std::function<void(int32_t)> click_callback;

    int32_t level; menu_item_state prev_state; /// don't fill

    inline bool operator==(int32_t id_)
    {
        return id == id_;
    }
};

class menu : public i_control, public std::enable_shared_from_this<menu>
{
public:
    menu(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~menu();

    virtual void draw(graphic &gr, rect);

    virtual void set_position(rect position);
    virtual rect position() const;

    virtual void set_parent(std::shared_ptr<window> window_);
    virtual std::weak_ptr<window> parent() const;
    virtual void clear_parent();

    virtual void set_topmost(bool);
    virtual bool topmost() const;

    virtual void update_theme_control_name(std::string_view theme_control_name);
    virtual void update_theme(std::shared_ptr<i_theme> theme_ = nullptr);

    virtual void show();
    virtual void hide();
    virtual bool showed() const;

    virtual void enable();
    virtual void disable();
    virtual bool enabled() const;

    virtual bool focused() const;
    virtual bool focusing() const;

    virtual error get_error() const;

public:
    /// Menu's interface
    void set_items(const menu_items_t &mi);
    void update_item(const menu_item &mi);
    void swap_items(int32_t first_item_id, int32_t second_item_id);
    void delete_item(int32_t id);

    void set_item_height(int32_t item_height);

    void show_on_control(std::shared_ptr<i_control> control, int32_t indent, int32_t x = -1, int32_t y = -1);

    void show_on_point(int32_t x, int32_t y);

public:
    /// Control name in theme
    static constexpr const char *tc = "menu";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_text = "text";
    static constexpr const char *tv_disabled_text = "disabled_text";
    static constexpr const char *tv_selected_item = "selected_item";
    static constexpr const char *tv_scrollbar = "scrollbar";
    static constexpr const char *tv_scrollbar_slider = "scrollbar_slider";
    static constexpr const char *tv_scrollbar_slider_acive = "scrollbar_slider_active";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_font = "font";

private:
    std::shared_ptr<i_theme> list_theme;
    std::shared_ptr<list> list_;

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_;

    std::weak_ptr<window> parent_;
    std::string my_subscriber_id;

    std::shared_ptr<i_control> activation_control;
    int32_t indent, x, y;

    std::vector<menu_item> items;

    int32_t max_text_width, max_hotkey_width;

    int32_t item_height_;

    bool showed_;
    bool size_updated;

    void update_list_theme();

    void receive_event(const event &ev);

    void update_size();

    void draw_arrow_down(graphic &gr, rect pos, bool expanded);
    void draw_list_item(wui::graphic &gr, int32_t n_item, rect item_rect_, list::item_state state);
    void activate_list_item(int32_t n_item);
};

}
