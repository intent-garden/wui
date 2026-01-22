//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/control/tray_icon.hpp>
#include <boost/nowide/convert.hpp>

namespace wui
{

#ifdef _WIN32
tray_icon::tray_icon(std::weak_ptr<window> parent__, int32_t icon_resource_index_, std::string_view tip_, std::function<void(tray_icon_action action)> click_callback_)
    : parent(parent__),
    icon_resource_index(icon_resource_index_),
    icon_file_name(),
    tip(tip_),
    click_callback(click_callback_),
    my_subscriber_id()
{
    auto parent_ = parent.lock();
    if (parent_)
    {
        my_subscriber_id = parent_->subscribe(std::bind(&tray_icon::receive_event, this, std::placeholders::_1), event_type::internal);

        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.uVersion = NOTIFYICON_VERSION_4;
        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_resource_index));
        wcscpy_s(nid.szTip, 128, boost::nowide::widen(tip).c_str());
        Shell_NotifyIconW(NIM_ADD, &nid);
    }
}
#endif

tray_icon::tray_icon(std::weak_ptr<window> parent__, std::string_view icon_file_name_, std::string_view tip_, std::function<void(tray_icon_action action)> click_callback_)
    : parent(parent__),
#ifdef _WIN32
    icon_resource_index(-1),
#endif
    icon_file_name(icon_file_name_),
    tip(tip_),
    click_callback(click_callback_),
    my_subscriber_id()
{
    auto parent_ = parent.lock();
    if (parent_)
    {
        my_subscriber_id = parent_->subscribe(std::bind(&tray_icon::receive_event, this, std::placeholders::_1), event_type::internal);

#ifdef _WIN32
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.uVersion = NOTIFYICON_VERSION_4;
        nid.hIcon = (HICON)LoadImageW(NULL, boost::nowide::widen(icon_file_name).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        wcscpy_s(nid.szTip, 128, boost::nowide::widen(tip).c_str());
        Shell_NotifyIconW(NIM_ADD, &nid);
#elif __linux__
#endif
    }
}

tray_icon::~tray_icon()
{
    auto parent_ = parent.lock();
    if (parent_)
    {
        parent_->unsubscribe(my_subscriber_id);

#ifdef _WIN32
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        Shell_NotifyIconW(NIM_DELETE, &nid);
#endif
    }
}

#ifdef _WIN32
void tray_icon::change_icon(int32_t icon_resource_index_)
{
    icon_resource_index = icon_resource_index_;
    auto parent_ = parent.lock();
    if (parent_)
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.uVersion = NOTIFYICON_VERSION_4;
        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_resource_index));
        wcscpy_s(nid.szTip, 128, boost::nowide::widen(tip).c_str());

        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
}
#endif

void tray_icon::change_icon(std::string_view icon_file_name_)
{
    icon_file_name = icon_file_name_;
#ifdef _WIN32
    auto parent_ = parent.lock();
    if (parent_)
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.uVersion = NOTIFYICON_VERSION_4;
        nid.hIcon = (HICON)LoadImageW(NULL, boost::nowide::widen(icon_file_name).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        wcscpy_s(nid.szTip, 128, boost::nowide::widen(tip).c_str());

        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
#elif __linux__
#endif
}

void tray_icon::change_tip(std::string_view tip_)
{
    tip = tip_;
#ifdef _WIN32
    auto parent_ = parent.lock();
    if (parent_)
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.uVersion = NOTIFYICON_VERSION_4;
        nid.hIcon = GetHIcon();
        wcscpy_s(nid.szTip, 128, boost::nowide::widen(tip).c_str());

        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
#elif __linux__
#endif
}

void tray_icon::show_message(std::string_view title, std::string_view message)
{
#ifdef _WIN32
    auto parent_ = parent.lock();
    if (parent_)
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = parent_->context().hwnd;
        nid.uID = ID;
        nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
        nid.uCallbackMessage = WM_USER;
        nid.uVersion = NOTIFYICON_VERSION_4;
        nid.hIcon = GetHIcon();
        wcscpy_s(nid.szTip, 128, boost::nowide::widen(tip).c_str());

        nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
        if (!title.empty())
        {
            wcscpy_s(nid.szInfoTitle, 128, boost::nowide::widen(title).c_str());
    }
        if (!message.empty())
        {
            wcscpy_s(nid.szInfo, 128, boost::nowide::widen(message).c_str());
        }

        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
#elif __linux__
#endif
}

void tray_icon::set_callback(std::function<void(tray_icon_action action)> click_callback_)
{
    click_callback = click_callback_;
}

void tray_icon::receive_event(const event &ev)
{
#ifdef _WIN32
    if (ev.internal_event_.type == wui::internal_event_type::user_emitted && ev.internal_event_.x == ID)
    {
        switch (ev.internal_event_.y)
        {
            case WM_LBUTTONDOWN:
                click_callback(tray_icon_action::left_click);
            break;
            case WM_RBUTTONDOWN:
                click_callback(tray_icon_action::right_click);
            break;
            case WM_MBUTTONDOWN:
                click_callback(tray_icon_action::center_click);
            break;
            case WM_USER + 5:
                click_callback(tray_icon_action::message_click);
            break;
        }
    }
#elif __linux__
#endif
}

#ifdef _WIN32
HICON tray_icon::GetHIcon()
{
    if (icon_resource_index != -1 && icon_file_name.empty())
    {
        return LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_resource_index));
    }
    else if (icon_resource_index == -1 && !icon_file_name.empty())
    {
        return (HICON)LoadImageW(NULL, boost::nowide::widen(icon_file_name).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }

    return NULL;
}
#endif

}
