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

#include <map>

namespace wui
{

class locale_impl : public i_locale
{
public:
    locale_impl(locale_type type, std::string_view name);

    virtual locale_type get_type() const;
    virtual std::string get_name() const;

    virtual void set(std::string_view section, std::string_view value, std::string_view str);
    virtual const std::string &get(std::string_view section, std::string_view value) const;

#ifdef _WIN32
    virtual void load_resource(int32_t resource_index, std::string_view resource_section);
#endif
    virtual void load_json(std::string_view json);
    virtual void load_file(std::string_view file_name);
    virtual void load_locale(const i_locale &locale_);

    virtual error get_error() const;

private:
    locale_type type;
    std::string name;

    std::map<std::pair<std::string, std::string>, std::string> strings;

    std::string dummy_string;

    error err;
};

}
