//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/graphic/graphic.hpp>
#include <wui/common/flag_helpers.hpp>
#include <wui/system/tools.hpp>

#include <boost/nowide/convert.hpp>

#include <unordered_map>

#ifdef __linux__
#include <xcb/xcb_image.h>

#include <cairo.h>
#include <cairo-xcb.h>

#include <algorithm>
#include <cmath>
#include <memory>

xcb_visualtype_t *default_visual_type(wui::system_context &context_)
{
    auto depth_iter = xcb_screen_allowed_depths_iterator(context_.screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter))
    {
        auto visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter))
        {
            if (context_.screen->root_visual == visual_iter.data->visual_id)
            {
                return visual_iter.data;
            }
        }
    }
    return nullptr;
}

#elif _WIN32

#include <gdiplus.h>

#endif

namespace wui
{

graphic::graphic(system_context &context__)
    : context_(context__),
      pc(context_),
      max_size_(),
      background_color(0)
#ifdef _WIN32
    , mem_dc(0),
      mem_bitmap(0),
#elif __linux__
    , mem_pixmap(0),
      surface(nullptr),
      device(nullptr),
#endif
    err{}
{
}

graphic::~graphic()
{
    release();
}

bool graphic::init(rect max_size__, color background_color_)
{
    max_size_ = max_size__;
    background_color = background_color_;

#ifdef _WIN32
    if (mem_dc)
    {
        err.type = error_type::already_started;
        err.component = "graphic::init()";
        return false;
    }

    err.reset();

    auto wnd_dc = GetDC(context_.hwnd);

    mem_dc = CreateCompatibleDC(wnd_dc);

    mem_bitmap = CreateCompatibleBitmap(wnd_dc, max_size_.width(), max_size_.height());
    if (!mem_bitmap)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::init()";
        err.message = "CreateCompatibleBitmap returns null";

        ReleaseDC(context_.hwnd, wnd_dc);

        return false;
    }

    SelectObject(mem_dc, mem_bitmap);

    SetMapMode(mem_dc, MM_TEXT);

    RECT filling_rect = { 0, 0, max_size_.width(), max_size_.height() };
    FillRect(mem_dc, &filling_rect, pc.get_brush(background_color));

    ReleaseDC(context_.hwnd, wnd_dc);
#elif __linux__
    if (!context_.display || mem_pixmap)
    {
        err.type = error_type::already_started;
        err.component = "graphic::init()";
        return false;
    }

    err.reset();

    mem_pixmap = xcb_generate_id(context_.connection);
    auto pixmap_create_cookie = xcb_create_pixmap(context_.connection,
        context_.screen->root_depth,
        mem_pixmap,
        context_.wnd,
        max_size_.width(),
        max_size_.height());
    if (!check_cookie(pixmap_create_cookie, context_.connection, err, "graphic::init() xcb_create_pixmap"))
    {
        err.type = error_type::no_handle;
        err.component = "graphic::init() xcb_create_pixmap";
        err.message = "Can't create the x11 pixmap";

        return false;
    }

    surface = cairo_xcb_surface_create(context_.connection, mem_pixmap, default_visual_type(context_), max_size_.width(), max_size_.height());
    if (!surface)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::init() cairo_xcb_surface_create";
        err.message = "Can't create the cairo surface";

        return false;
    }

    clear(max_size_);
#endif

    pc.init();

    return true;
}

void graphic::release()
{
    pc.release();
    
#ifdef _WIN32
    DeleteObject(mem_bitmap);
    mem_bitmap = 0;

    DeleteDC(mem_dc);
    mem_dc = 0;
#elif __linux__
    if (surface)
    {
        cairo_surface_destroy(surface);
        surface = nullptr;
    }

    if (context_.connection && mem_pixmap)
    {
        auto free_pixmap_cookie = xcb_free_pixmap(context_.connection, mem_pixmap);
        check_cookie(free_pixmap_cookie, context_.connection, err, "graphic::release()");
        mem_pixmap = 0;
    }
#endif
}

