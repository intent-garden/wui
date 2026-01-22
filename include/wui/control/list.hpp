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
#include <wui/control/scroll.hpp>

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace wui
{

class list : public i_control, public std::enable_shared_from_this<list>
{
public:
    list(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~list();

    virtual void draw(graphic &gr, rect );

    virtual void set_position(rect position);
    virtual rect position() const;

    virtual void set_parent(std::shared_ptr<window> window_);
    virtual std::weak_ptr<window> parent() const;
    virtual void clear_parent();

    virtual void set_topmost(bool yes);
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
    /// List's interface
    struct column
    {
        int32_t width;
        std::string caption;
    };
    void update_columns(const std::vector<column> &columns_);
    const std::vector<column> &columns();

    enum class list_mode
    {
        simple,
        auto_select,
        simple_topmost
    };
    void set_mode(list_mode mode);
    
    void select_item(int32_t n_item);
    int32_t selected_item() const;

    void set_column_width(int32_t n_column, int32_t width);
    int32_t get_item_height(int32_t n_item) const;
    
    void set_item_count(int32_t count);
    int32_t get_item_count() const;

    void make_selected_visible();
    void scroll_to_start();
    void scroll_to_end();

    int32_t get_item_top(int32_t n_item) const;

    enum class item_state
    {
        normal,
        active,
        selected
    };

    enum class click_button
    {
        left,
        center,
        right
    };

    void set_draw_callback(std::function<void(graphic&, int32_t, rect, item_state)> draw_callback_);
    void set_item_height_callback(std::function<void(int32_t, int32_t&)> item_height_callback_);
    void set_item_click_callback(std::function<void(click_button, int32_t, int32_t, int32_t)> item_click_callback_);
    void set_item_change_callback(std::function<void(int32_t)> item_change_callback_);
    void set_item_activate_callback(std::function<void(int32_t)> item_activate_callback_);
    void set_column_click_callback(std::function<void(int32_t)> column_click_callback_);
    void set_scroll_callback(std::function<void(scroll_state, int32_t)> scroll_callback_);

public:
    /// Control name in theme
    static constexpr const char *tc = "list";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_hover_border = "hover_border";
    static constexpr const char *tv_focused_border = "focused_border";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_title = "title";
    static constexpr const char *tv_title_text = "title_text";
    static constexpr const char *tv_selected_item = "selected_item";
    static constexpr const char *tv_active_item = "active_item";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_font = "font";

private:
    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_;
    
    std::weak_ptr<window> parent_;
    std::string my_control_sid;

    bool showed_, enabled_, focused_, mouse_on_control, mouse_on_slider;

    std::vector<column> columns_;

    list_mode mode;

    int32_t item_count, selected_item_, active_item_;

    std::unique_ptr<graphic> mem_gr;

    int32_t title_height;

    int32_t scroll_area;

    std::shared_ptr<scroll> vert_scroll;

    std::function<void(graphic&, int32_t, rect, item_state)> draw_callback;
    std::function<void(int32_t, int32_t&)> item_height_callback;
    std::function<void(click_button, int32_t, int32_t, int32_t)> item_click_callback;
    std::function<void(int32_t)> item_change_callback;
    std::function<void(int32_t)> item_activate_callback;
    std::function<void(int32_t)> column_click_callback;
    std::function<void(scroll_state, int32_t)> scroll_callback;
    
    void receive_control_events(const event &ev);

    void on_scroll(scroll_state, int32_t);

    void redraw();

    void redraw_item(int32_t item);

    void calc_title_height(graphic &gr_);
    void draw_titles(graphic &gr_);

    void draw_items(graphic &gr_);

    bool has_scrollbar();

    void update_selected_item(int32_t y);
    void update_active_item(int32_t y);

    void update_scroll_area();

    bool update_mem_gr();
};

}
