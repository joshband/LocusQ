#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace locusq::spatial_audition_primitives
{

inline float auditionLevelDbForPreset (int presetIndex) noexcept
{
    switch (presetIndex)
    {
        case 0: return -36.0f;
        case 1: return -30.0f;
        case 2: return -24.0f;
        case 3: return -18.0f;
        case 4: return -12.0f;
        default: break;
    }

    return -24.0f;
}

inline float advanceOscillator (double frequencyHz, double& phase, double currentSampleRate) noexcept
{
    const auto sampleRate = std::max (1.0, currentSampleRate);
    constexpr double kTwoPi = 6.28318530717958647692;
    const auto sample = std::sin (kTwoPi * phase);
    phase += frequencyHz / sampleRate;
    phase -= std::floor (phase);
    return static_cast<float> (sample);
}

inline float nextWhiteNoise (std::uint32_t& noiseState) noexcept
{
    noiseState = noiseState * 1664525u + 1013904223u;
    return static_cast<float> ((noiseState >> 8) & 0x00FFFFFFu) / 8388608.0f - 1.0f;
}

inline float nextRand01 (std::uint32_t& noiseState) noexcept
{
    return 0.5f * (nextWhiteNoise (noiseState) + 1.0f);
}

inline float wrapAzimuthDegrees (float azimuthDeg) noexcept
{
    while (azimuthDeg > 180.0f)
        azimuthDeg -= 360.0f;
    while (azimuthDeg < -180.0f)
        azimuthDeg += 360.0f;
    return azimuthDeg;
}

inline float voiceHashUnit (int voiceIndex, std::uint32_t salt) noexcept
{
    auto hash = static_cast<std::uint32_t> (voiceIndex + 1);
    hash ^= salt;
    hash ^= hash >> 16;
    hash *= 0x7FEB352Du;
    hash ^= hash >> 15;
    hash *= 0x846CA68Bu;
    hash ^= hash >> 16;
    return static_cast<float> (hash & 0x00FFFFFFu) / 16777215.0f;
}

inline bool isMultiSourceSignal (int signalIndex) noexcept
{
    switch (signalIndex)
    {
        case 3:  // rain
        case 4:  // snow
        case 5:  // bouncing balls
        case 6:  // wind chimes
        case 7:  // crickets
        case 8:  // song birds
        case 9:  // karplus plucks
        case 10: // membrane drops
        case 11: // krell patch
        case 12: // generative arp
            return true;
        default:
            return false;
    }
}

inline int voiceCountForSignal (int signalIndex, bool qualityHigh) noexcept
{
    if (! isMultiSourceSignal (signalIndex))
        return 1;

    switch (signalIndex)
    {
        case 3: // rain
        case 4: // snow
        case 7: // crickets
        case 8: // song birds
            return qualityHigh ? 7 : 5;
        case 5: // bouncing
            return qualityHigh ? 6 : 4;
        case 6:  // chimes
        case 9:  // karplus
        case 10: // membrane
        case 12: // arp
            return qualityHigh ? 5 : 4;
        case 11: // krell
            return qualityHigh ? 4 : 3;
        default:
            return 1;
    }
}

inline float voiceSpreadDegreesForSignal (int signalIndex) noexcept
{
    switch (signalIndex)
    {
        case 3: // rain
        case 4: // snow
        case 5: // bouncing balls
        case 7: // crickets
        case 8: // song birds
            return 172.0f;
        case 6:  // chimes
        case 9:  // karplus plucks
        case 10: // membrane drops
        case 11: // krell patch
        case 12: // generative arp
            return 156.0f;
        default:
            return 0.0f;
    }
}

inline int voiceDelaySamplesForSignal (int signalIndex,
                                       bool qualityHigh,
                                       int voiceIndex,
                                       int voiceCount,
                                       double currentSampleRate,
                                       int historyBufferSamples) noexcept
{
    if (voiceIndex <= 0 || voiceCount <= 1)
        return 0;

    int maxDelayMs = 18;
    switch (signalIndex)
    {
        case 3: // rain
        case 4: // snow
            maxDelayMs = qualityHigh ? 95 : 70;
            break;
        case 5: // bouncing balls
            maxDelayMs = qualityHigh ? 140 : 95;
            break;
        case 7: // crickets
        case 8: // birds
            maxDelayMs = qualityHigh ? 82 : 56;
            break;
        case 6:  // chimes
        case 9:  // plucks
        case 10: // membrane
        case 11: // krell
        case 12: // arp
            maxDelayMs = qualityHigh ? 62 : 44;
            break;
        default:
            maxDelayMs = 18;
            break;
    }

    const auto voiceNorm = static_cast<float> (voiceIndex)
        / static_cast<float> (std::max (1, voiceCount - 1));
    const auto jitterMs = 10.0f * voiceHashUnit (voiceIndex, 0xA53C9E11u);
    const auto delayMs = std::clamp (
        static_cast<float> (maxDelayMs) * voiceNorm + jitterMs,
        0.0f,
        static_cast<float> (maxDelayMs));
    const auto sampleRate = std::max (1.0, currentSampleRate);
    const auto computedSamples = static_cast<int> (std::round (
        delayMs * static_cast<float> (sampleRate) * 0.001f));
    return std::clamp (computedSamples, 0, std::max (0, historyBufferSamples - 1));
}

} // namespace locusq::spatial_audition_primitives
