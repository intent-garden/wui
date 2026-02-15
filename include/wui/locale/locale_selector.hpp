//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/locale/locale_type.hpp>
#include <vector>

namespace wui
{

struct locale_params
{
    locale_type type;
    std::string name;
    std::string file_name;
    int32_t resource_id;

    inline bool operator==(const locale_params &lv)
    {
        return type == lv.type;
    }

    inline bool operator==(locale_type lt)
    {
        return type == lt;
    }
};

using locales_t = std::vector<locale_params>;

/// A function that memorizes the locales supported by the application.
/// The names, file paths or resource icons of each available locale are set here
void set_app_locales(const locales_t &);

/// If there is no locale_type to be selected this locale will be used
void set_default_locale(locale_type);

/// Return the locale params of locale type
locale_params get_app_locale(locale_type);

/// Revolving locale search
void set_current_app_locale(locale_type);
locale_type get_next_app_locale();

/// Calls the system function to return system's locale
locale_type get_default_system_locale();

/// Return all setted locales in vector
const locales_t &get_app_locales();

}
