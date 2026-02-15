//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/theme/theme_impl.hpp>
#include <wui/system/tools.hpp>
#include <wui/system/path_tools.hpp>

#include <nlohmann/json.hpp>
#include <boost/nowide/convert.hpp>

#include <sstream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace wui
{

theme_impl::theme_impl(std::string_view name_)
    : name(name_), ints(), strings(), fonts(), imgs(), dummy_string(), dummy_image(), err{}
{
}

std::string theme_impl::get_name() const
{
    return name;
}

void theme_impl::set_color(std::string_view control, std::string_view value, color color_)
{
    ints[{ control.data(), value.data() }] = color_;
}

color theme_impl::get_color(std::string_view control, std::string_view value) const
{
    auto it = ints.find({ control.data(), value.data() });
    if (it != ints.end())
    {
        return static_cast<color>(it->second);
    }
    return 0;
}

void theme_impl::set_dimension(std::string_view control, std::string_view value, int32_t dimension)
{
    ints[{ control.data(), value.data() }] = dimension;
}

int32_t theme_impl::get_dimension(std::string_view control, std::string_view value) const
{
    auto it = ints.find({ control.data(), value.data() });
    if (it != ints.end())
    {
        return it->second;
    }
    return 0;
}

void theme_impl::set_string(std::string_view control, std::string_view value, std::string_view str)
{
    strings[{ control.data(), value.data() }] = str;
}

const std::string &theme_impl::get_string(std::string_view control, std::string_view value) const
{
    auto it = strings.find({ control.data(), value.data() });
    if (it != strings.end())
    {
        return it->second;
    }
    return dummy_string;
}

void theme_impl::set_font(std::string_view control, std::string_view value, const font &font_)
{
    fonts[{ control.data(), value.data() }] = font_;
}

font theme_impl::get_font(std::string_view control, std::string_view value) const
{
    auto it = fonts.find({ control.data(), value.data() });
    if (it != fonts.end())
    {
        return it->second;
    }
    return font();
}

void theme_impl::set_image(std::string_view name_, const std::vector<uint8_t> &data)
{
    imgs[name_.data()] = data;
}

const std::vector<uint8_t> &theme_impl::get_image(std::string_view name_)
{
    auto it = imgs.find(name_.data());
    if (it != imgs.end())
    {
        return it->second;
    }
    return dummy_image;
}

#ifdef _WIN32
void theme_impl::load_resource(int32_t resource_index, std::string_view resource_section)
{
    auto h_inst = GetModuleHandle(NULL);
    auto h_resource = FindResourceW(h_inst, (LPCWSTR)MAKEINTRESOURCE(resource_index), boost::nowide::widen(resource_section).c_str());
    if (!h_resource)
    {
        return;
    }

    auto resource_size = ::SizeofResource(h_inst, h_resource);
    if (!resource_size)
    {
        return;
    }

    const void* resource_data = ::LockResource(::LoadResource(h_inst, h_resource));
    if (!resource_data)
    {
        return;
    }

    load_json(std::string(static_cast<const char*>(resource_data), resource_size));
}
#endif

void theme_impl::load_json(std::string_view json_)
{
    err.reset();

    try
    {
        auto j = nlohmann::json::parse(json_);

        if (j.is_discarded())
        {
            err.type = error_type::invalid_json;
            err.component = "theme_impl::load_json()";
            err.message = "json parse discarded";

            return;
        }

        auto controls = j.at("controls");
        for (auto &c : controls)
        {
            std::string control = c.at("type").get<std::string>();

            auto obj = c.get<nlohmann::json::object_t>();
            for (auto& kvp : obj)
            {
                if (kvp.first == "type")
                {
                    continue;
                }

                if (kvp.second.is_string())
                {
                    auto str = kvp.second.get<std::string>();
                    if (str[0] == '#')
                    {
                        try
                        {
                            str.erase(0, 1);
                            str.insert(0, "0x");
                            int32_t color = std::stol(str, nullptr, 16);

                            ints[{ control, kvp.first }] = make_color(get_red(color), get_green(color), get_blue(color));
                        }
                        catch (...)
                        {
                            err.type = error_type::invalid_value;
                            err.component = "theme_impl::load_json()";
                            err.message = "Error reading color in control: " + control + ", key: " + kvp.first + ", value: " + str;
                        }
                    }
                    else
                    {
                        strings[{ control, kvp.first }] = str;
                    }
                }
                else if (kvp.second.is_number_integer())
                {
                    ints[{ control, kvp.first }] = kvp.second.get<int32_t>();
                }
                else if (kvp.second.is_object() && kvp.first.find("font") != std::string::npos)
                {
                    auto fnt = kvp.second.get<nlohmann::json::object_t>();
                    
                    int32_t decorations_ = static_cast<int32_t>(decorations::normal);
                    auto decorations_it = fnt.find("decorations");
                    if (decorations_it != fnt.end())
                    {
                        if (decorations_it->second.get<std::string>().find("bold") != std::string::npos)
                            decorations_ |= static_cast<int32_t>(decorations::bold);
                        if (decorations_it->second.get<std::string>().find("italic") != std::string::npos)
                            decorations_ |= static_cast<int32_t>(decorations::italic);
                        if (decorations_it->second.get<std::string>().find("underline") != std::string::npos)
                            decorations_ |= static_cast<int32_t>(decorations::underline);
                        if (decorations_it->second.get<std::string>().find("strike") != std::string::npos)
                            decorations_ |= static_cast<int32_t>(decorations::strike_out);
                    }

                    std::string font_name = "Segoe UI";
                    auto name_it = fnt.find("name");
                    if (name_it != fnt.end())
                    {
                        font_name = name_it->second.get<std::string>();
                    }

                    int32_t size = 18;
                    auto size_it = fnt.find("size");
                    if (size_it != fnt.end())
                    {
                        size = size_it->second.get<std::int32_t>();
                    }

                    fonts[{ control, kvp.first }] = font{ font_name, size, static_cast<decorations>(decorations_) };
                }
            }
        }

        auto images = j.at("images");
        for (auto &i : images)
        {
            std::string image_name;
            std::vector<uint8_t> image_data;

            auto obj = i.get<nlohmann::json::object_t>();
            for (auto& kvp : obj)
            {
                image_name = kvp.first;

                std::string byte_val;
                auto s = kvp.second.get<std::string>();
                for (auto &c : s)
                {
                    if (c == ' ' || c == ',')
                    {
                        continue;
                    }

                    if (byte_val.size() != 3)
                    {
                        byte_val += c;
                    }
                    else
                    {
                        byte_val += c;

                        char *p;
                        auto n = strtoul(byte_val.c_str(), &p, 16);
                        if (*p == 0)
                        {
                            image_data.emplace_back(static_cast<uint8_t>(n));
                        }
                        else
                        {
                            err.type = error_type::invalid_value;
                            err.component = "theme_impl::load_json()";
                            err.message = "Error reading theme image json: " + image_name + ", value: " + byte_val;
                        }

                        byte_val.clear();
                    }
                }
            }
            set_image(image_name, image_data);
        }
    }
    catch (nlohmann::detail::exception &e)
    {
        err.type = error_type::invalid_json;
        err.component = "theme_impl::load_json()";
        err.message = "Error reading theme json: " + std::string(e.what());
    }
}

void theme_impl::load_file(std::string_view file_name)
{
    err.reset();

    std::ifstream f(wui::real_path(file_name));

    if (!f)
    {
        err.type = error_type::file_not_found;
        err.component = "theme_impl::load_file()";
        err.message = "Unable to open theme file: " + wui::real_path(file_name) + " errno: " + std::to_string(errno);

        return;
    }
    
    std::stringstream buffer;
    buffer << f.rdbuf();

    load_json(buffer.str());
}

void theme_impl::load_theme(const i_theme &theme_)
{
    ints = static_cast<const theme_impl*>(&theme_)->ints;
    strings = static_cast<const theme_impl*>(&theme_)->strings;
    fonts = static_cast<const theme_impl*>(&theme_)->fonts;
}

error theme_impl::get_error() const
{
    return err;
}

}
