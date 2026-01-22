//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/common/color.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace wui
{

class i_theme
{
public:
    virtual std::string get_name() const = 0;

    virtual void set_color(std::string_view control, std::string_view value, color color_) = 0;
    virtual color get_color(std::string_view control, std::string_view value) const = 0;

    virtual void set_dimension(std::string_view control, std::string_view value, int32_t dimension) = 0;
    virtual int32_t get_dimension(std::string_view control, std::string_view value) const = 0;

    virtual void set_string(std::string_view control, std::string_view value, std::string_view str) = 0;
    virtual const std::string &get_string(std::string_view control, std::string_view value) const = 0;

    virtual void set_font(std::string_view control, std::string_view value, const font &font_) = 0;
    virtual font get_font(std::string_view control, std::string_view value) const = 0;

    virtual void set_image(std::string_view name, const std::vector<uint8_t> &data) = 0;
    virtual const std::vector<uint8_t> &get_image(std::string_view name) = 0;

#ifdef _WIN32
    virtual void load_resource(int32_t resource_index, std::string_view resource_section) = 0;
#endif
    virtual void load_json(std::string_view json) = 0;
    virtual void load_file(std::string_view file_name) = 0;
    virtual void load_theme(const i_theme &theme_) = 0;

    virtual error get_error() const = 0;

    virtual ~i_theme() {}
};

}
