//
// framework.hpp - Contains the ui starting / stoping interface
//
// Copyright (c) 2014-2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/common/error.hpp>

namespace wui
{

namespace framework
{

void init();

void run();
void stop();

bool started();

error get_error();

}

}
