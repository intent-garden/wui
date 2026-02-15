//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/image.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>
#include <wui/system/path_tools.hpp>

#include <boost/nowide/convert.hpp>

#include <cstring>

#ifdef _WIN32

void load_image_from_data(const std::vector<uint8_t> &data, Gdiplus::Image **img)
{
    HGLOBAL h_buffer = ::GlobalAlloc(GMEM_MOVEABLE, data.size());
    if (h_buffer)
    {
        void* p_buffer = ::GlobalLock(h_buffer);
        if (p_buffer)
        {
            CopyMemory(p_buffer, data.data(), data.size());

            IStream* p_stream = NULL;
            if (::CreateStreamOnHGlobal(h_buffer, FALSE, &p_stream) == S_OK)
            {
                *img = Gdiplus::Image::FromStream(p_stream);
                p_stream->Release();
            }

            ::GlobalUnlock(p_buffer);
        }
        ::GlobalFree(h_buffer);
    }
}

void load_image_from_resource(WORD image_id, const std::wstring &resource_section, Gdiplus::Image **img)
{
    HINSTANCE h_inst = GetModuleHandle(NULL);
    HRSRC h_resource = FindResourceW(h_inst, (LPCWSTR)MAKEINTRESOURCE(image_id), resource_section.c_str());
    if (!h_resource)
    {
        return;
    }

    DWORD image_size = ::SizeofResource(h_inst, h_resource);
    if (!image_size)
    {
        return;
    }

    const void* resource_data = ::LockResource(::LoadResource(h_inst, h_resource));
    if (!resource_data)
    {
        return;
    }

    load_image_from_data(std::vector<uint8_t>(static_cast<const uint8_t*>(resource_data), static_cast<const uint8_t*>(resource_data) + image_size), img);
}

void load_image_from_file(std::string_view file_name, std::string_view images_path, Gdiplus::Image **img, wui::error &err)
{
    (void)err; // TODO: check error status
    *img = Gdiplus::Image::FromFile(std::wstring(boost::nowide::widen(images_path) + L"\\" + boost::nowide::widen(file_name)).c_str());
}

void free_image(Gdiplus::Image **img)
{
    if (*img)
    {
        delete *img;
        *img = nullptr;
    }
}

#elif __linux__

#include <boost/nowide/fstream.hpp>

void load_image_from_data(const std::vector<uint8_t> &data_, cairo_surface_t **img)
{
    struct png_reader_data
    {
        const uint8_t *data;
        uint32_t size_left;
    };

    auto read_png_data = [](void *closure,
        uint8_t *data,
        uint32_t length) noexcept -> cairo_status_t
    {
        auto &reader_data = *reinterpret_cast<png_reader_data *>(closure);
        if (reader_data.size_left < length)
        {
            return CAIRO_STATUS_READ_ERROR;
        }

        memcpy(data, reader_data.data, length);
        reader_data.data += length;
        reader_data.size_left -= length;

        return CAIRO_STATUS_SUCCESS;
    };

    png_reader_data reader_data = { data_.data(), static_cast<uint32_t>(data_.size()) };
    *img = cairo_image_surface_create_from_png_stream(+read_png_data, &reader_data);
}

void load_image_from_file(std::string_view file_name, std::string_view images_path, cairo_surface_t **img, wui::error &err)
{
    auto full_image_path = wui::real_path(std::string(images_path) + "/" + std::string(file_name));

    boost::nowide::ifstream f(full_image_path);
    if (!f)
    {
        err.type = wui::error_type::file_not_found;
        err.component = "image::load_image_from_file()";
        err.message = "unable to open image file: " + full_image_path + " errno: " + std::to_string(errno);
        return;
    }
    f.close();

    *img = cairo_image_surface_create_from_png(full_image_path.c_str());
}

void free_image(cairo_surface_t **img)
{
    if (*img)
    {
        cairo_surface_destroy(*img);
        *img = nullptr;
    }
}

#endif

