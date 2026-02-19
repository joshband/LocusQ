#pragma once

#include <array>
#include <algorithm>

//==============================================================================
/**
 * SpreadProcessor
 *
 * Blends focused panning gains toward a diffuse distribution.
 * spread = 0.0 -> focused (VBAP result)
 * spread = 1.0 -> diffuse (equal-power quad distribution)
 */
class SpreadProcessor
{
public:
    static constexpr int NUM_SPEAKERS = 4;

    void apply (std::array<float, NUM_SPEAKERS>& gains, float spread) const
    {
        const float s = std::clamp (spread, 0.0f, 1.0f);
        if (s <= 0.0f)
            return;

        // Equal-power diffuse target for 4 speakers.
        static constexpr float diffuseGain = 0.5f;

        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            const float focused = gains[static_cast<size_t> (spk)];
            gains[static_cast<size_t> (spk)] = focused * (1.0f - s) + diffuseGain * s;
        }
    }
};
