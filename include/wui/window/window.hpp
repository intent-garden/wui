//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/window/i_window.hpp>
#include <wui/system/system_context.hpp>
#include <wui/control/i_control.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/common/rect.hpp>

#include <atomic>
#include <vector>
#include <memory>
#include <mutex>

#ifdef __linux__
#include <wui/window/listener.h>
#endif

namespace wui
{

enum class window_state
{
    normal,
    minimized,
    maximized,
    pinned
};

enum class window_control
{
    close,
    state,
    pin,
    theme,
	lang
};

class button;
class udev_handler;
class listener;

class window : public i_window, public i_control, public std::enable_shared_from_this<window>
{
public:
    window(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    virtual ~window();

    /// i_window impl
    virtual bool init(std::string_view caption, rect position, window_style style, std::function<void(void)> close_callback = []() {});
    virtual void destroy();

    virtual void add_control(std::shared_ptr<i_control> control, rect position);
    virtual void remove_control(std::shared_ptr<i_control> control);

    virtual void bring_to_front(std::shared_ptr<i_control> control);
    virtual void move_to_back(std::shared_ptr<i_control> control);

    virtual void redraw(rect position, bool clear = false);

    virtual std::string subscribe(std::function<void(const event&)> receive_callback, event_type event_types, std::shared_ptr<i_control> control = nullptr);
    virtual void unsubscribe(std::string_view subscriber_id);

    virtual system_context &context();

	/// i_control impl
    virtual void draw(graphic &gr, rect paint_rect);

    virtual void set_position(rect position);
    virtual rect position() const;

    virtual void set_parent(std::shared_ptr<window> window_);
    virtual std::weak_ptr<window> parent() const;
    virtual void clear_parent();

    virtual void set_topmost(bool yes);
    virtual bool topmost() const;

    virtual bool focused() const;
    virtual bool focusing() const;

    virtual void update_theme_control_name(std::string_view theme_control_name);
    virtual void update_theme(std::shared_ptr<i_theme> theme_ = nullptr);

    virtual void show();
    virtual void hide();
    virtual bool showed() const;

    virtual void enable();
    virtual void disable();
    virtual bool enabled() const;

    virtual error get_error() const;

    /// Window style methods
    void set_caption(std::string_view caption);
    void set_style(window_style style);
    void set_min_size(int32_t width, int32_t height);

    void set_transient_for(std::shared_ptr<window> window_, bool docked = true);

    /// Window state methods
	void switch_lang();
    void switch_theme();
    void pin();
    void minimize();
    void expand();
    void normal();
    window_state state() const;

    /// Disabling draw for increase performance on mass operations
    void disable_draw();
    void enable_draw();
    bool draw_enabled() const;

    /// Emit event methods
    void emit_event(int32_t x, int32_t y);
    
    /// Method to set the focus of the child control
    void set_focused(std::shared_ptr<i_control> control);

    /// Callbacks
    void set_control_callback(std::function<void(window_control control, std::string &text, bool &continue_)> callback);

    /// Set the control (button) to be pressed when user press enter on the keyboard
    void set_default_push_control(std::shared_ptr<i_control> control);

    /// Enable/disable the window to receive events about connecting / disconnecting devices (will be sent to system_event)
    /// Call this only after the window::init()
    void enable_device_change_handling(bool yes);

    /// Direct link to window's graphic
    graphic &get_graphic();

    /// Get the type of window
    bool is_physical_window() const;

    /// Set this window always physical
    void set_root_window(bool yes);

public:
    /// Control name in theme / locale
    static constexpr const char *tc = "window";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_text = "text";
    static constexpr const char *tv_caption_font = "caption_font";

    ///Used theme images
    static constexpr const char *ti_close = "window_close";
    static constexpr const char *ti_expand = "window_expand";
    static constexpr const char *ti_normal = "window_normal";
    static constexpr const char *ti_minimize = "window_minimize";
    static constexpr const char *ti_pin = "window_pin";
    static constexpr const char *ti_switch_theme = "window_switch_theme";
	static constexpr const char *ti_switch_lang = "window_switch_lang";

    /// Used locale values (from section window)
    static constexpr const char *cl_pin = "pin";
    static constexpr const char *cl_unpin = "unpin";
    static constexpr const char *cl_dark_theme = "dark_theme";
    static constexpr const char *cl_light_theme = "light_theme";
	static constexpr const char *cl_switch_lang = "switch_lang";

private:
    system_context context_;
    graphic graphic_;

    std::vector<std::shared_ptr<i_control>> controls;
    std::shared_ptr<i_control> active_control;

    std::string caption;
    rect position_, parent_position_, normal_position;
    int32_t min_width, min_height;
    window_style window_style_;
    wui::window_state window_state_, prev_window_state_;

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    bool showed_, enabled_, root_window_;
    std::atomic<bool> skip_draw_;

    size_t focused_index;

    std::weak_ptr<window> parent_;
    std::string my_control_sid, my_plain_sid;

    std::weak_ptr<window> transient_window;
    bool docked_;
    std::shared_ptr<i_control> docked_control;

    struct event_subscriber
    {
        std::string id;
        std::function<void(const event&)> receive_callback;
        event_type event_types;
        std::shared_ptr<i_control> control;
    };
    std::vector<event_subscriber> subscribers_;
    std::mutex subscribers_mutex_;

    enum class moving_mode
    {
        none,
        move,
        size_we_left,
        size_we_right,
        size_ns_top,
        size_ns_bottom,
        size_nwse_top,
        size_nwse_bottom,
        size_nesw_top,
        size_nesw_bottom
    };
    moving_mode moving_mode_;

    int16_t x_click, y_click;

    error err;

    std::function<void(void)> close_callback;
    std::function<void(window_control control, std::string &text, bool &continue_)> control_callback;

    std::shared_ptr<i_control> default_push_control;

    std::shared_ptr<button> switch_lang_button, switch_theme_button, pin_button, minimize_button, expand_button, close_button;

#ifdef _WIN32

    bool mouse_tracked;
    bool device_change_handling;

    HDEVNOTIFY dev_notify_handle;

    static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM w_param, LPARAM l_param);

