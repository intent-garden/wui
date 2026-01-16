//
// Copyright (c) 2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/ud84/wui
//

#pragma once

#include <wui/window/window.hpp>

#include <wui/system/system_context.hpp>
#include <wui/common/error.hpp>

#include <thread>
#include <unordered_map>

#ifdef __linux__
#include <xcb/xcb.h>
#endif

namespace wui
{

#ifdef __linux__

struct wnd
{
    std::shared_ptr<window> window_;
    bool created = false;
};

class listener
{
public:
    listener();
    ~listener();

    void add_window(xcb_window_t id, std::shared_ptr<window> window);
    void delete_window(xcb_window_t id);

    bool init();
    
    system_context const &context() const;

    error const &get_error() const;

private:
    bool started;
    std::thread thread;
    system_context context_;

    std::unordered_map<xcb_window_t, wnd> windows;

    error err;

    void start();
    void stop();
    
    void process_events();
};

listener& get_listener(); /// Singleton

#else // Windows - stub implementation

class listener
{
public:
    listener() {}
    ~listener() {}
    void add_window(void*, std::shared_ptr<window>) {}
    void delete_window(void*) {}
    bool init() { return true; }
    system_context const &context() const { static system_context ctx{}; return ctx; }
    error const &get_error() const { static error e{}; return e; }
};

inline listener& get_listener() { static listener instance; return instance; }

#endif

}
