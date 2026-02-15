//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/system/system_context.hpp>

#include <wui/common/color.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>

#include <wui/graphic/primitive_container.hpp>

#include <string_view>
#include <cstdint>

#ifdef __linux__
struct _cairo_surface;
struct _cairo_device;
#endif

namespace wui
{

class graphic
{
public:
    graphic(system_context &context);
    ~graphic();

    bool init(rect max_size, color background_color);
    void release();

    rect max_size() const;

    void set_background_color(color background_color);

    void clear(rect position = { 0 });

    void flush(rect updated_size);

    void draw_pixel(rect position, color color_);

    void draw_line(rect position, color color_, uint32_t width = 1);

    rect measure_text(std::string_view text_, const font &font__);
    void draw_text(rect position, std::string_view text, color color_, const font &font_);

    void draw_rect(rect position, color fill_color);
    void draw_rect(rect position, color border_color, color fill_color, uint32_t border_width, uint32_t round);

    /// draw some buffer on context
    void draw_buffer(rect position, uint8_t *buffer, int32_t left_shift, int32_t top_shift);

    /// draw another graphic on context
    void draw_graphic(rect position, graphic &graphic_, int32_t left_shift, int32_t top_shift);

#ifdef _WIN32
    HDC drawable();
#elif __linux__
    xcb_drawable_t drawable();

    /// workaround on linux
    void draw_surface(_cairo_surface &surface, rect position);
#endif

    error get_error() const;

private:
    system_context &context_;

    primitive_container pc;

    rect max_size_;

    color background_color;

#ifdef _WIN32
    HDC mem_dc;
    HBITMAP mem_bitmap;
#elif __linux__
    xcb_pixmap_t mem_pixmap;

    _cairo_surface *surface;
    _cairo_device *device;
#endif

    error err;
};

void init_text_measurer(graphic &gr);
rect measure_text(std::string_view text, const font &font_, graphic *gr = nullptr);

}
