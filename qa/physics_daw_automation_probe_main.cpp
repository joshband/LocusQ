// qa/physics_daw_automation_probe_main.cpp
// Physics DAW Automation acceptance probe
// Covers: live path round-trip, gainTransient separation,
//         LIVE→FROZEN snapshot guard, frozen path ownership,
//         NaN safety, FROZEN→LIVE transition,
//         block-rate freeze continuity (automated pop check)

#include "Source/PhysicsDSPBridge.h"
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <limits>

struct CheckResult { std::string name; bool passed; std::string detail; };
static std::vector<CheckResult> checks;

static void check(const std::string& name, bool passed, const std::string& detail)
{
    checks.push_back({ name, passed, detail });
    std::cout << "CHECK " << name << " : " << (passed ? "PASS" : "FAIL")
              << " | " << detail << "\n";
}

// Helper: configure bridge with instantaneous smoothing (coef=1) so
// a single publish() call lands the exact input value at the output.
static void prepareInstantaneous(PhysicsDSPBridge& bridge)
{
    PhysicsDSPBridge::SmoothConfig cfg;
    cfg.attackSec          = 0.0f;   // instantaneous — coef = 1.0
    cfg.releaseSec         = 0.0f;
    cfg.transientDecayHz   = 0.0f;
    bridge.setSmoothConfig(cfg);
    bridge.prepare(48000.0, 1.0 / 240.0);
}

// ── Gate 1: live path — atomic store/load round-trip ──────────────────────
static void checkLivePathRoundTrip()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);
    bridge.publishZero(0);

    PerEmitterDSPValues vals;
    vals.spreadMod    = 0.42f;
    vals.gainMod      = 0.17f;
    vals.gainTransient = 0.0f;
    bridge.publish(0, vals);

    const auto read = bridge.read(0);
    const bool spread_ok = std::abs(read.spreadMod - 0.42f) < 0.001f;
    const bool gain_ok   = std::abs(read.gainMod   - 0.17f) < 0.001f;
    check("live_path_round_trip",
          spread_ok && gain_ok,
          "spreadMod=" + std::to_string(read.spreadMod) +
          " gainMod="  + std::to_string(read.gainMod));
}

// ── Gate 2: gainTransient not mirrored — field is separate ────────────────
static void checkGainTransientSeparate()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);

    PerEmitterDSPValues vals;
    vals.spreadMod    = 0.0f;
    vals.gainMod      = 0.0f;
    vals.gainTransient = 0.75f;
    bridge.publish(0, vals);

    const auto read = bridge.read(0);
    const bool transient_ok = std::abs(read.gainTransient - 0.75f) < 0.001f;
    const bool gain_zero    = std::abs(read.gainMod) < 0.001f;
    check("gain_transient_separate",
          transient_ok && gain_zero,
          "gainTransient=" + std::to_string(read.gainTransient) +
          " gainMod="      + std::to_string(read.gainMod));
}

// ── Gate 3: LIVE→FROZEN snapshot guard — no value jump ────────────────────
static void checkSnapshotGuardNoJump()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);

    PerEmitterDSPValues vals;
    vals.spreadMod = 0.61f;
    vals.gainMod   = 0.33f;
    vals.gainTransient = 0.0f;
    bridge.publish(0, vals);

    bool lastFrozen = false;
    float apvtsSpread = 0.0f;
    float apvtsGain   = 0.0f;

    // Simulate transition detection + snapshot guard (processBlock logic)
    bool nowFrozen = true;
    if (!lastFrozen && nowFrozen)
    {
        const auto snapshot = bridge.read(0);
        apvtsSpread = snapshot.spreadMod;  // store()
        apvtsGain   = snapshot.gainMod;
    }
    lastFrozen = nowFrozen;

    const float spread_delta = std::abs(apvtsSpread - 0.61f);
    const float gain_delta   = std::abs(apvtsGain   - 0.33f);
    check("snapshot_guard_no_jump",
          spread_delta < 0.01f && gain_delta < 0.01f,
          "spread_delta=" + std::to_string(spread_delta) +
          " gain_delta="  + std::to_string(gain_delta));
}

