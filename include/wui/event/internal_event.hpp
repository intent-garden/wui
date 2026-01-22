//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

namespace wui
{

enum class internal_event_type
{
    window_created,

    set_focus,
    remove_focus,
    execute_focused,
    
    size_changed,
    position_changed,

    window_expanded,
    window_normalized,
    window_minimized,

    user_emitted
};

struct internal_event
{
    internal_event_type type;
    int32_t x, y;
};

}
