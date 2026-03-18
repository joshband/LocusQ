#pragma once

#include "SceneGraph.h"

#include <atomic>
#include <cmath>
#include <cstdint>

//==============================================================================
/**
 * TurbulenceSystem — per-emitter stochastic force injection.
 *
 * Each emitter gets an independent LCG noise seed filtered through a
 * one-pole IIR smoother at phys_turbulence_rate Hz. Low rate produces
 * coherent spatial drift; high rate approaches white-noise jitter.
 *
 * Bounded: max impulse magnitude = phys_turbulence × mass × 9.8 m/s².
 * Additive on top of all deterministic forces (spring, attractor, gravity).
 *
 * DSP hook (P3):
 *   getMagnitude(i) → [0..1] for spread jitter additive contribution.
 *   0 = no turbulence active; 1 = full-amplitude noise.
 *
 * Acceptance gate (P3):
 *   Max impulse = turbulence × mass × 9.8, verified at boundary values.
 *
 * Threading contract:
 *   - Setters: APVTS / UI thread (atomic write).
 *   - computeForce(), getMagnitude(): worker thread only.
 */
class TurbulenceSystem
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Setters — APVTS / UI thread

    void setAmplitude (float a)  { amplitude.store  (juce::jlimit (0.0f, 1.0f, a), std::memory_order_release); }
    void setRate      (float hz) { filterRate.store (juce::jmax   (0.01f, hz),     std::memory_order_release); }

    float getAmplitude() const { return amplitude.load (std::memory_order_acquire); }

    //==========================================================================
    // Worker-thread API

    /**
     * Compute turbulence force for one emitter and update its noise state.
     *
     * @param emitterIdx  Slot index [0, kMaxEmitters)
     * @param mass        Emitter mass (scales force magnitude)
     * @param dt          Tick period in seconds
     * @return            Stochastic force vector
     */
    Vec3 computeForce (int emitterIdx, float mass, float dt) noexcept
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters)
            return {};

        const float amp = amplitude.load (std::memory_order_acquire);
        if (amp < 1.0e-6f)
            return {};

        auto& s = noiseState[static_cast<std::size_t> (emitterIdx)];

        // Lazy seed initialisation — unique sequence per emitter slot
        if (s.seed == 0)
            s.seed = static_cast<uint32_t> (emitterIdx + 1) * 2654435761u;

        // One-pole IIR coefficient: coef = 1 - exp(-dt * rateHz)
        const float rateHz = filterRate.load (std::memory_order_acquire);
        const float coef   = 1.0f - std::exp (-dt * rateHz);

        // Generate raw noise in [-1..1] per axis via LCG
        const float rawX = lcgNext (s.seed);
        const float rawY = lcgNext (s.seed);
        const float rawZ = lcgNext (s.seed);

        // Apply one-pole IIR smoothing (produces coherent drift at low rate)
        s.filtered.x += coef * (rawX - s.filtered.x);
        s.filtered.y += coef * (rawY - s.filtered.y);
        s.filtered.z += coef * (rawZ - s.filtered.z);

        // Scale: max impulse magnitude = amp * mass * 9.8 m/s²
        const float scale = amp * std::max (mass, 0.001f) * 9.8f;
        return { s.filtered.x * scale, s.filtered.y * scale, s.filtered.z * scale };
    }

    /**
     * Normalised turbulence magnitude for DSP spread mapping.
     * Returns [0..1]: 0 = no activity, approaches 1 at full amplitude noise.
     * Must be called after computeForce() to reflect current-tick state.
     */
    float getMagnitude (int emitterIdx) const noexcept
    {
        if (emitterIdx < 0 || emitterIdx >= kMaxEmitters)
            return 0.0f;

        const float amp = amplitude.load (std::memory_order_acquire);
        if (amp < 1.0e-6f)
            return 0.0f;

        const auto& s = noiseState[static_cast<std::size_t> (emitterIdx)];
        const float len = std::sqrt (s.filtered.x * s.filtered.x
                                   + s.filtered.y * s.filtered.y
                                   + s.filtered.z * s.filtered.z);

        // Normalise: max 3D length when each component is ±1 is sqrt(3) ≈ 1.732
        // Multiply by amp so getMagnitude → 0 when amp=0
        return juce::jlimit (0.0f, 1.0f, len * amp * kInvSqrt3);
    }

private:
    // 1 / sqrt(3) — normalises max 3D unit-component vector length to 1
    static constexpr float kInvSqrt3 = 0.577350269f;

    // Knuth multiplicative LCG: maps uint32 state to float in [-1, 1)
    static float lcgNext (uint32_t& seed) noexcept
    {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float> (static_cast<int32_t> (seed)) * (1.0f / 2147483648.0f);
    }

    struct NoiseState
    {
        uint32_t seed    = 0;
        Vec3     filtered { 0.0f, 0.0f, 0.0f };
    };

    std::atomic<float> amplitude  { 0.0f };
    std::atomic<float> filterRate { 2.0f };  // Hz

    // Per-emitter noise state (worker-thread-local, no atomics needed)
    NoiseState noiseState[kMaxEmitters] {};
};
