// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#pragma once

#include <wui/common/rect.hpp>

namespace wui
{
inline uint32_t clamp_corner_radius(rect bounds, uint32_t requested)
{
    if (bounds.width() <= 0 || bounds.height() <= 0) return 0;
    const auto shortest = bounds.width() < bounds.height() ? bounds.width() : bounds.height();
    const auto maximum = static_cast<uint32_t>(shortest / 2);
    return requested < maximum ? requested : maximum;
}
}
