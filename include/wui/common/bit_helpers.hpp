//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

namespace wui
{

template <typename T, typename U>
inline void set_bit(T &reg, U bit)
{
    reg |= (1 << static_cast<int>(bit));
}

template <typename T, typename U>
inline void clear_bit(T &reg, U bit)
{
    reg &= (~(1 << static_cast<int>(bit)));
}

template <typename T, typename U>
inline void inv_bit(T &reg, U bit)
{
    reg ^= (1 << static_cast<int>(bit));
}

template <typename T, typename U>
inline bool bit_is_set(T reg, U bit)
{
    return ((static_cast<int>(reg) & (1 << static_cast<int>(bit))) != 0);
}

template <typename T, typename U>
inline bool bit_is_clear(T reg, U bit)
{
    return ((static_cast<int>(reg) & (1 << static_cast<int>(bit))) == 0);
}

}
