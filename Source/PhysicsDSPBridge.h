#pragma once

#include "SceneGraph.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

//==============================================================================
/**
 * PerEmitterDSPValues - normalized DSP modulation values for one emitter.
 *
 * All fields are in [0..1] after the bridge pipeline:
 *   normalize(domain) → deadband → curve → smooth → clamp(0..1) → publish
 *
 * Audio thread reads these values and adds them as offsets to the base
 * DSP parameters (spread, gain). The bridge never writes to APVTS directly.
 *
 * Zero default: no modulation applied when no physics source is active.
 */
struct PerEmitterDSPValues
{
    float spreadMod     = 0.0f;   // additive spread offset [0..1]
    float gainMod       = 0.0f;   // additive gain modulation [0..1]
    float gainTransient = 0.0f;   // one-shot transient burst [0..1], decays per tick
};

//==============================================================================
/**
 * PhysicsDSPBridge - publishes physics-driven DSP modulation values to the
 * audio thread with lock-free double-buffer per emitter.
 *
 * Writer: PhysicsWorker thread calls publish() after each simulation tick.
 * Reader: processBlock() calls read() — allocation-free, lock-free.
 *
 * Pipeline stages applied by publish() before storing:
 *   1. normalize    — map raw physics value to [0..1] based on declared domain
 *   2. deadband     — zero outputs below a configurable threshold
 *   3. curve        — apply shaping function (linear / log / sigmoid)
 *   4. smooth       — one-pole IIR attack/release per field
 *   5. clamp        — hard clamp to [0..1]; guards against finite-contract violation
 *
 * Stall contract: if the worker stops publishing (missed ticks > 2), the audio
 * thread will continue to see the last published values — effectively frozen.
 * Callers may check getTickCount() vs a saved snapshot to detect staleness.
 *
 * Phase P1: all inputs are zero (subsystems not yet wired). The pipeline,
 * smoothing state, and atomic publication are exercised with identity inputs
 * so the audio thread path is stable before subsystems arrive in P2–P6.
 */
class PhysicsDSPBridge
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Configuration — set from UI/APVTS thread before first tick

    struct SmoothConfig
    {
        float attackSec  = 0.005f;   // spread/gain attack  (5 ms default)
        float releaseSec = 0.020f;   // spread/gain release (20 ms default)
        float transientDecayHz = 8.0f; // gain transient decay rate (Hz)
    };

    void setSmoothConfig (const SmoothConfig& cfg)
    {
        smoothCfg = cfg;
    }

    void prepare (double sampleRate, double workerPeriodSec)
    {
        // Compute one-pole coefficients from smooth config.
        // These are used on the worker thread per-tick, so workerPeriodSec
        // is the correct time base (not audio sample rate).
        computeCoefficients (workerPeriodSec);
        (void) sampleRate; // reserved for future per-sample bridge path
    }

    //==========================================================================
    // Writer API — worker thread only

    /**
     * Publish a new set of DSP values for one emitter.
     * Applies deadband, curve, smooth, and clamp before storing.
     *
     * @param index      Emitter slot index [0, kMaxEmitters)
     * @param raw        Pre-normalized raw values (already in [0..1] domain)
     */
    void publish (int index, const PerEmitterDSPValues& raw)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        auto& s = smoothState[static_cast<std::size_t> (index)];

        // --- Deadband ---
        const float spreadIn    = applyDeadband (raw.spreadMod,     kDeadband);
        const float gainIn      = applyDeadband (raw.gainMod,       kDeadband);
        const float transientIn = applyDeadband (raw.gainTransient, kDeadband);

        // --- Curve (linear pass-through in P1; subsystems set curve type in P2+) ---
        const float spreadCurved    = spreadIn;
        const float gainCurved      = gainIn;
        const float transientCurved = transientIn;

        // --- Smooth (one-pole attack/release) ---
        s.spreadSmoothed     = onePoleSmoothStep (s.spreadSmoothed,     spreadCurved,    s.coefAttack, s.coefRelease);
        s.gainSmoothed       = onePoleSmoothStep (s.gainSmoothed,       gainCurved,      s.coefAttack, s.coefRelease);
        s.transientSmoothed  = onePoleSmoothStep (s.transientSmoothed,  transientCurved, s.coefAttack, s.coefRelease);

        // --- Clamp (finite-contract guard) ---
        PerEmitterDSPValues out;
        out.spreadMod     = clampFinite (s.spreadSmoothed);
        out.gainMod       = clampFinite (s.gainSmoothed);
        out.gainTransient = clampFinite (s.transientSmoothed);

        writeSlot (index, out);
        tickCount.fetch_add (1, std::memory_order_release);
    }

    /** Publish zero values for a slot — resets smoothing state to zero. */
    void publishZero (int index)
    {
        if (index < 0 || index >= kMaxEmitters)
            return;

        auto& s = smoothState[static_cast<std::size_t> (index)];
        s.spreadSmoothed    = 0.0f;
        s.gainSmoothed      = 0.0f;
        s.transientSmoothed = 0.0f;
        writeSlot (index, {});
    }

    //==========================================================================
    // Reader API — audio thread (lock-free, allocation-free)

    PerEmitterDSPValues read (int index) const
    {
        if (index < 0 || index >= kMaxEmitters)
            return {};

        const auto& slot = slots[static_cast<std::size_t> (index)];
        return slot.buffers[slot.readIndex.load (std::memory_order_acquire)];
    }

    /** Monotonically increasing publish counter.
     *  Audio thread saves a snapshot and compares on successive calls.
     *  If unchanged for >2 expected tick periods the worker has stalled;
     *  callers should hold the last read values (already the default behaviour). */
    std::uint64_t getTickCount() const
    {
        return tickCount.load (std::memory_order_acquire);
    }