    static void ProcessDeviceChanges(window* wnd, WPARAM w, LPARAM l);

#elif __linux__

    xcb_atom_t wm_protocols_event,
        wm_delete_msg,
        wm_change_state,
        net_wm_state,
        net_wm_state_focused,
        net_wm_state_above,
        net_wm_state_skip_taskbar,
        net_wm_name,
        utf8_string,
        net_active_window,
        net_wm_state_fullscreen,
        net_wm_state_maximized_vert, net_wm_state_maximized_horz,
        net_wm_moveresize;

    time_t prev_button_click;

    bool started;

    std::unique_ptr<udev_handler> udev_handler_;
    
    uint8_t key_modifier;

    void process_events(xcb_generic_event_t &e);

    void init_atoms();

    void send_destroy_event();

    void change_style(xcb_atom_t type, xcb_atom_t action, xcb_atom_t style) noexcept;

    void update_window_style();

    void set_wm_name(std::string_view caption);

#endif
    void receive_control_events(const event &ev);
    void receive_plain_events(const event &ev);

    void send_event_to_control(const std::shared_ptr<i_control> &control, const event &ev);
    void send_event_to_plains(const event &ev);
    void send_event_to_plains_and_control(const event& ev, const std::shared_ptr<i_control>& control);

    void send_mouse_event(const mouse_event &ev);

    bool check_control_here(int32_t x, int32_t y);

    void change_focus();
    void execute_focused();
    void set_focused(size_t index);
    std::shared_ptr<i_control> get_focused();

    void start_docking(std::shared_ptr<i_control> control);
    void end_docking();
	std::shared_ptr<window> get_transient_window();

    void update_button_images();
    void update_buttons();

    void draw_border(graphic &gr);
    void draw_caption(graphic& gr, rect paint_rect);

    void send_internal(internal_event_type type, int32_t x, int32_t y);

    friend listener;
};

}
