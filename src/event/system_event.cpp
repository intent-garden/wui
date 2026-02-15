//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/event/system_event.hpp>

namespace wui
{

std::string_view to_string(system_event_type t)
{
    switch (t)
    {
    case system_event_type::device_connected: return "device_connected";
    case system_event_type::device_disconnected: return "device_disconnected";
    case system_event_type::device_reordered: return "device_reordered";
    default: return "undefined";
    }
}

std::string_view to_string(device_type t)
{
    switch (t)
    {
    case device_type::usb: return "usb";
    case device_type::monitor: return "monitor";
    case device_type::camera: return "camera";
    case device_type::microphone: return "microphone";
    case device_type::speaker: return "speaker";
    case device_type::media: return "media";
    case device_type::storage: return "storage";
    case device_type::smartcard: return "smartcard";
    case device_type::network: return "network";
    case device_type::keyboard: return "keyboard";
    case device_type::mouse: return "mouse";
    case device_type::hid: return "hid";
    default: return "undefined";
    }
}

}
