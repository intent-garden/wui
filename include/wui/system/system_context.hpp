//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif __linux__
#include <X11/Xlib-xcb.h>
#endif

namespace wui
{

class graphic;

#ifdef _WIN32

struct system_context
{
    HWND hwnd;

    bool valid() const
    {
        return hwnd != 0;
    }
};

#elif __linux__

struct system_context
{
    Display          *display;

    xcb_connection_t *connection;
    xcb_screen_t     *screen;
    xcb_window_t     wnd;

    bool valid() const
    {
        return display != nullptr;
    }
};

#elif __EMSCRIPTEN__

struct system_context
{
    int canvas = 0;
    double scale = 1.0;
    bool valid() const { return canvas != 0; }
};

#elif __APPLE__

// Opaque AppKit handles keep public headers usable from ordinary C++.
struct system_context
{
    void *native_window = nullptr;
    void *native_view = nullptr;
    double scale = 1.0;

    bool valid() const { return native_window != nullptr; }
};

#endif

}
