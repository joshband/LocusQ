#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "SpatialRendererTypes.h"

namespace locusq::spatial_profile_router
{

using SpatialOutputProfile = spatial_renderer_types::SpatialOutputProfile;
using SpatialProfileStage = spatial_renderer_types::SpatialProfileStage;

struct SpatialProfileResolution
{
    SpatialOutputProfile profile = SpatialOutputProfile::Auto;
    SpatialProfileStage stage = SpatialProfileStage::Direct;
};

inline bool isStereoOrBinauralProfile (SpatialOutputProfile profile) noexcept
{
    switch (profile)
    {
        case SpatialOutputProfile::Stereo20:
        case SpatialOutputProfile::Virtual3dStereo:
        case SpatialOutputProfile::AmbisonicFOA:
        case SpatialOutputProfile::AmbisonicHOA:
            return true;
        default:
            break;
    }

    return false;
}

inline SpatialProfileResolution resolveSpatialProfileForHost (
    SpatialOutputProfile requested,
    int numOutputChannels,
    int numSpeakers) noexcept
{
    if (requested == SpatialOutputProfile::Auto)
    {
        if (numOutputChannels >= 13)
            return { SpatialOutputProfile::Surround742, SpatialProfileStage::Direct };
        if (numOutputChannels >= 10)
            return { SpatialOutputProfile::Surround721, SpatialProfileStage::Direct };
        if (numOutputChannels >= 8)
            return { SpatialOutputProfile::Surround521, SpatialProfileStage::Direct };
        if (numOutputChannels >= numSpeakers)
            return { SpatialOutputProfile::Quad40, SpatialProfileStage::Direct };
        return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };
    }

    switch (requested)
    {
        case SpatialOutputProfile::Surround742:
            if (numOutputChannels >= 13)
                return { requested, SpatialProfileStage::Direct };
            if (numOutputChannels >= numSpeakers)
                return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
            return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

        case SpatialOutputProfile::Surround721:
        case SpatialOutputProfile::AtmosBed:
            if (numOutputChannels >= 10)
                return { requested == SpatialOutputProfile::AtmosBed ? SpatialOutputProfile::AtmosBed
                                                                     : SpatialOutputProfile::Surround721,
                         SpatialProfileStage::Direct };
            if (numOutputChannels >= numSpeakers)
                return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
            return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

        case SpatialOutputProfile::Surround521:
            if (numOutputChannels >= 8)
                return { requested, SpatialProfileStage::Direct };
            if (numOutputChannels >= numSpeakers)
                return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
            return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

        case SpatialOutputProfile::CodecIAMF:
        case SpatialOutputProfile::CodecADM:
            if (numOutputChannels >= 13)
                return { SpatialOutputProfile::Surround742, SpatialProfileStage::CodecLayoutPlaceholder };
            if (numOutputChannels >= numSpeakers)
                return { SpatialOutputProfile::Quad40, SpatialProfileStage::FallbackQuad };
            return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

        case SpatialOutputProfile::AmbisonicHOA:
            if (numOutputChannels >= 16)
                return { requested, SpatialProfileStage::Direct };
            if (numOutputChannels >= 4)
                return { SpatialOutputProfile::AmbisonicFOA, SpatialProfileStage::FallbackQuad };
            return { SpatialOutputProfile::AmbisonicFOA, SpatialProfileStage::AmbiDecodeStereo };

        case SpatialOutputProfile::AmbisonicFOA:
            if (numOutputChannels >= 4)
                return { requested, SpatialProfileStage::Direct };
            return { requested, SpatialProfileStage::AmbiDecodeStereo };

        case SpatialOutputProfile::Quad40:
            if (numOutputChannels >= numSpeakers)
                return { requested, SpatialProfileStage::Direct };
            return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };

        case SpatialOutputProfile::Stereo20:
        case SpatialOutputProfile::Virtual3dStereo:
            return { requested, SpatialProfileStage::Direct };

        case SpatialOutputProfile::Auto:
        default:
            break;
    }

    return { SpatialOutputProfile::Stereo20, SpatialProfileStage::FallbackStereo };
}

inline int ambisonicOrderForProfile (SpatialOutputProfile profile) noexcept
{
    switch (profile)
    {
        case SpatialOutputProfile::AmbisonicFOA: return 1;
        case SpatialOutputProfile::AmbisonicHOA: return 3;
        default: break;
    }

    return 0;
}

inline void encodeAmbisonicFoaProxyFromQuad (float fl, float fr, float rr, float rl,
                                             float& w, float& x, float& y, float& z) noexcept
{
    const float sum = fl + fr + rr + rl;
    w = 0.35355339f * sum; // SN3D-style proxy
    x = 0.5f * ((fr + rr) - (fl + rl));
    y = 0.5f * ((fl + fr) - (rl + rr));
    z = 0.0f; // No elevation energy in quad bed proxy.
}

