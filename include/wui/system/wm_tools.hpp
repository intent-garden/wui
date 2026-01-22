//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/system/system_context.hpp>

#include <wui/common/rect.hpp>

namespace wui
{

/// Hide the WM's task bar icon
void hide_taskbar_icon(system_context &ctx);

/// Show the WM's task bar icon
void show_taskbar_icon(system_context &ctx);

/// Determine screen dimensions
rect get_screen_size(system_context &context);

}
