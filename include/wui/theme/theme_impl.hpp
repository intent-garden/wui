//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/theme/i_theme.hpp>

#include <map>

namespace wui
{

class theme_impl : public i_theme
{
public:
    theme_impl(std::string_view name);

    virtual std::string get_name() const;

    virtual void set_color(std::string_view control, std::string_view value, color color_);
    virtual color get_color(std::string_view control, std::string_view value) const;

    virtual void set_dimension(std::string_view control, std::string_view value, int32_t dimension);
    virtual int32_t get_dimension(std::string_view control, std::string_view value) const;

    virtual void set_string(std::string_view control, std::string_view value, std::string_view str);
    virtual const std::string &get_string(std::string_view control, std::string_view value) const;

    virtual void set_font(std::string_view control, std::string_view value, const font &font_);
    virtual font get_font(std::string_view control, std::string_view value) const;

    virtual void set_image(std::string_view name, const std::vector<uint8_t> &data);
    virtual const std::vector<uint8_t> &get_image(std::string_view name);

#ifdef _WIN32
    virtual void load_resource(int32_t resource_index, std::string_view resource_section);
#endif
    virtual void load_json(std::string_view json);
    virtual void load_file(std::string_view file_name);
    virtual void load_theme(const i_theme &theme_);

    virtual error get_error() const;

private:
    std::string name;

    std::map<std::pair<std::string, std::string>, int32_t> ints;
    std::map<std::pair<std::string, std::string>, std::string> strings;
    std::map<std::pair<std::string, std::string>, font> fonts;
    std::map<std::string, std::vector<uint8_t>> imgs;

    std::string dummy_string;
    std::vector<uint8_t> dummy_image;

    error err;
};

}
