//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/locale/locale_impl.hpp>
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

locale_impl::locale_impl(locale_type type_, std::string_view name_)
    : type(type_), name(name_), strings(), dummy_string(), err{}
{
}

locale_type locale_impl::get_type() const
{
    return type;
}

std::string locale_impl::get_name() const
{
    return name;
}

void locale_impl::set(std::string_view control, std::string_view value, std::string_view str)
{
    strings[{ control.data(), value.data() }] = str;
}

const std::string &locale_impl::get(std::string_view control, std::string_view value) const
{
    auto it = strings.find({ control.data(), value.data() });
    if (it != strings.end())
    {
        return it->second;
    }
    return dummy_string;
}

#ifdef _WIN32
void locale_impl::load_resource(int32_t resource_index, std::string_view resource_section)
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

void locale_impl::load_json(std::string_view json_)
{
    err.reset();

    try
    {
        auto j = nlohmann::json::parse(json_);

        if (j.is_discarded())
        {
            err.type = error_type::invalid_json;
            err.component = "locale_impl::load_json()";
            err.message = "json parse discarded";

            return;
        }

        auto sections = j.at("sections");
        for (auto &s : sections)
        {
            std::string section = s.at("type").get<std::string>();

            auto obj = s.get<nlohmann::json::object_t>();
            for (auto& kvp : obj)
            {
                if (kvp.first == "type")
                {
                    continue;
                }

                if (kvp.second.is_string())
                {
                    auto str = kvp.second.get<std::string>();
                    strings[{ section, kvp.first }] = str;
                }
            }
        }
    }
    catch (nlohmann::detail::exception &e)
    {
        err.type = error_type::invalid_json;
        err.component = "locale_impl::load_json()";
        err.message = "Error reading locale json: " + std::string(e.what());
    }
}

void locale_impl::load_file(std::string_view file_name)
{
    err.reset();

    std::ifstream f(wui::real_path(file_name));
    if (!f)
    {
        err.type = error_type::file_not_found;
        err.component = "locale_impl::load_file()";
        err.message = "Unable to open locale file: " + wui::real_path(file_name) + " errno: " + std::to_string(errno);

        return;
    }
    
    std::stringstream buffer;
    buffer << f.rdbuf();

    load_json(buffer.str());
}

void locale_impl::load_locale(const i_locale &locale_)
{
    strings = static_cast<const locale_impl*>(&locale_)->strings;
}

error locale_impl::get_error() const
{
    return err;
}

}
