//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/theme/theme_selector.hpp>

#include <algorithm>

namespace wui
{

static themes_t instance;
static std::string default_theme = "dark";
static size_t theme_pos = 0;

void set_app_themes(const themes_t &tt)
{
    instance = tt;
}

void set_default_theme(std::string_view name)
{
    default_theme = name;
}

theme_params get_app_theme(std::string_view name)
{
    auto l = std::find(instance.begin(), instance.end(), name);
    if (l != instance.end())
    {
        return *l;
    }
    
    /// We don't find the theme by name, try to return default theme
    l = std::find(instance.begin(), instance.end(), default_theme);
    if (l != instance.end())
    {
        return *l;
    }
    return {};
}

void set_current_app_theme(std::string_view name)
{
    auto it = std::find(instance.begin(), instance.end(), name);
    if (it != instance.end())
    {
        theme_pos = it - instance.begin();
    }
}

std::string get_next_app_theme()
{
    if (instance.empty())
    {
        return "";
    }

    ++theme_pos;
    if (theme_pos + 1 > instance.size())
    {
        theme_pos = 0;
    }

    return instance[theme_pos].name;
}

const themes_t &get_app_themes()
{
    return instance;
}

}
