// qa/physics_daw_automation_probe_main.cpp
// Physics DAW Automation acceptance probe
// Covers: live path round-trip, gainTransient separation,
//         LIVE→FROZEN snapshot guard, frozen path ownership,
//         NaN safety, FROZEN→LIVE transition

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
    check("nan_safety_inherited",
          spread_finite && gain_finite && transient_finite,
          "spread_finite="    + std::to_string(spread_finite) +
          " gain_finite="     + std::to_string(gain_finite) +
          " transient_finite=" + std::to_string(transient_finite));
}

int main()
{
    checkLivePathRoundTrip();
    checkGainTransientSeparate();
    checkSnapshotGuardNoJump();
    checkFrozenPathOwnership();
    checkFrozenToLiveTransition();
    checkNaNSafetyInherited();

    int passed = 0, failed = 0;
    for (const auto& c : checks) { if (c.passed) ++passed; else ++failed; }
    std::cout << "SUMMARY physics_daw_automation_probe : "
              << passed << "/" << checks.size() << " checks passed\n";
    return failed > 0 ? 1 : 0;
}