rect graphic::max_size() const
{
    return max_size_;
}

void graphic::set_background_color(color background_color_)
{
    background_color = background_color_;

    clear({ 0, 0, max_size_.width(), max_size_.height() });
}

void graphic::clear(rect position)
{
#ifdef _WIN32
    if (!mem_dc)
    {
        return;
    }

    RECT filling_rect = !position.is_null() ? 
        RECT{ position.left, position.top, position.right, position.bottom } :
        RECT{ 0, 0, max_size_.right, max_size_.bottom };
    FillRect(mem_dc, &filling_rect, pc.get_brush(background_color));
#elif __linux__
    if (!mem_pixmap || !surface)
    {
        return;
    }

    auto cr = cairo_create(surface);

    cairo_set_source_rgb(cr, static_cast<double>(wui::get_red(background_color)) / 255,
        static_cast<double>(wui::get_green(background_color)) / 255,
        static_cast<double>(wui::get_blue(background_color)) / 255);
    if (!position.is_null())
        cairo_rectangle(cr, position.left, position.top, position.width(), position.height());
    else
        cairo_rectangle(cr, 0, 0, max_size_.right, max_size_.bottom);
    cairo_fill(cr);

    cairo_destroy(cr);
#endif
}

void graphic::flush(rect updated_size)
{
#ifdef _WIN32
    auto wnd_dc = GetDC(context_.hwnd);

    if (wnd_dc)
    {
        BitBlt(wnd_dc,
            updated_size.left,
            updated_size.top,
            updated_size.width(),
            updated_size.height(),
            mem_dc,
            updated_size.left,
            updated_size.top,
            SRCCOPY);
    }

    ReleaseDC(context_.hwnd, wnd_dc);
#elif __linux__
    if (context_.wnd)
    {
        auto copy_area_cookie = xcb_copy_area(context_.connection,
            mem_pixmap,
            context_.wnd,
            pc.get_gc(background_color),
            updated_size.left,
            updated_size.top,
            updated_size.left,
            updated_size.top,
            updated_size.width(),
            updated_size.height());

        if (!check_cookie(copy_area_cookie, context_.connection, err, "graphic::end_drawing xcb_copy_area"))
        {
            return;
        }
    }
#endif
}

void graphic::draw_pixel(rect position, color color_)
{
#ifdef _WIN32
    SetPixel(mem_dc, position.left, position.top, color_);
#elif __linux__
    xcb_point_t points[] = { { static_cast<int16_t>(position.left), static_cast<int16_t>(position.top) } };
    xcb_poly_point(context_.connection, XCB_COORD_MODE_ORIGIN, mem_pixmap, pc.get_gc(color_), 1, points);
#endif
}

void graphic::draw_line(rect position, color color_, uint32_t width)
{
#ifdef _WIN32
    auto old_pen = (HPEN)SelectObject(mem_dc, pc.get_pen(PS_SOLID, width, color_));

    MoveToEx(mem_dc, position.left, position.top, (LPPOINT)NULL);
    LineTo(mem_dc, position.right, position.bottom);

    SelectObject(mem_dc, old_pen);
#elif __linux__
    xcb_point_t polyline[] = { { static_cast<int16_t>(position.left), static_cast<int16_t>(position.top) },
        { static_cast<int16_t>(position.right), static_cast<int16_t>(position.bottom) } };
    xcb_poly_line(context_.connection, XCB_COORD_MODE_ORIGIN, mem_pixmap, pc.get_gc(color_), 2, polyline);
#endif
}

rect graphic::measure_text(std::string_view text_, const font &font__)
{
    if (text_.empty())
    {
        return {0, 0, 0, font__.size};
    }
#ifdef _WIN32
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font__));

    RECT text_rect = { 0 };
    auto wide_str = boost::nowide::widen(text_);
    DrawTextW(mem_dc, wide_str.c_str(), static_cast<int32_t>(wide_str.size()), &text_rect, DT_CALCRECT);

    SelectObject(mem_dc, old_font);

    return {0, 0, text_rect.right, text_rect.bottom};
