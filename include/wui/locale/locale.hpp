//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/locale/i_locale.hpp>

#include <memory>

namespace wui
{

/// Set and get the current locale
#ifdef _WIN32
bool set_locale_from_resource(locale_type type, std::string_view name, int32_t resource_index, std::string_view resource_section);
#endif
bool set_locale_from_json(locale_type type, std::string_view name, std::string_view json);
bool set_locale_from_file(locale_type type, std::string_view name, std::string_view file_name);
void set_locale_empty(locale_type type, std::string_view name);

/// Load locale from regsitry on Windows or from file on other systems
/// Parameters are setted by set_app_locales() in locale_selector.hpp
bool set_locale_from_type(locale_type type, error &err);

error get_locale_error();

/// Return the pointer to current default locale instance
std::shared_ptr<i_locale> get_locale();

/// Set locale section value
void set_locale_value(std::string_view section, std::string_view value, std::string_view str);

/// Return the item's string value by current locale
const std::string &locale(std::string_view section, std::string_view value);

}
