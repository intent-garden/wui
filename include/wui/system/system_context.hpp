//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#ifdef _WIN32
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

#endif

}