#elif __linux__
    if (!surface)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::measure_text()";
        err.message = "No cairo surface";
        return rect{ 0 };
    }

    auto cr = pc.get_font(font__, surface);
    if (!cr)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::measure_text()";
        err.message = "No cairo font context";
        return rect{ 0 };
    }

    cairo_text_extents_t dot_extents, extents;   // It's a workaround 'magic'
    cairo_text_extents(cr, ".", &dot_extents);   // to work the spaces
    std::string s; s.reserve(text_.size() + 2);  //
    s = '.' + std::string(text_) + '.';          // =)
    cairo_text_extents(cr, s.c_str(), &extents);

    return { 0, 0, 
        static_cast<int32_t>(ceil(extents.width - (dot_extents.width * 3))),
        static_cast<int32_t>(ceil(extents.height)) };
#endif
}

void graphic::draw_text(rect position, std::string_view text_, color color_, const font &font__)
{
#ifdef _WIN32
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font__));
    
    SetTextColor(mem_dc, color_);
    SetBkMode(mem_dc, TRANSPARENT);

    auto wide_str = boost::nowide::widen(text_);
    TextOutW(mem_dc, position.left, position.top, wide_str.c_str(), static_cast<int32_t>(wide_str.size()));

    SelectObject(mem_dc, old_font);
#elif __linux__
    if (!surface)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::draw_text()";
        err.message = "No cairo surface";
        return;
    }

    auto cr = pc.get_font(font__, surface);
    if (!cr)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::draw_text()";
        err.message = "No cairo font context";
        return;
    }

    cairo_set_source_rgb(cr,
        static_cast<double>(wui::get_red(color_)) / 255,
        static_cast<double>(wui::get_green(color_)) / 255,
        static_cast<double>(wui::get_blue(color_)) / 255);

    cairo_move_to(cr, position.left, (double)position.top + font__.size * 5 / 6);
    
    std::string text__(text_); /// Workaround to prevent crashes
    text__ += '\0';
    
    cairo_show_text(cr, text__.c_str());
#endif
}

void graphic::draw_rect(rect position, color fill_color)
{
#ifdef _WIN32
    RECT position_rect = { position.left, position.top, position.right, position.bottom };
    FillRect(mem_dc, &position_rect, pc.get_brush(fill_color));
#elif __linux__
    auto pos = position;
    if (pos.left > pos.right)
    {
        std::swap(pos.left, pos.right);
    }
    if (pos.top > pos.bottom)
    {
        std::swap(pos.top, pos.bottom);
    }

    if (!surface)
    {
        return;
    }

    auto cr = cairo_create(surface);

    cairo_set_source_rgb(cr, static_cast<double>(wui::get_red(fill_color)) / 255,
        static_cast<double>(wui::get_green(fill_color)) / 255,
        static_cast<double>(wui::get_blue(fill_color)) / 255);
    cairo_rectangle(cr, pos.left, pos.top, pos.width(), pos.height());
    cairo_fill(cr);

    cairo_destroy(cr);
#endif
}

#ifdef _WIN32
void DrawRoundBox(HDC dc,
    rect pos,
    int  radius_,
    int  borderWidth,
    COLORREF background,
    COLORREF border)
{
    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // Mkae the path
    Gdiplus::GraphicsPath path;
    int radius = radius_ > 0 ? radius_ : 0;

    if (radius)
    {
        path.AddArc(pos.left, pos.top, radius * 2, radius * 2, 180, 90);
        path.AddArc(pos.right - radius * 2, pos.top, radius * 2, radius * 2, 270, 90);
        path.AddArc(pos.right - radius * 2, pos.bottom - radius * 2, radius * 2, radius * 2, 0, 90);
        path.AddArc(pos.left, pos.bottom - radius * 2, radius * 2, radius * 2, 90, 90);
        path.CloseFigure();
    }
    else
    {
        Gdiplus::Point p[] = {
            { pos.left,  pos.top },
            { pos.right, pos.top },
            { pos.right, pos.bottom },
            { pos.left,  pos.bottom }
        };
        path.AddLines(p, 4);
        path.CloseFigure();
    }

    // Fill (only if the color is set)
    if (background != make_color(0, 0, 0, 255))
    {
        Gdiplus::Color fill(255,
            GetRValue(background),
            GetGValue(background),
            GetBValue(background));

        Gdiplus::SolidBrush br(fill);
        g.FillPath(&br, &path);
    }

    // Border outline
    if (borderWidth > 0 && border != make_color(0, 0, 0, 255))
    {
        Gdiplus::Pen pen(
            Gdiplus::Color(255,
                GetRValue(border),
                GetGValue(border),
                GetBValue(border)),
            1.0f * borderWidth);

        g.DrawPath(&pen, &path);
    }
}
#endif

