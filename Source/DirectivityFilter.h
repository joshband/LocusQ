#pragma once

#include "SceneGraph.h"

#include <array>
#include <algorithm>
#include <cmath>

//==============================================================================
/**
 * DirectivityFilter
 *
 * Cardioid-like directivity shaping for speaker gains.
 * directivity = 0.0 -> omnidirectional
 * directivity = 1.0 -> tight cardioid pattern
 */
class DirectivityFilter
{
public:
    static constexpr int NUM_SPEAKERS = 4;

    void apply (std::array<float, NUM_SPEAKERS>& gains,
                float directivity,
                const Vec3& directivityAim,
                const Vec3& emitterPosition) const
    {
        const float directivityClamped = std::clamp (directivity, 0.0f, 1.0f);
        if (directivityClamped <= 0.0f)
            return;

        const Vec3 aim = normalize (directivityAim);
        if (lengthSq (aim) < 1.0e-6f)
            return;

        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            const Vec3 toSpeaker = normalize (Vec3
            {
                speakerPositions[static_cast<size_t> (spk)].x - emitterPosition.x,
                speakerPositions[static_cast<size_t> (spk)].y - emitterPosition.y,
                speakerPositions[static_cast<size_t> (spk)].z - emitterPosition.z
            });

            float cosTheta = dot (aim, toSpeaker);
            cosTheta = std::clamp (cosTheta, -1.0f, 1.0f);

            // Cardioid-like response in [0, 1].
            const float cardioid = std::clamp (0.5f * (1.0f + cosTheta), 0.0f, 1.0f);
            const float gainScale = (1.0f - directivityClamped) + directivityClamped * cardioid;

            gains[static_cast<size_t> (spk)] *= gainScale;
        }
    }

private:
    static float lengthSq (const Vec3& v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    static Vec3 normalize (const Vec3& v)
    {
        const float len2 = lengthSq (v);
        if (len2 < 1.0e-8f)
            return {};

        const float invLen = 1.0f / std::sqrt (len2);
        return { v.x * invLen, v.y * invLen, v.z * invLen };
    }

    static float dot (const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // Nominal quad speaker placement around listener.
    const std::array<Vec3, NUM_SPEAKERS> speakerPositions
    {
        Vec3 { -2.5f, 1.2f,  2.0f }, // FL
        Vec3 {  2.5f, 1.2f,  2.0f }, // FR
        Vec3 {  2.5f, 1.2f, -2.0f }, // RR
        Vec3 { -2.5f, 1.2f, -2.0f }  // RL
    };
};
