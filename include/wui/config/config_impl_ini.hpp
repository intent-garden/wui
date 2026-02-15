//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/config/i_config.hpp>

#include <map>

namespace wui
{

namespace config
{

class config_impl_ini : public i_config
{
public:
    config_impl_ini(std::string_view file_name);
    ~config_impl_ini();

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
    enum class value_type
    {
        string,
        int64,
        double_,

        only_comment
    };
    struct value
    {
        value_type  type;

        int64_t     int_val;
        std::string str_val;
        double      dbl_val;

        std::string comment;
    };

    std::string file_name;

    std::map<std::pair<std::string, std::string>, value> values;

    error err;

    bool changed;

    bool load_values();
    bool save_values();
};

}

}