void graphic::draw_rect(rect position, color border_color, color fill_color, uint32_t border_width, uint32_t rnd)
{
#ifdef _WIN32
    DrawRoundBox(mem_dc, position, rnd, border_width, fill_color, border_color);
#elif __linux__

    if (!surface)
    {
        return;
    }

    auto cr = cairo_create(surface);

    double l = position.left,
       t     = position.top,
       r     = position.right,
       b     = position.bottom,
       width  = position.width(),
       height = position.height();

    cairo_new_sub_path(cr);

    if (rnd == 0)
    {
        cairo_move_to(cr, l, t);
        cairo_line_to(cr, r, t);
        cairo_line_to(cr, r, b);
        cairo_line_to(cr, l, b);
        cairo_line_to(cr, l, t);
    }
    else
    {
        double radius = rnd;
        double degrees = M_PI / 180.0;
        
        cairo_arc (cr, l + width - radius, t + radius, radius, -90 * degrees, 0 * degrees);
        cairo_arc (cr, l + width - radius, t + height - radius, radius, 0 * degrees, 90 * degrees);
        cairo_arc (cr, l + radius, t + height - radius, radius, 90 * degrees, 180 * degrees);
        cairo_arc (cr, l + radius, t + radius, radius, 180 * degrees, 270 * degrees);
    }

    cairo_close_path(cr);

    if (get_alpha(fill_color) != 0)
    {
        cairo_set_source_rgb(cr, static_cast<double>(wui::get_red(fill_color)) / 255,
            static_cast<double>(wui::get_green(fill_color)) / 255,
            static_cast<double>(wui::get_blue(fill_color)) / 255);
        cairo_fill_preserve (cr);
    }

    cairo_set_source_rgb(cr, static_cast<double>(wui::get_red(border_color)) / 255,
        static_cast<double>(wui::get_green(border_color)) / 255,
        static_cast<double>(wui::get_blue(border_color)) / 255);
    cairo_set_line_width(cr, border_width);
    cairo_stroke(cr);

    cairo_destroy(cr);

#endif
}

void graphic::draw_buffer(rect position, uint8_t *buffer, int32_t left_shift, int32_t top_shift)
{
#ifdef _WIN32
    auto source_bitmap = pc.get_bitmap(position.width(), position.height(), buffer, mem_dc);
    auto source_dc = CreateCompatibleDC(mem_dc);
    SelectObject(source_dc, source_bitmap);

    BitBlt(mem_dc,
        position.left,
        position.top,
        position.width(),
        position.height(),
        source_dc,
        left_shift,
        top_shift,
        SRCCOPY);

    DeleteDC(source_dc);
#elif __linux__
    auto pixmap = xcb_generate_id(context_.connection);
    auto pixmap_cookie = xcb_create_pixmap(context_.connection,
        context_.screen->root_depth,
        pixmap,
        context_.wnd,
        position.width(), position.height());

    if (!check_cookie(pixmap_cookie, context_.connection, err, "graphic::draw_buffer() xcb_create_pixmap"))
    {
        return;
    }

    auto image = xcb_image_create_native(context_.connection,
           position.width(), position.height(),
           XCB_IMAGE_FORMAT_Z_PIXMAP,
           context_.screen->root_depth,
           nullptr,
           position.width() * position.height() * 4,
           nullptr);

    if (!image)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::draw_buffer()";
        err.message = "xcb_image_create_native error";

        return;
    }

    image->data = buffer;

    xcb_image_put(context_.connection, pixmap, pc.get_gc(background_color), image, 0, 0, 0);

    xcb_image_destroy(image);

    auto copy_area_cookie = xcb_copy_area(context_.connection,
        pixmap,
        mem_pixmap,
        pc.get_gc(background_color),
        left_shift,
        top_shift,
        position.left,
        position.top,
        position.right,
        position.bottom);

    xcb_free_pixmap(context_.connection, pixmap);

    if (!check_cookie(copy_area_cookie, context_.connection, err, "graphic::draw_buffer() xcb_copy_area"))
    {
        return;
    }
