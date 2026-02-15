//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#ifdef __linux__

#include <wui/framework/framework_lin_impl.hpp>

#include <thread>

namespace wui
{

namespace framework
{

framework_lin_impl::framework_lin_impl()
    : started_(false),
    err{}
{
}

void framework_lin_impl::run()
{
    if (started_)
    {
        err.type = error_type::already_started;
        err.component = "framework_lin_impl::run()";

        return;
    }
    
    started_ = true;

    while (started_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void framework_lin_impl::stop()
{
    if (started_)
    {
        started_ = false;

        err.reset();
    }
}

bool framework_lin_impl::started() const
{
    return started_;
}

error framework_lin_impl::get_error() const
{
    return err;
}

}

}

#endif
