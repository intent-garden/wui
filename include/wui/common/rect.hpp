//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <cstdint>

namespace wui
{

struct rect
{
    int32_t left, top, right, bottom;

    inline bool operator==(rect lv)
    {
        return left == lv.left && top == lv.top && right == lv.right && bottom == lv.bottom;
    }

    inline bool operator>(rect lv)
    {
        return width() > lv.width() && height() > lv.height();
    }

    inline rect operator+(rect lv)
    {
        return rect{ left + lv.left, top + lv.top, left + lv.right, top + lv.bottom };
    }

    inline bool in(int32_t x, int32_t y) const
    {
        return x >= left && x <= right && y >= top && y <= bottom;
    }

    inline bool in(rect outer) const
    {
        return !((outer.right <= left || right <= outer.left || outer.bottom <= top || bottom <= outer.top));
    }

    inline bool is_null() const
    {
        return left == 0 && top == 0 && right == 0 && bottom == 0;
    }

    inline int32_t width() const
    {
        return right - left;
    }

    inline int32_t height() const
    {
        return bottom - top;
    }

    inline void move(int32_t x, int32_t y)
    {
        left += x;
        top += y;
        right += x;
        bottom += y;
    }

    inline void put(int32_t x, int32_t y)
    {
        right = x + width();
        bottom = y + height();
        left = x;
        top = y;
    }

    inline void widen(int32_t val)
    {
        left -= val;
        top -= val;
        right += val;
        bottom += val;
    }
};

}
