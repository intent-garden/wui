//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
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

    // Явно установить нормальный курсор в начале цикла
    // Это предотвращает показ курсора ожидания при старте приложения
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // Периодически проверять и восстанавливать нормальный курсор
        // Windows может автоматически менять курсор на ожидание, если приложение долго не обрабатывает сообщения
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        
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
