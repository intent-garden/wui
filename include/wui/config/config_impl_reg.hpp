//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#ifdef _WIN32

#include <wui/config/i_config.hpp>

#include <windows.h>

namespace wui
{

namespace config
{

class config_impl_reg : public i_config
{
public:
    config_impl_reg(std::string_view base_application_key, HKEY root = HKEY_CURRENT_USER);
    virtual ~config_impl_reg();

    int32_t get_int(std::string_view section, std::string_view entry, int32_t default_);
    void set_int(std::string_view section, std::string_view entry, int32_t value);

    int64_t get_int64(std::string_view section, std::string_view entry, int64_t default_);
    void set_int64(std::string_view section, std::string_view entry, int64_t value);

    std::string get_string(std::string_view section, std::string_view entry, std::string_view default_);
    void set_string(std::string_view section, std::string_view entry, std::string_view value);

    void delete_value(std::string_view section, std::string_view entry);

    void delete_key(std::string_view section);

    virtual error get_error() const;

private:
    std::string base_application_key;
    HKEY root;
};

}

}

#endif
