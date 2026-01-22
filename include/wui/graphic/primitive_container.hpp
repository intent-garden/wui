//
// Copyright (c) 2021-2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/common/color.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>
#include <wui/system/system_context.hpp>

#include <map>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
struct _cairo;
struct _cairo_surface;
#endif

namespace wui
{

class primitive_container
{
public:
    primitive_container(wui::system_context &context_);
    ~primitive_container();

    void init();
    void release();

    wui::error get_error() const;

#ifdef _WIN32
    HPEN get_pen(int32_t style, int32_t width, color color_);
    HBRUSH get_brush(color color_);
    HFONT get_font(font font_);
    HBITMAP get_bitmap(int32_t width, int32_t height, uint8_t *buffer, HDC hdc);
#elif __linux__
    xcb_gcontext_t get_gc(color color_);
    _cairo *get_font(font font_, _cairo_surface *surface);
#endif

private:
    wui::system_context &context_;

    wui::error err;

#ifdef _WIN32
    std::map<std::pair<std::pair<int32_t, int32_t>, color>, HPEN> pens;
    std::map<int32_t, HBRUSH> brushes;
    std::map<std::pair<std::pair<std::string, int32_t>, decorations>, HFONT> fonts;
    std::map<std::pair<int32_t, int32_t>, HBITMAP> bitmaps;
#elif __linux__
    std::map<color, xcb_gcontext_t> gcs;
    std::map<std::pair<std::pair<std::string, int32_t>, decorations>, _cairo*> fonts;
#endif
};

}