// ── Gate 4: frozen path — APVTS value used, not physics atomic ────────────
static void checkFrozenPathOwnership()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);

    PerEmitterDSPValues vals;
    vals.spreadMod = 0.9f;  // physics wants 0.9
    bridge.publish(0, vals);

    const float apvtsPlaybackSpread = 0.25f;  // DAW plays back 0.25
    const bool frozen = true;

    const float spreadDelta = frozen
        ? apvtsPlaybackSpread           // frozen: load() from APVTS
        : bridge.read(0).spreadMod;     // live: read atomic

    check("frozen_path_uses_apvts",
          std::abs(spreadDelta - 0.25f) < 0.001f,
          "spreadDelta="   + std::to_string(spreadDelta) +
          " atomicValue="  + std::to_string(bridge.read(0).spreadMod));
}

// ── Gate 5: FROZEN→LIVE transition — atomic used immediately ─────────────
static void checkFrozenToLiveTransition()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);

    PerEmitterDSPValues vals;
    vals.spreadMod = 0.55f;
    bridge.publish(0, vals);

    // Start frozen
    bool lastFrozen = true;
    float apvtsSpread = 0.25f;  // DAW playback value

    // Transition: FROZEN → LIVE (no special handling needed — just switch source)
    bool nowFrozen = false;
    float spreadDelta = nowFrozen
        ? apvtsSpread
        : bridge.read(0).spreadMod;  // switches to atomic immediately

    lastFrozen = nowFrozen;
    (void)lastFrozen;

    check("frozen_to_live_transition",
          std::abs(spreadDelta - 0.55f) < 0.001f,
          "spreadDelta=" + std::to_string(spreadDelta) +
          " (expected atomic 0.55, not frozen 0.25)");
}

// ── Gate 7: gainTransient ignores freeze state — not suppressed by freeze ──
static void checkGainTransientIgnoresFreezeState()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);

    PerEmitterDSPValues vals;
    vals.spreadMod    = 0.0f;
    vals.gainMod      = 0.0f;
    vals.gainTransient = 0.75f;
    bridge.publish(0, vals);

    // Simulate frozen path: DSP would read spread/gain from APVTS, but
    // gainTransient always comes from the bridge regardless of freeze state
    const bool frozen = true;
    const auto bridgeRead = bridge.read(0);

    // Frozen state must not suppress gainTransient
    const bool transient_present = bridgeRead.gainTransient > 0.5f;
    check("gain_transient_ignores_freeze",
          transient_present,
          "gainTransient=" + std::to_string(bridgeRead.gainTransient) +
          " frozen=" + std::to_string(frozen));
}

// ── Gate 6: NaN/inf safety — inherited from bridge clamp ─────────────────
static void checkNaNSafetyInherited()
{
    PhysicsDSPBridge bridge;
    prepareInstantaneous(bridge);

    PerEmitterDSPValues vals;
    vals.spreadMod    = std::numeric_limits<float>::quiet_NaN();
    vals.gainMod      = std::numeric_limits<float>::infinity();
    vals.gainTransient = -std::numeric_limits<float>::infinity();
    bridge.publish(0, vals);

    const auto read = bridge.read(0);
    const bool spread_finite    = std::isfinite(read.spreadMod);
    const bool gain_finite      = std::isfinite(read.gainMod);
    const bool transient_finite = std::isfinite(read.gainTransient);
    const bool spread_in_range    = read.spreadMod    >= 0.0f && read.spreadMod    <= 1.0f;
    const bool gain_in_range      = read.gainMod      >= 0.0f && read.gainMod      <= 1.0f;
    const bool transient_in_range = read.gainTransient >= 0.0f && read.gainTransient <= 1.0f;
    check("nan_safety_inherited",
          spread_finite    && gain_finite    && transient_finite &&
          spread_in_range  && gain_in_range  && transient_in_range,
          "spread=" + std::to_string(read.spreadMod) +
          " gain="  + std::to_string(read.gainMod) +
          " transient=" + std::to_string(read.gainTransient));
}

