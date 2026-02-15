//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/theme/theme.hpp>
#include <wui/theme/theme_impl.hpp>
#include <wui/theme/theme_selector.hpp>

namespace wui
{

static std::shared_ptr<i_theme> instance = nullptr;
static std::string dummy_string;
static std::vector<uint8_t> dummy_image;

/// Interface

#ifdef _WIN32
bool set_default_theme_from_resource(std::string_view name, int32_t resource_index, std::string_view resource_section)
{
    instance.reset();
    instance = std::make_shared<theme_impl>(name);
    instance->load_resource(resource_index, resource_section);

    return instance->get_error().is_ok();
}
#endif

bool set_default_theme_from_json(std::string_view name, std::string_view json)
{
    instance.reset();
    instance = std::make_shared<theme_impl>(name);
    instance->load_json(json);

    return instance->get_error().is_ok();
}

bool set_default_theme_from_file(std::string_view name, std::string_view file_name)
{
    instance.reset();
    instance = std::make_shared<theme_impl>(name);
    instance->load_file(file_name);

    return instance->get_error().is_ok();
}

void set_default_theme_empty(std::string_view name)
{
    instance.reset();
    instance = std::make_shared<theme_impl>(name);
}

bool set_default_theme_from_name(std::string_view name, error &err)
{
    auto theme_params = wui::get_app_theme(name);

#ifdef _WIN32
    bool ok = wui::set_default_theme_from_resource(name, theme_params.resource_id, "JSONS");
#else
    bool ok = wui::set_default_theme_from_file(name, theme_params.file_name);
#endif

    err = instance->get_error();
    return ok;
}

error get_theme_error()
{
    if (instance)
    {
        instance->get_error();
    }
    return {};
}

std::shared_ptr<i_theme> get_default_theme()
{
    return instance;    
}

std::shared_ptr<i_theme> make_custom_theme(std::string_view name)
{
    return std::make_shared<theme_impl>(name);
}

std::shared_ptr<i_theme> make_custom_theme(std::string_view name, std::string_view json)
{
    auto ct = std::make_shared<theme_impl>(name);
    ct->load_json(json);
    return ct;
}

color theme_color(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_)
{
    if (theme_)
    {
        return theme_->get_color(control, value);
    }
    else if (instance)
    {
        return instance->get_color(control, value);
    }
    return 0;
}

int32_t theme_dimension(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_)
{
    if (theme_)
    {
        return theme_->get_dimension(control, value);
    }
    else if (instance)
    {
        return instance->get_dimension(control, value);
    }
    return 0;
}

const std::string &theme_string(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_)
{
    if (theme_)
    {
        return theme_->get_string(control, value);
    }
    else if (instance)
    {
        return instance->get_string(control, value);
    }
    return dummy_string;
}

font theme_font(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_)
{
    if (theme_)
    {
        return theme_->get_font(control, value);
    }
    else if (instance)
    {
        return instance->get_font(control, value);
    }
    return font();
}

const std::vector<uint8_t> &theme_image(std::string_view name, std::shared_ptr<i_theme> theme_)
{
    if (theme_)
    {
        return theme_->get_image(name);
    }
    else if (instance)
    {
        return instance->get_image(name);
    }

    return dummy_image;
}

}
