// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#pragma once
#include <algorithm>
#include <cmath>

namespace wui::media::detail
{
// Prefer the highest supported rate at/below the requested rate. If every
// available rate is higher, choose the lowest of those instead.
struct frame_rate_selection
{
    double value = 0;
    bool consider(double requested, double minimum, double maximum)
    {
        if (!std::isfinite(requested) || !std::isfinite(minimum) || !std::isfinite(maximum)
            || requested <= 0 || minimum <= 0 || maximum < minimum) return false;
        const double candidate = std::clamp(requested, minimum, maximum);
        const bool below = candidate <= requested;
        const bool previous_below = value <= requested;
        if (value == 0 || (below && !previous_below)
            || (below == previous_below && (below ? candidate > value : candidate < value))) {
            value = candidate;
            return true;
        }
        return false;
    }
};
}
