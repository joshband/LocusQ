#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include "SpatialRendererTypes.h"

namespace locusq::spatial_headphone_pose
{

inline constexpr int kNumSpeakers = spatial_renderer_types::kNumSpeakers;
using ListenerOrientation = spatial_renderer_types::ListenerOrientation;
using SpeakerMixMatrix = std::array<std::array<float, kNumSpeakers>, kNumSpeakers>;

// World speaker vectors use scene coordinates where +Z is front.
inline constexpr std::array<std::array<float, 3>, kNumSpeakers> kQuadWorldSpeakerDirs
{{
    { -0.70710678f, 0.0f,  0.70710678f }, // FL
    {  0.70710678f, 0.0f,  0.70710678f }, // FR
    {  0.70710678f, 0.0f, -0.70710678f }, // RR
    { -0.70710678f, 0.0f, -0.70710678f }  // RL
}};

// Listener-local speaker vectors follow Steam canonical axes where -Z is ahead.
inline constexpr std::array<std::array<float, 2>, kNumSpeakers> kQuadListenerSpeakerDirsXZ
{{
    { -0.70710678f, -0.70710678f }, // FL
    {  0.70710678f, -0.70710678f }, // FR
    {  0.70710678f,  0.70710678f }, // RR
    { -0.70710678f,  0.70710678f }  // RL
}};

inline float dot3 (const std::array<float, 3>& lhs, const std::array<float, 3>& rhs) noexcept
{
    return (lhs[0] * rhs[0]) + (lhs[1] * rhs[1]) + (lhs[2] * rhs[2]);
}

inline bool normalizeVector3 (std::array<float, 3>& vector) noexcept
{
    const float magnitudeSq = dot3 (vector, vector);
    if (! std::isfinite (magnitudeSq) || magnitudeSq <= 1.0e-12f)
        return false;

    const float invMagnitude = 1.0f / std::sqrt (magnitudeSq);
    vector[0] *= invMagnitude;
    vector[1] *= invMagnitude;
    vector[2] *= invMagnitude;
    return std::isfinite (vector[0]) && std::isfinite (vector[1]) && std::isfinite (vector[2]);
}

inline void updateOrientationFromQuaternion (
    float qx,
    float qy,
    float qz,
    float qw,
    ListenerOrientation& orientation) noexcept
{
    const float xx = qx * qx;
    const float yy = qy * qy;
    const float zz = qz * qz;
    const float xy = qx * qy;
    const float xz = qx * qz;
    const float yz = qy * qz;
    const float xw = qx * qw;
    const float yw = qy * qw;
    const float zw = qz * qw;

    const float m00 = 1.0f - 2.0f * (yy + zz);
    const float m10 = 2.0f * (xy + zw);
    const float m20 = 2.0f * (xz - yw);

    const float m01 = 2.0f * (xy - zw);
    const float m11 = 1.0f - 2.0f * (xx + zz);
    const float m21 = 2.0f * (yz + xw);

    const float m02 = 2.0f * (xz + yw);
    const float m12 = 2.0f * (yz - xw);
    const float m22 = 1.0f - 2.0f * (xx + yy);

    orientation.right = { m00, m10, m20 };
    orientation.up = { m01, m11, m21 };
    orientation.ahead = { -m02, -m12, -m22 };
}

inline void setHeadPoseIdentityMix (SpeakerMixMatrix& speakerMix) noexcept
{
    for (int dst = 0; dst < kNumSpeakers; ++dst)
    {
        for (int src = 0; src < kNumSpeakers; ++src)
            speakerMix[static_cast<size_t> (dst)][static_cast<size_t> (src)] = (dst == src) ? 1.0f : 0.0f;
    }
}

inline void buildSpeakerMixFromOrientation (
    const ListenerOrientation& orientation,
    SpeakerMixMatrix& speakerMix) noexcept
{
    for (int sourceSpeaker = 0; sourceSpeaker < kNumSpeakers; ++sourceSpeaker)
    {
        const auto& worldDir = kQuadWorldSpeakerDirs[static_cast<size_t> (sourceSpeaker)];
        const float relX = dot3 (worldDir, orientation.right);
        const float relZ = dot3 (worldDir, orientation.ahead);
        const float planarMag = std::sqrt ((relX * relX) + (relZ * relZ));

        float planarX = 0.0f;
        float planarZ = -1.0f;
        if (planarMag > 1.0e-6f && std::isfinite (planarMag))
        {
            const float invPlanar = 1.0f / planarMag;
            planarX = relX * invPlanar;
            planarZ = relZ * invPlanar;
        }

        float weightSum = 0.0f;
        float bestDot = -2.0f;
        int bestSpeaker = 0;
        for (int targetSpeaker = 0; targetSpeaker < kNumSpeakers; ++targetSpeaker)
        {
            const auto& targetDir = kQuadListenerSpeakerDirsXZ[static_cast<size_t> (targetSpeaker)];
            const float projection = (planarX * targetDir[0]) + (planarZ * targetDir[1]);
            if (projection > bestDot)
            {
                bestDot = projection;
                bestSpeaker = targetSpeaker;
            }

            const float weight = std::max (0.0f, projection);
            speakerMix[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (sourceSpeaker)] = weight;
            weightSum += weight;
        }

        if (weightSum > 1.0e-6f && std::isfinite (weightSum))
        {
            const float invWeightSum = 1.0f / weightSum;
            for (int targetSpeaker = 0; targetSpeaker < kNumSpeakers; ++targetSpeaker)
            {
                speakerMix[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (sourceSpeaker)] *= invWeightSum;
            }
        }
        else
        {
            for (int targetSpeaker = 0; targetSpeaker < kNumSpeakers; ++targetSpeaker)
            {
                speakerMix[static_cast<size_t> (targetSpeaker)][static_cast<size_t> (sourceSpeaker)] =
                    (targetSpeaker == bestSpeaker) ? 1.0f : 0.0f;
            }
        }
    }
}

inline void mixHeadPoseAdjustedQuadSample (
    const SpeakerMixMatrix& speakerMix,
    float sourceFl,
    float sourceFr,
    float sourceRr,
    float sourceRl,
    float& fl,
    float& fr,
    float& rr,
    float& rl) noexcept
{
    const auto mixSpeaker = [&speakerMix, sourceFl, sourceFr, sourceRr, sourceRl] (int targetSpeaker) noexcept
    {
        const auto& mix = speakerMix[static_cast<size_t> (targetSpeaker)];
        return (mix[0] * sourceFl)
             + (mix[1] * sourceFr)
             + (mix[2] * sourceRr)
             + (mix[3] * sourceRl);
    };

    fl = mixSpeaker (0);
    fr = mixSpeaker (1);
    rr = mixSpeaker (2);
    rl = mixSpeaker (3);
}

inline void renderVirtual3dStereoFromQuad (
    float fl,
    float fr,
    float rr,
    float rl,
    float& left,
    float& right) noexcept
{
    left = (0.74f * fl) + (0.46f * rl) + (0.12f * fr) + (0.08f * rr);
    right = (0.74f * fr) + (0.46f * rr) + (0.12f * fl) + (0.08f * rl);
}

inline void renderStereoDownmixFromQuad (
    float fl,
    float fr,
    float rr,
    float rl,
    float& left,
    float& right) noexcept
{
    left = (fl + rl) * 0.707f;
    right = (fr + rr) * 0.707f;
}

} // namespace locusq::spatial_headphone_pose