inline void decodeAmbisonicFoaProxyToStereo (float w, float x, float y, float z,
                                             float& left, float& right) noexcept
{
    left = 0.70710678f * w - 0.50f * x + 0.22f * y + 0.08f * z;
    right = 0.70710678f * w + 0.50f * x + 0.22f * y + 0.08f * z;
}

inline void writeSurround521Sample (juce::AudioBuffer<float>& outputBuffer,
                                    int sampleIndex,
                                    float masterGain,
                                    float fl,
                                    float fr,
                                    float rr,
                                    float rl) noexcept
{
    const float bed = (fl + fr + rr + rl) * 0.25f;

    outputBuffer.setSample (0, sampleIndex, fl * masterGain);
    outputBuffer.setSample (1, sampleIndex, fr * masterGain);
    outputBuffer.setSample (2, sampleIndex, (fl + fr) * 0.70710678f * masterGain);
    outputBuffer.setSample (3, sampleIndex, bed * 0.35f * masterGain);
    outputBuffer.setSample (4, sampleIndex, bed * 0.35f * masterGain);
    outputBuffer.setSample (5, sampleIndex, rl * masterGain);
    outputBuffer.setSample (6, sampleIndex, rr * masterGain);
    outputBuffer.setSample (7, sampleIndex, bed * 0.8f * masterGain);
}

inline void writeSurround721Sample (juce::AudioBuffer<float>& outputBuffer,
                                    int sampleIndex,
                                    float masterGain,
                                    float fl,
                                    float fr,
                                    float rr,
                                    float rl) noexcept
{
    const float bed = (fl + fr + rr + rl) * 0.25f;
    const float lrs = (0.72f * rl) + (0.28f * fl);
    const float rrs = (0.72f * rr) + (0.28f * fr);

    outputBuffer.setSample (0, sampleIndex, fl * masterGain);
    outputBuffer.setSample (1, sampleIndex, fr * masterGain);
    outputBuffer.setSample (2, sampleIndex, (fl + fr) * 0.70710678f * masterGain);
    outputBuffer.setSample (3, sampleIndex, bed * 0.33f * masterGain);
    outputBuffer.setSample (4, sampleIndex, bed * 0.33f * masterGain);
    outputBuffer.setSample (5, sampleIndex, rl * masterGain);
    outputBuffer.setSample (6, sampleIndex, rr * masterGain);
    outputBuffer.setSample (7, sampleIndex, lrs * masterGain);
    outputBuffer.setSample (8, sampleIndex, rrs * masterGain);
    outputBuffer.setSample (9, sampleIndex, bed * 0.8f * masterGain);
}

inline void writeSurround742Sample (juce::AudioBuffer<float>& outputBuffer,
                                    int sampleIndex,
                                    float masterGain,
                                    float fl,
                                    float fr,
                                    float rr,
                                    float rl) noexcept
{
    const float bed = (fl + fr + rr + rl) * 0.25f;
    const float lrs = (0.72f * rl) + (0.28f * fl);
    const float rrs = (0.72f * rr) + (0.28f * fr);
    const float topFl = (0.70f * fl) + (0.25f * rl);
    const float topFr = (0.70f * fr) + (0.25f * rr);
    const float topRl = (0.78f * rl) + (0.12f * fl);
    const float topRr = (0.78f * rr) + (0.12f * fr);

    outputBuffer.setSample (0, sampleIndex, fl * masterGain);
    outputBuffer.setSample (1, sampleIndex, fr * masterGain);
    outputBuffer.setSample (2, sampleIndex, (fl + fr) * 0.70710678f * masterGain);
    outputBuffer.setSample (3, sampleIndex, bed * 0.30f * masterGain);
    outputBuffer.setSample (4, sampleIndex, bed * 0.30f * masterGain);
    outputBuffer.setSample (5, sampleIndex, rl * masterGain);
    outputBuffer.setSample (6, sampleIndex, rr * masterGain);
    outputBuffer.setSample (7, sampleIndex, lrs * masterGain);
    outputBuffer.setSample (8, sampleIndex, rrs * masterGain);
    outputBuffer.setSample (9, sampleIndex, topFl * masterGain);
    outputBuffer.setSample (10, sampleIndex, topFr * masterGain);
    outputBuffer.setSample (11, sampleIndex, topRl * masterGain);
    outputBuffer.setSample (12, sampleIndex, topRr * masterGain);
}

} // namespace locusq::spatial_profile_router
