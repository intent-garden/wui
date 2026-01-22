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
#include <wui/system/timer.hpp>
#include <wui/control/menu.hpp>
#include <wui/control/scroll.hpp>

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace wui
{

enum class input_view
{
    singleline,
    multiline,
    readonly,
    password
};

enum class input_content
{
    text,
    integer,
    numeric
};

class input : public i_control, public std::enable_shared_from_this<input>
{
public:
    input(std::string_view text = "",
        input_view input_view_ = input_view::singleline,
        input_content input_content_ = input_content::text,
        int32_t symbols_limit = 10000, /// By default, the maximum limit - 10000 symbols
        std::string_view theme_control_name = tc,
        std::shared_ptr<i_theme> theme_ = nullptr);
    ~input();

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
    /// Input's interface
    void set_text(std::string_view text);
    std::string text() const;

    void set_input_view(input_view input_view_);
    void set_input_content(input_content input_content_);
    void set_symbols_limit(int32_t symbols_limit);

    void set_change_callback(std::function<void()> change_callback);
    void set_return_callback(std::function<void()> return_callback);

    const std::vector<std::string>& get_lines() const;

public:
    /// Control name in theme
    static constexpr const char *tc = "input";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_text = "text";
    static constexpr const char *tv_selection = "selection";
    static constexpr const char *tv_cursor = "cursor";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_hover_border = "hover_border";
    static constexpr const char *tv_focused_border = "focused_border";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_font = "font";

    /// Used locale values (from section input)
    static constexpr const char *cl_copy = "copy";
    static constexpr const char *cl_cut = "cut";
    static constexpr const char *cl_paste = "paste";

private:
    input_view input_view_;
    input_content input_content_;
    int32_t symbols_limit;

    // For multiline
    std::vector<std::string> lines_;
    size_t cursor_row = 0, cursor_col = 0;
    
    // Selection multiline
    size_t select_start_row = 0, select_start_col = 0, select_end_row = 0, select_end_col = 0;

    // Scrollbars for multiline
    std::shared_ptr<scroll> vert_scroll;
    std::shared_ptr<scroll> hor_scroll;
    int32_t scroll_offset_x = 0;
    int32_t scroll_offset_y = 0;

    std::function<void()> change_callback;
    std::function<void()> return_callback;

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_;
        
    std::weak_ptr<window> parent_;
    std::string my_control_sid, my_plain_sid;

    timer timer_;

    std::shared_ptr<menu> menu_;

    bool showed_, enabled_, topmost_;
    bool active, focused_;
    bool cursor_visible;
    bool selecting;

    std::unique_ptr<graphic> mem_gr;

    // Cache for maximum line width to avoid expensive recalculations
    int cached_max_width_ = -1;
    bool max_width_dirty_ = true;
    
    // Auto-scroll timer for mouse selection
    enum class auto_scroll_type {
        idle = 0, left, right, up, down
    };
    std::shared_ptr<timer> auto_scroll_timer_;
    auto_scroll_type auto_scroll_type_ = auto_scroll_type::idle;
    void start_auto_scroll(bool up = false);
    void start_auto_hscroll(bool left = false);
    void stop_auto_scroll();
    void on_auto_scroll();

    void receive_control_events(const event &ev);
    void receive_plain_events(const event &ev);

    void redraw();

    void redraw_cursor();

    // Multiline helpers
    void update_lines(std::string_view text);
    void reset_state();

    // Selections and cursor
    bool clear_selected_text(); /// returns true if selection is not empty
    std::pair<size_t, size_t> calculate_mouse_cursor_position(int x, int y);
    
    // Selection helper
    void select_all();
    void select_current_word(int x, int y);
    
    // Clipboard
    void buffer_copy();
    void buffer_cut();
    void buffer_paste();

    // Scrolling methods
    void update_scroll_areas();
    void on_vert_scroll(scroll_state ss, int32_t v);
    void on_hor_scroll(scroll_state ss, int32_t v);
    void update_scroll_visibility();
    void scroll_to_cursor();
    
    // Cache management for performance
    int get_max_line_width();
    void invalidate_max_width_cache();

    // Get font
    font get_font();

    bool update_mem_gr();
};

}
