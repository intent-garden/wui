//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/common/error.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <string_view>

namespace wui
{

class i_config
{
public:
    virtual int32_t get_int(std::string_view section, std::string_view entry, int32_t default_) = 0;
    virtual void set_int(std::string_view section, std::string_view entry, int32_t value) = 0;

    virtual int64_t get_int64(std::string_view section, std::string_view entry, int64_t default_) = 0;
    virtual void set_int64(std::string_view section, std::string_view entry, int64_t value) = 0;

    virtual std::string get_string(std::string_view section, std::string_view entry, std::string_view default_) = 0;
    virtual void set_string(std::string_view section, std::string_view entry, std::string_view value) = 0;

    virtual void delete_value(std::string_view section, std::string_view entry) = 0;

    virtual void delete_key(std::string_view section) = 0;
       
    virtual error get_error() const = 0;

    virtual ~i_config() {}
};

}
