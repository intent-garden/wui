//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <string>
#include <vector>

namespace wui
{

struct theme_params
{
    std::string name;
    std::string file_name;
    int32_t resource_id;

    inline bool operator==(const theme_params &lv)
    {
        return name == lv.name;
    }

    inline bool operator==(std::string_view nm)
    {
        return name == nm;
    }
};

using themes_t = std::vector<theme_params>;

/// A function that memorizes the themes supported by the application.
/// The names, file paths or resource icons of each available theme are set here
void set_app_themes(const themes_t &);

/// If there is no name to be selected this theme will be used
void set_default_theme(std::string_view name);

/// Return the theme params of theme name
theme_params get_app_theme(std::string_view name);

/// Revolving theme search
void set_current_app_theme(std::string_view name);
std::string get_next_app_theme();

/// Return all setted themes in vector
const themes_t &get_app_themes();

}
