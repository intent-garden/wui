//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstdint>
#include <cstdarg>

namespace wui
{

template <typename T>
inline bool flag_is_set(T value, T flag)
{
    return ((static_cast<int>(value)) & (static_cast<int>(flag)));
}

template <typename T>
T flags_map(int32_t cnt, ...)
{
    va_list valist;

    int32_t out = 0;

    va_start(valist, cnt);

    for (auto i = 0; i != cnt; i++)
    {
        out |= va_arg(valist, int32_t);
    }

    va_end(valist);

    return static_cast<T>(out);
}


}