private:
    //==========================================================================
    struct SmoothState
    {
        float spreadSmoothed    = 0.0f;
        float gainSmoothed      = 0.0f;
        float transientSmoothed = 0.0f;

        float coefAttack  = 0.0f;
        float coefRelease = 0.0f;
    };

    struct BridgeSlot
    {
        std::array<PerEmitterDSPValues, 2> buffers {};
        std::atomic<int> readIndex { 0 };
    };

    //==========================================================================
    void computeCoefficients (double periodSec)
    {
        // One-pole IIR coefficients: coef = 1 - exp(-periodSec / timeSec)
        // Larger coef → faster tracking; coef=1 → instantaneous.
        const float period = static_cast<float> (std::max (1.0e-6, periodSec));

        for (auto& s : smoothState)
        {
            s.coefAttack  = onePoleCoefficientFromTime (smoothCfg.attackSec,  period);
            s.coefRelease = onePoleCoefficientFromTime (smoothCfg.releaseSec, period);
        }
    }

    static float onePoleCoefficientFromTime (float timeSec, float periodSec) noexcept
    {
        if (timeSec < 1.0e-6f) return 1.0f; // instantaneous
        return 1.0f - std::exp (-periodSec / timeSec);
    }

    static float onePoleSmoothStep (float current, float target,
                                    float coefAttack, float coefRelease) noexcept
    {
        const float coef = (target > current) ? coefAttack : coefRelease;
        return current + coef * (target - current);
    }

    static float applyDeadband (float value, float threshold) noexcept
    {
        return (value < threshold) ? 0.0f : value;
    }

    static float clampFinite (float value) noexcept
    {
        if (! std::isfinite (value)) return 0.0f;
        return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    }

    void writeSlot (int index, const PerEmitterDSPValues& values)
    {
        auto& slot = slots[static_cast<std::size_t> (index)];
        const int writeIdx = 1 - slot.readIndex.load (std::memory_order_acquire);
        slot.buffers[static_cast<std::size_t> (writeIdx)] = values;
        slot.readIndex.store (writeIdx, std::memory_order_release);
    }

    //==========================================================================
    static constexpr float kDeadband = 0.001f;

    std::array<BridgeSlot,   kMaxEmitters> slots {};
    std::array<SmoothState,  kMaxEmitters> smoothState {};

    std::atomic<std::uint64_t> tickCount { 0 };

    SmoothConfig smoothCfg {};
};
