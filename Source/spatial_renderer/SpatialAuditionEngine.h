#pragma once

#include "SpatialAuditionPrimitives.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <cstdint>

namespace locusq::spatial_audition_engine
{
struct VoiceExcitationState
{
    double phaseA = 0.0;
    double phaseB = 0.0;
    float env = 0.0f;
    int cooldownSamples = 0;
    std::uint32_t noiseState = 0u;
};

struct VoiceExcitationInput
{
    int voiceIndex = 0;
    int activeVoices = 1;
    int signalTypeIndex = 0;
    bool qualityHigh = false;
    float delayedSample = 0.0f;
    double sampleRate = 48000.0;
};

inline float nextVoiceWhiteNoise (std::uint32_t& state) noexcept
{
    state = state * 1664525u + 1013904223u;
    const auto scrambled = state ^ (state >> 11) ^ (state << 7);
    return static_cast<float> (scrambled & 0x00FFFFFFu) / 8388608.0f - 1.0f;
}

inline float advanceSine (double& phase, double frequencyHz, double sampleRate) noexcept
{
    const auto safeSampleRate = juce::jmax (1.0, sampleRate);
    phase += frequencyHz / safeSampleRate;
    phase -= std::floor (phase);
    return static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * phase));
}

inline float renderVoiceExcitation (
    VoiceExcitationState& state,
    const VoiceExcitationInput& input) noexcept
{
    if (input.activeVoices <= 1
        || ! spatial_audition_primitives::isMultiSourceSignal (input.signalTypeIndex))
    {
        return input.delayedSample;
    }

    const auto hashA = spatial_audition_primitives::voiceHashUnit (input.voiceIndex, 0xB5297A4Du);
    const auto hashB = spatial_audition_primitives::voiceHashUnit (input.voiceIndex, 0x68E31DA4u);
    const auto sampleRate = juce::jmax (1.0, input.sampleRate);

    if (state.cooldownSamples > 0)
        --state.cooldownSamples;

    switch (input.signalTypeIndex)
    {
        case 3: // rain
        {
            const auto toneA = advanceSine (state.phaseA, 740.0 + 2420.0 * (0.25 + 0.75 * hashA), sampleRate);
            const auto toneB = advanceSine (state.phaseB, 520.0 + 1560.0 * (0.30 + 0.70 * hashB), sampleRate);
            const auto triggerGate = input.qualityHigh ? 0.84f : 0.89f;
            const auto voiceNoise = nextVoiceWhiteNoise (state.noiseState);
            if (state.cooldownSamples <= 0 && voiceNoise > triggerGate)
            {
                const auto dropletPulse = 0.5f + 0.5f * toneB;
                state.env = juce::jmax (state.env, 0.24f + 0.72f * dropletPulse);
                const auto cooldownSeconds = 0.010f + 0.024f * hashB;
                state.cooldownSamples = static_cast<int> (std::round (cooldownSeconds * static_cast<float> (sampleRate)));
            }

            state.env *= input.qualityHigh ? 0.9939f : 0.9920f;
            const auto sparkle = toneA * std::abs (toneA);
            const auto droplet = (0.66f * toneA + 0.34f * sparkle) * state.env;
            const auto mist = 0.09f * voiceNoise * (0.35f + 0.65f * state.env);
            return juce::jlimit (-2.0f, 2.0f, 0.76f * input.delayedSample + 0.24f * droplet + mist);
        }
        case 4: // snow
        {
            const auto drift = advanceSine (state.phaseA, 72.0 + 120.0 * hashA, sampleRate);
            const auto flutter = advanceSine (state.phaseB, 0.38 + 0.44 * hashB, sampleRate);
            const auto frostNoise = nextVoiceWhiteNoise (state.noiseState)
                * (0.26f + 0.74f * (0.5f + 0.5f * flutter));
            const auto veil = 0.88f * input.delayedSample + 0.12f * frostNoise;
            const auto shimmer = 0.17f * drift * (0.45f + 0.55f * std::abs (flutter));
            return juce::jlimit (-2.0f, 2.0f, 0.84f * veil + shimmer);
        }
        case 5:  // bouncing balls
        case 10: // membrane drops
        {
            const auto triggerGate = (input.signalTypeIndex == 5)
                ? (input.qualityHigh ? 0.80f : 0.86f)
                : (input.qualityHigh ? 0.78f : 0.84f);
            const auto voiceNoise = nextVoiceWhiteNoise (state.noiseState);
            if (state.cooldownSamples <= 0 && voiceNoise > triggerGate)
            {
                state.env = juce::jmax (state.env, 0.52f + 0.42f * (0.5f + 0.5f * voiceNoise));
                const auto cooldownSeconds = (input.signalTypeIndex == 5 ? 0.040f : 0.055f)
                    + (input.signalTypeIndex == 5 ? 0.090f : 0.120f) * hashA;
                state.cooldownSamples = static_cast<int> (std::round (cooldownSeconds * static_cast<float> (sampleRate)));
            }

            state.env *= input.qualityHigh ? 0.9898f : 0.9868f;
            const auto modalA = advanceSine (state.phaseA, 130.0 + 410.0 * hashA + 170.0 * state.env, sampleRate);
            const auto modalB = advanceSine (state.phaseB, 208.0 + 500.0 * hashB, sampleRate);
            auto strikeEnv = state.env;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            const auto resonant = (0.70f * modalA + 0.30f * modalB) * state.env;
            const auto click = 0.24f * voiceNoise * strikeEnv;
            const auto blended = (input.signalTypeIndex == 5)
                ? (0.70f * input.delayedSample + 0.30f * resonant + click)
                : (0.76f * input.delayedSample + 0.24f * resonant + 0.16f * click);
            return juce::jlimit (-2.0f, 2.0f, blended);
        }
        case 6: // chimes
        {
            const auto voiceNoise = nextVoiceWhiteNoise (state.noiseState);
            if (state.cooldownSamples <= 0 && voiceNoise > (input.qualityHigh ? 0.88f : 0.92f))
            {
                state.env = juce::jmax (state.env, 0.48f + 0.48f * (0.5f + 0.5f * voiceNoise));
                const auto cooldownSeconds = (input.qualityHigh ? 0.11f : 0.15f) + 0.18f * hashA;
                state.cooldownSamples = static_cast<int> (std::round (cooldownSeconds * static_cast<float> (sampleRate)));
            }

            state.env *= input.qualityHigh ? 0.99934f : 0.99886f;
            const auto partialA = advanceSine (state.phaseA, 520.0 + 1080.0 * hashA, sampleRate);
            const auto partialB = advanceSine (state.phaseB, 780.0 + 1540.0 * hashB, sampleRate);
            const auto inharmonic = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * (state.phaseA * 1.618 + state.phaseB * 0.337)));
            auto strikeEnv = state.env;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            const auto resonant = (0.58f * partialA + 0.29f * partialB + 0.13f * inharmonic) * state.env;
            const auto strike = 0.18f * voiceNoise * strikeEnv;
            return juce::jlimit (-2.0f, 2.0f, 0.64f * input.delayedSample + 0.36f * resonant + strike);
        }
        default:
            return input.delayedSample;
    }
}