namespace wui
{

#ifdef _WIN32
image::image(int32_t resource_index_, std::shared_ptr<i_theme> theme__)
    : theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), topmost_(false),
    file_name(),
    resource_index(resource_index_),
    img(nullptr),
    err{}
{
    load_image_from_resource(static_cast<WORD>(resource_index), boost::nowide::widen(theme_string(tc, tv_resource, theme_)), &img);
}
#endif

image::image(std::string_view file_name_, std::shared_ptr<i_theme> theme__)
    : theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), topmost_(false),
    file_name(file_name_),
#ifdef _WIN32
    resource_index(0),
#endif
    img(nullptr),
    err{}
{
    load_image_from_file(file_name_, theme_string(tc, tv_path, theme_), &img, err);
}

image::image(const std::vector<uint8_t> &data)
    : theme_(),
    position_{ 0 },
    parent_(),
    showed_(true), topmost_(false),
    file_name(),
#ifdef _WIN32
    resource_index(0),
#endif
    img(nullptr),
    err{}
{
    load_image_from_data(data, &img);
}

image::~image()
{
    free_image(&img);

    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void image::draw(graphic &gr_, rect)
{
    auto control_pos = position();

    if (!showed_ || control_pos.is_null())
    {
        return;
    }

#ifdef _WIN32
    if (img)
    {
        Gdiplus::Graphics gr(gr_.drawable());

        gr.DrawImage(
            img,
            Gdiplus::Rect(control_pos.left, control_pos.top, control_pos.width(), control_pos.height()),
            0, 0, img->GetWidth(), img->GetHeight(),
            Gdiplus::UnitPixel,
            nullptr);
    }
#elif __linux__
    if (img)
    {
        gr_.draw_surface(*img, control_pos);
    }
#endif
}

void image::set_position(rect position__)
{
    position_ = position__;
}

rect image::position() const
{
    return get_control_position(position_, parent_);
}

void image::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;
}

std::weak_ptr<window> image::parent() const
{
    return parent_;
}

void image::clear_parent()
{
    parent_.reset();
}

void image::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool image::topmost() const
{
    return topmost_;
}

bool image::focused() const
{
    return false;
}

bool image::focusing() const
{
    return false;
}

error image::get_error() const
{
    return err;
}

void image::update_theme_control_name(std::string_view )
{
}

void image::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

#ifdef _WIN32
    if (resource_index)
    {
        change_image(resource_index);
    }
    else
#endif
    if (!file_name.empty())
    {
        change_image(file_name);
    }
}

void image::show()
{
    showed_ = true;
    redraw();
}

void image::hide()
{
    showed_ = false;
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->redraw(position(), true);
    }
}

bool image::showed() const
{
    return showed_;
}

void image::enable()
{
}

void image::disable()
{
}

bool image::enabled() const
{
    return true;
}

#ifdef _WIN32
void image::change_image(int32_t resource_index_)
{
    resource_index = resource_index_;

    free_image(&img);
    load_image_from_resource(static_cast<WORD>(resource_index), boost::nowide::widen(theme_string(tc, tv_resource, theme_)), &img);
    
    redraw();
}
#endif

void image::change_image(std::string_view file_name_)
{
    file_name = file_name_;

    free_image(&img);
    load_image_from_file(file_name, theme_string(tc, tv_path, theme_), &img, err);

    redraw();
}

void image::change_image(const std::vector<uint8_t> &data)
{
    free_image(&img);
    load_image_from_data(data, &img);

    redraw();
}

int32_t image::width() const
{
    if (img)
    {
#ifdef _WIN32
        return img->GetWidth();
#elif __linux__
        return cairo_image_surface_get_width(img);
#endif
    }
    return 0;
}

int32_t image::height() const
{
    if (img)
    {
#ifdef _WIN32
        return img->GetHeight();
#elif __linux__
        return cairo_image_surface_get_height(img);
#endif
    }
    return 0;
}

void image::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            parent__->redraw(position());
        }
    }
}

}
