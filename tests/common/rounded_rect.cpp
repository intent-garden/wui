#include "../../src/graphic/rounded_rect.hpp"

#include <iostream>
#include <limits>

int main()
{
    const struct { wui::rect bounds; uint32_t requested; uint32_t expected; } cases[] = {
        {{0, 0, 24, 24}, 24, 12}, // Radio outline: radius is not diameter.
        {{0, 0, 8, 8}, 24, 4},   // Selected radio dot.
        {{0, 0, 48, 24}, 24, 12}, // Switch track.
        {{0, 0, 16, 16}, 16, 8}, // Switch thumb.
        {{0, 0, 24, 24}, 4, 4},  // Checkbox corners unchanged.
        {{10, 10, 35, 35}, 25, 12},
        {{0, 0, 24, 24}, 0, 0},
        {{0, 0, 24, 24}, (std::numeric_limits<uint32_t>::max)(), 12},
        {{0, 0, 0, 24}, 24, 0},
        {{10, 10, 0, 0}, 24, 0},
    };
    for (const auto& item : cases)
    {
        if (wui::clamp_corner_radius(item.bounds, item.requested) != item.expected)
        {
            std::cerr << "Invalid rounded rectangle radius\n";
            return 1;
        }
    }
    std::cout << "PASS: rounded indicator bounds (10 cases)\n";
}