struct PhysicsReactiveState
{
    float timbreLowpassState = 0.0f;
};

struct PhysicsReactiveInput
{
    int signalTypeIndex = 0;
    float sample = 0.0f;
    float physicsVelocity = 0.0f;
    float physicsCollision = 0.0f;
    float physicsDensity = 0.0f;
    float motionEnergy = 0.0f;
};

inline float applyPhysicsReactiveTimbre (
    PhysicsReactiveState& state,
    const PhysicsReactiveInput& input) noexcept
{
    const auto couplingBlend = juce::jlimit (
        0.0f,
        1.0f,
        0.44f * input.physicsVelocity + 0.36f * input.physicsCollision + 0.20f * input.physicsDensity);
    if (couplingBlend <= 1.0e-5f)
        return input.sample;

    const auto lowpassAlpha = juce::jlimit (0.02f, 0.34f, 0.055f + 0.18f * input.physicsVelocity);
    state.timbreLowpassState += (input.sample - state.timbreLowpassState) * lowpassAlpha;
    const auto high = input.sample - state.timbreLowpassState;
    const auto transient = 1.0f + 0.85f * input.physicsCollision;
    const auto densityBody = 0.92f + 0.28f * input.physicsDensity;
    float shaped = input.sample;

    switch (input.signalTypeIndex)
    {
        case 3: shaped = input.sample * densityBody + high * (0.18f + 0.44f * input.physicsVelocity)
                            + high * (0.10f + 0.18f * input.motionEnergy) * transient; break; // rain
        case 4: shaped = input.sample * (0.94f + 0.24f * input.physicsDensity)
                            + high * (0.06f + 0.16f * input.physicsVelocity)
                            - high * (0.04f + 0.10f * input.physicsCollision); break; // snow
        case 5: shaped = std::tanh (input.sample * (1.0f + 0.56f * input.physicsCollision + 0.24f * input.physicsVelocity))
                            + high * (0.10f + 0.16f * input.physicsVelocity); break; // bouncing
        case 6: shaped = input.sample * (1.0f + 0.26f * input.physicsCollision)
                            + high * (0.14f + 0.26f * input.physicsVelocity + 0.08f * input.physicsDensity); break; // chimes
        default: shaped = input.sample * (0.95f + 0.20f * couplingBlend)
                            + high * (0.08f + 0.18f * input.physicsVelocity); break;
    }

    const auto wet = juce::jlimit (0.08f, 0.82f, 0.20f + 0.52f * couplingBlend + 0.10f * input.motionEnergy);
    return juce::jlimit (-2.0f, 2.0f, input.sample + (shaped - input.sample) * wet);
}
} // namespace locusq::spatial_audition_engine
