//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/system/wm_tools.hpp>

#ifdef _WIN32
#pragma warning(suppress: 4091)
#include <shobjidl.h>
#endif

namespace wui
{

#ifdef _WIN32
void hide_taskbar_icon(system_context &ctx)
{
    ITaskbarList* pTaskbar = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_TaskbarList,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITaskbarList,
        reinterpret_cast<void**>(&pTaskbar)
    );

    if (SUCCEEDED(hr) && pTaskbar)
    {
        hr = pTaskbar->HrInit();
        if (SUCCEEDED(hr))
        {
            pTaskbar->DeleteTab(ctx.hwnd);
        }
        pTaskbar->Release();
    }
}

void show_taskbar_icon(system_context &ctx)
{
    ITaskbarList* pTaskbar = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_TaskbarList,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITaskbarList,
        reinterpret_cast<void**>(&pTaskbar)
    );

    if (SUCCEEDED(hr) && pTaskbar)
    {
        hr = pTaskbar->HrInit();
        if (SUCCEEDED(hr))
        {
            pTaskbar->AddTab(ctx.hwnd);
        }
        pTaskbar->Release();
    }
}

rect get_screen_size(system_context &context)
{
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfo(MonitorFromWindow(context.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
    {
        auto width = mi.rcMonitor.right - mi.rcMonitor.left;
        auto height = mi.rcMonitor.bottom - mi.rcMonitor.top;

        return { 0, 0, width, height };
    }

    return { 0 };
}

#else

void hide_taskbar_icon(system_context &ctx)
{
}

void show_taskbar_icon(system_context &ctx)
{
}

rect get_screen_size(system_context &context)
{
    if (context.screen)
    {
        return { 0, 0, context.screen->width_in_pixels, context.screen->height_in_pixels };
    }
    return { 0 };
}

#endif

}
