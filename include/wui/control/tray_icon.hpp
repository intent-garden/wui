//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
//

#pragma once

#include <wui/event/event.hpp>
#include <wui/window/window.hpp>

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace wui
{

enum class tray_icon_action
{
    left_click,
    right_click,
    center_click,
    message_click
};

class tray_icon : public std::enable_shared_from_this<tray_icon>
{
public:
#ifdef _WIN32
    tray_icon(std::weak_ptr<window> parent, int32_t icon_resource_index, std::string_view tip, std::function<void(tray_icon_action action)> click_callback);
#endif
    tray_icon(std::weak_ptr<window> parent, std::string_view icon_file_name, std::string_view tip, std::function<void(tray_icon_action action)> click_callback);
    ~tray_icon();

#ifdef _WIN32
    void change_icon(int32_t icon_resource_index);
#endif
    void change_icon(std::string_view icon_file_name);

    void change_tip(std::string_view tip);

    void show_message(std::string_view title, std::string_view message);

    void set_callback(std::function<void(tray_icon_action action)> click_callback);

private:
    static const uint32_t ID = 3556;

    std::weak_ptr<window> parent;
#ifdef _WIN32
    int32_t icon_resource_index;
#endif
    std::string icon_file_name;
    std::string tip;
    std::function<void(tray_icon_action action)> click_callback;

    std::string my_subscriber_id;

    void receive_event(const event &ev);

#ifdef _WIN32
    HICON GetHIcon();
#endif
};

}
