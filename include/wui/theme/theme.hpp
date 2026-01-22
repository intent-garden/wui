//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/theme/i_theme.hpp>

#include <memory>

namespace wui
{

/// Set and get the current theme
#ifdef _WIN32
bool set_default_theme_from_resource(std::string_view name, int32_t resource_index, std::string_view resource_section);
#endif
bool set_default_theme_from_json(std::string_view name, std::string_view json);
bool set_default_theme_from_file(std::string_view name, std::string_view file_name);
void set_default_theme_empty(std::string_view name);

/// Load theme from regsitry on Windows or from file on other systems
/// Parameters are setted by set_app_themes() in theme_selector.hpp
bool set_default_theme_from_name(std::string_view name, error &err);

/// Return details of the error that occurred
error get_theme_error();

/// Return the pointer to current default theme instance
std::shared_ptr<i_theme> get_default_theme();

/// Make the custom theme for the some control
std::shared_ptr<i_theme> make_custom_theme(std::string_view name = "");
std::shared_ptr<i_theme> make_custom_theme(std::string_view name, std::string_view json);

/// Return the item's color by current theme
color theme_color(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_ = nullptr);

/// Return the item's dimension by current theme
int32_t theme_dimension(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_ = nullptr);

/// Return the item's string value by current theme
const std::string &theme_string(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_ = nullptr);

/// Return the item's font value by current theme
font theme_font(std::string_view control, std::string_view value, std::shared_ptr<i_theme> theme_ = nullptr);

const std::vector<uint8_t> &theme_image(std::string_view name, std::shared_ptr<i_theme> theme_ = nullptr);

}