#endif
}

void graphic::draw_graphic(rect position, graphic &graphic_, int32_t left_shift, int32_t top_shift)
{
#ifdef _WIN32
    if (graphic_.drawable())
    {
        BitBlt(mem_dc,
            position.left,
            position.top,
            position.right,
            position.bottom,
            graphic_.drawable(),
            left_shift,
            top_shift,
            SRCCOPY);
    }
#elif __linux__
    if (graphic_.drawable())
    {
        auto copy_area_cookie = xcb_copy_area(context_.connection,
            graphic_.drawable(),
            mem_pixmap,
            pc.get_gc(background_color),
            left_shift,
            top_shift,
            position.left,
            position.top,
            position.right,
            position.bottom);

        if (!check_cookie(copy_area_cookie, context_.connection, err, "graphic::draw_graphic() xcb_copy_area"))
        {
            return;
        }
    }
#endif
}

#ifdef _WIN32
HDC graphic::drawable()
{
    return mem_dc;
}
#elif __linux__
xcb_drawable_t graphic::drawable()
{
    return mem_pixmap;
}

/// workarounds
void graphic::draw_surface(cairo_surface_t &surface_, rect position__)
{
    if (!surface)
    {
        return;
    }
    
    auto cr = cairo_create(surface);

    auto surface_width = cairo_image_surface_get_width(&surface_);
    auto surface_height = cairo_image_surface_get_height(&surface_);

    double x_scale_factor = 1.0, y_scale_factor = 1.0;

    if (surface_width != 0 && surface_height != 0)
    {
        x_scale_factor = static_cast<double>(position__.width()) / surface_width;
        y_scale_factor = static_cast<double>(position__.height()) / surface_height;

        cairo_scale(cr, x_scale_factor, y_scale_factor);
    }

    cairo_set_source_surface(cr,
        &surface_,
        position__.left / x_scale_factor,
        position__.top / y_scale_factor);

    cairo_paint(cr);

    cairo_destroy(cr);
}

#endif

error graphic::get_error() const
{
    return err;
}

/// Text measurer //////////////////////////

static graphic* tm_graphic;
static std::unordered_map<std::string, std::pair<int32_t, int32_t>> tm_cache;

void init_text_measurer(graphic &gr)
{
    tm_graphic = &gr;
}

size_t font_hash(const font &font_)
{
    std::hash<std::string> hasher;

    return hasher(font_.name + std::to_string(font_.size));
}

rect measure_text(std::string_view text, const font &font_, graphic *gr)
{
    if (text.empty()) return { 0, 0, 0, font_.size };

    std::hash<std::string> hasher;

    // Compute the hash value
    auto stringHash = hasher(std::string(text));
    auto fontHash = font_hash(font_);
    auto hash = std::to_string(stringHash) + "_" + std::to_string(fontHash);

    // Find in cache if finded - return
    auto it = tm_cache.find(hash);
    if (it != tm_cache.end())
    {
        return { 0, 0, it->second.first, it->second.second };
    }

    if (!gr && !tm_graphic)
    {
        return { 0, 0, 0, font_.size };
    }

    // Measure and cache
    auto pos = gr ? gr->measure_text(text, font_) : tm_graphic->measure_text(text, font_);
    tm_cache[hash] = { pos.width(), pos.height() };
    return pos;
}

}
