#pragma once

#include <algorithm>
#include <cmath>

namespace locusq::spatial_headphone_pose
{

struct HeadphoneCompensationConfig
{
    float lowAlpha = 0.0f;
    float lowGain = 1.0f;
    float highGain = 1.0f;
    float crossfeed = 0.0f;
};

inline HeadphoneCompensationConfig makeHeadphoneCompensationConfig (
    int profileIndex,
    double currentSampleRate) noexcept
{
    constexpr float pi = 3.14159265358979323846f;
    constexpr float lowCutoffHz = 700.0f;
    const float sampleRate = std::max (1.0f, static_cast<float> (currentSampleRate));

    HeadphoneCompensationConfig config;
    config.lowAlpha = std::clamp (
        1.0f - std::exp (-2.0f * pi * lowCutoffHz / sampleRate),
        1.0e-4f,
        1.0f);

    switch (profileIndex)
    {
        case 1: // AirPods Pro 2
        case 2: // AirPods Pro 3
            config.lowGain = 0.98f;
            config.highGain = 1.03f;
            config.crossfeed = 0.015f;
            break;
        case 3: // Sony WH-1000XM5
            config.lowGain = 1.04f;
            config.highGain = 0.97f;
            config.crossfeed = 0.020f;
            break;
        case 4: // Custom SOFA
            config.lowGain = 1.00f;
            config.highGain = 1.00f;
            config.crossfeed = 0.010f;
            break;
        case 0: // Generic
        default:
            config.lowGain = 1.00f;
            config.highGain = 1.00f;
            config.crossfeed = 0.0f;
            break;
    }

    return config;
}

inline void resetHeadphoneCompensationState (
    float& lowStateLeft,
    float& lowStateRight) noexcept
{
    lowStateLeft = 0.0f;
    lowStateRight = 0.0f;
}

inline void applyHeadphoneCompensation (
    float& left,
    float& right,
    const HeadphoneCompensationConfig& config,
    float& lowStateLeft,
    float& lowStateRight) noexcept
{
    if (config.crossfeed == 0.0f
        && config.lowGain == 1.0f
        && config.highGain == 1.0f)
    {
        return;
    }

    const float inLeft = left;
    const float inRight = right;
    lowStateLeft += config.lowAlpha * (inLeft - lowStateLeft);
    lowStateRight += config.lowAlpha * (inRight - lowStateRight);

    const float highLeft = inLeft - lowStateLeft;
    const float highRight = inRight - lowStateRight;
    const float eqLeft = (lowStateLeft * config.lowGain) + (highLeft * config.highGain);
    const float eqRight = (lowStateRight * config.lowGain) + (highRight * config.highGain);

    left = eqLeft + (inRight * config.crossfeed);
    right = eqRight + (inLeft * config.crossfeed);

    if (! std::isfinite (left))
        left = 0.0f;
    if (! std::isfinite (right))
        right = 0.0f;
}

} // namespace locusq::spatial_headphone_pose
