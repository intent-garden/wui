//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/locale/locale.hpp>
#include <wui/locale/locale_impl.hpp>
#include <wui/locale/locale_selector.hpp>

namespace wui
{

static std::shared_ptr<i_locale> instance = nullptr;
static std::string dummy_string;
static std::vector<uint8_t> dummy_image;

/// Interface

#ifdef _WIN32
bool set_locale_from_resource(locale_type type, std::string_view name, int32_t resource_index, std::string_view resource_section)
{
    instance.reset();
    instance = std::make_shared<locale_impl>(type, name);
    instance->load_resource(resource_index, resource_section);

    return instance->get_error().is_ok();
}
#endif

bool set_locale_from_json(locale_type type, std::string_view name, std::string_view json)
{
    instance.reset();
    instance = std::make_shared<locale_impl>(type, name);
    instance->load_json(json);

    return instance->get_error().is_ok();
}

bool set_locale_from_file(locale_type type, std::string_view name, std::string_view file_name)
{
    instance.reset();
    instance = std::make_shared<locale_impl>(type, name);
    instance->load_file(file_name);

    return instance->get_error().is_ok();
}

void set_locale_empty(locale_type type, std::string_view name)
{
    instance.reset();
    instance = std::make_shared<locale_impl>(type, name);
}

bool set_locale_from_type(locale_type type, error &err)
{
    auto locale_params = wui::get_app_locale(type);

#ifdef _WIN32
    bool ok = wui::set_locale_from_resource(locale_params.type, locale_params.name, locale_params.resource_id, "JSONS");
#else
    bool ok = wui::set_locale_from_file(locale_params.type, locale_params.name, locale_params.file_name);
#endif

    err = instance->get_error();
    return ok;
}

error get_locale_error()
{
    if (instance)
    {
        instance->get_error();
    }
    return {};
}

std::shared_ptr<i_locale> get_locale()
{
    return instance;    
}

void set_locale_value(std::string_view section, std::string_view value, std::string_view str)
{
    if (instance)
    {
        instance->set(section, value, str);
    }
}

const std::string &locale(std::string_view section, std::string_view value)
{
    if (instance)
    {
        return instance->get(section, value);
    }
    return dummy_string;
}

}
