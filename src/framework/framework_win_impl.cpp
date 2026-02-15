//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#ifdef _WIN32

#include <wui/framework/framework_win_impl.hpp>

#include <windows.h>

namespace wui
{

namespace framework
{

framework_win_impl::framework_win_impl()
    : started_(false),
    err{}
{
}

void framework_win_impl::run()
{
    if (started_)
    {
        err.type = error_type::already_started;
        err.component = "framework_win_impl::run()";

        return;
    }
    started_ = true;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void framework_win_impl::stop()
{
    if (started_)
    {
        PostQuitMessage(IDCANCEL);
        started_ = false;

        err.reset();
    }
}

bool framework_win_impl::started() const
{
    return started_;
}

error framework_win_impl::get_error() const
{
    return err;
}

}

}

#endif
