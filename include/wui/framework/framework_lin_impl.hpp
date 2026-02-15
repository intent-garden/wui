//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/framework/i_framework.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <atomic>

namespace wui
{

namespace framework
{

class framework_lin_impl : public i_framework
{
public:
    framework_lin_impl();

    virtual void run();
    virtual void stop();

    virtual bool started() const;

    virtual error get_error() const;

private:
    std::atomic<bool> started_;

    error err;
};

}


}