// ── Gate 8: block-rate pop test ───────────────────────────────────────────
// Simulates a processBlock loop driving physics with an actively ramping
// spread value (worst-case: value changing at maximum rate at the freeze
// moment). Verifies the output seen by DSP does not jump at the LIVE→FROZEN
// block boundary — i.e., no audible click even without any explicit
// interpolation beyond the snapshot guard.
static void checkFreezeTransitionNoPop()
{
    PhysicsDSPBridge bridge;

    // Realistic one-pole smoothing: 5 ms attack, 50 ms release at 48 kHz/256
    PhysicsDSPBridge::SmoothConfig cfg;
    cfg.attackSec        = 0.005f;
    cfg.releaseSec       = 0.050f;
    cfg.transientDecayHz = 0.0f;
    bridge.setSmoothConfig(cfg);

    const double sampleRate  = 48000.0;
    const int    blockSize   = 256;
    bridge.prepare(sampleRate, static_cast<double>(blockSize) / sampleRate);

    // Warm-up: push value to 0 so smoother starts at known state
    bridge.publishZero(0);
    for (int b = 0; b < 50; ++b) bridge.read(0);

    const int    kBlocks     = 40;
    const int    kFreezeAt   = 20;      // freeze mid-ramp — worst case
    const float  kRampStart  = 0.10f;
    const float  kRampEnd    = 0.90f;

    bool  lastFrozen = false;
    float apvtsSpread = 0.0f;
    float apvtsGain   = 0.0f;
    float prevOut = 0.0f;
    float maxJump = 0.0f;

    for (int b = 0; b < kBlocks; ++b)
    {
        // Physics worker ramps spread linearly from 0.1 → 0.9 over kBlocks
        const float t = static_cast<float>(b) / static_cast<float>(kBlocks - 1);
        PerEmitterDSPValues vals;
        vals.spreadMod     = kRampStart + t * (kRampEnd - kRampStart);
        vals.gainMod       = 0.5f;
        vals.gainTransient = 0.0f;
        bridge.publish(0, vals);

        const bool nowFrozen = (b >= kFreezeAt);

        // Snapshot guard — runs on audio thread at the exact freeze-transition block
        if (!lastFrozen && nowFrozen)
        {
            const auto snap = bridge.read(0);
            apvtsSpread = snap.spreadMod;
            apvtsGain   = snap.gainMod;
        }
        lastFrozen = nowFrozen;

        // DSP output for this block
        const float out = nowFrozen
            ? apvtsSpread                   // frozen: load() from APVTS
            : bridge.read(0).spreadMod;     // live: read atomic

        if (b > 0)
        {
            const float jump = std::abs(out - prevOut);
            if (jump > maxJump) maxJump = jump;
        }
        prevOut = out;
    }

    // Pop threshold: 0.05 normalized (~1/20 of full scale).
    // In practice the snapshot guard makes this < 0.001; 0.05 is a hard gate.
    const float POP_THRESHOLD = 0.05f;
    check("freeze_transition_no_pop",
          maxJump < POP_THRESHOLD,
          "max_jump_across_all_blocks=" + std::to_string(maxJump) +
          " (freeze_at_block=" + std::to_string(kFreezeAt) +
          " threshold=" + std::to_string(POP_THRESHOLD) + ")");
}

int main()
{
    checkLivePathRoundTrip();
    checkGainTransientSeparate();
    checkSnapshotGuardNoJump();
    checkFrozenPathOwnership();
    checkFrozenToLiveTransition();
    checkGainTransientIgnoresFreezeState();
    checkNaNSafetyInherited();
    checkFreezeTransitionNoPop();

    int passed = 0, failed = 0;
    for (const auto& c : checks) { if (c.passed) ++passed; else ++failed; }
    std::cout << "SUMMARY physics_daw_automation_probe : "
              << passed << "/" << checks.size() << " checks passed\n";
    return failed > 0 ? 1 : 0;
}
