//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/config/config.hpp>
#include <wui/config/config_impl_ini.hpp>
#include <wui/config/config_impl_reg.hpp>

namespace wui
{

namespace config
{

static std::shared_ptr<i_config> instance = nullptr;

/// Interface

bool use_ini_file(std::string_view file_name)
{
    instance.reset();
    instance = std::make_shared<config_impl_ini>(file_name);

    return instance->get_error().is_ok();
}
#ifdef _WIN32
bool use_registry(std::string_view app_key, HKEY root)
{
    instance.reset();
    instance = std::make_shared<config_impl_reg>(app_key, root);

    return instance->get_error().is_ok();
}
#endif

bool create_config(std::string_view file_name, std::string_view app_key, int64_t root)
{
    (void)file_name; // Unused on Windows, used on other platforms
#ifdef _WIN32
    return use_registry(app_key, root == 0 ? HKEY_CURRENT_USER : (HKEY)root);
#else
    return use_ini_file(file_name);
#endif
}

error get_error()
{
    if (instance)
    {
        return instance->get_error();
    }
    return {};
}

int32_t get_int(std::string_view section, std::string_view entry, int32_t default_)
{
    if (instance)
    {
        return instance->get_int(section, entry, default_);
    }
    return -1;
}

void set_int(std::string_view section, std::string_view entry, int32_t value)
{
    if (instance)
    {
        instance->set_int(section, entry, value);
    }
}

int64_t get_int64(std::string_view section, std::string_view entry, int64_t default_)
{
    if (instance)
    {
        return instance->get_int64(section, entry, default_);
    }
    return -1;
}

void set_int64(std::string_view section, std::string_view entry, int64_t value)
{
    if (instance)
    {
        instance->set_int64(section, entry, value);
    }
}

std::string get_string(std::string_view section, std::string_view entry, std::string_view default_)
{
    if (instance)
    {
        return instance->get_string(section, entry, default_);
    }
    return "";
}

void set_string(std::string_view section, std::string_view entry, std::string_view value)
{
    if (instance)
    {
        instance->set_string(section, entry, value);
    }
}

void delete_value(std::string_view section, std::string_view entry)
{
    if (instance)
    {
        instance->delete_value(section, entry);
    }
}

void delete_key(std::string_view section)
{
    if (instance)
    {
        instance->delete_key(section);
    }
}

}

}
