//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/common/error.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace wui
{

class i_framework
{
public:
    virtual void run() = 0;
    virtual void stop() = 0;

    virtual bool started() const = 0;

    virtual error get_error() const = 0;

    virtual ~i_framework() {}
};

}
