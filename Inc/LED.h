#pragma once

#include <vector>
#include <utility>

namespace LED
{
    struct BlinkPattern
    {
        bool oneshot = false;
        std::vector<std::pair<uint8_t, uint16_t>> pattern; // mask, duration
    };

    void init();
    void deinit();
    void resetIndex();

    void setCurrentPattern(const BlinkPattern &pattern);
} // namespace LED
