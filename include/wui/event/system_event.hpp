//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <string_view>

namespace wui
{

enum class system_event_type
{
    undefined = 0,

    device_connected,
    device_disconnected,
    device_reordered
};

enum class device_type
{
    undefined = 0,

    usb,        /// On Linux all devices on disconnecting

    monitor,
    camera,
    
    microphone,
    speaker,
    media,      /// It's microphone or speaker

    storage,
    smartcard,

    network,

    keyboard,
    mouse,
    hid         /// It's keyboard or mouse
};

struct system_event
{
    system_event_type type;
    device_type device;
    
    uint64_t w, l;
};

std::string_view to_string(system_event_type t);
std::string_view to_string(device_type t);

}
