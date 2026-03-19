// qa/fir_truthfulness_probe_main.cpp
// BL-095 Slice C — FIR engine truthfulness objective probe
// Gates:
//   1. getLatencySamples() == 0 for short tap count (64)
//   2. getLatencySamples() == 0 for long tap count (512, above kDirectFirTapThreshold)
//   3. Identity impulse response → unit dirac input produces unit output at sample 0 (zero offset)
//   4. HeadphoneCalibrationChain.getActiveLatencySamples() == 0 when FIR engine is active
//   5. Known shifted impulse response → impulse offset matches tap index (FIR accuracy)
//   6. Long (512-tap) identity FIR passes dirac with zero offset

#include "Source/headphone_dsp/HeadphoneFirHook.h"
#include "Source/headphone_dsp/HeadphoneCalibrationChain.h"
#include "Source/headphone_core/HeadphoneCalibrationChainState.h"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace locusq::headphone_dsp;

struct CheckResult { std::string name; bool passed; std::string detail; };
static std::vector<CheckResult> checks;

static void check (const std::string& name, bool passed, const std::string& detail)
{
    checks.push_back ({ name, passed, detail });
    std::cout << "CHECK " << name << " : " << (passed ? "PASS" : "FAIL")
              << " | " << detail << "\n";
}

// ── Gate 1: latency reporting — short tap count ────────────────────────────
static void checkLatencyShortTaps()
{
    HeadphoneFirHook hook;
    hook.prepare (256, 64);
    const int lat = hook.getLatencySamples();
    check ("latency_short_taps",
           lat == 0,
           "tapThreshold=64 getLatencySamples()=" + std::to_string (lat));
}

// ── Gate 2: latency reporting — long tap count (above kDirectFirTapThreshold) ──
static void checkLatencyLongTaps()
{
    HeadphoneFirHook hook;
    hook.prepare (256, 512);   // 512 > kDirectFirTapThreshold (256)

    // Load a 512-tap impulse so the engine selection can't collapse to short-path.
    std::vector<float> ir (512, 0.0f);
    ir[0] = 1.0f;
    hook.loadImpulseResponse (ir.data(), static_cast<int> (ir.size()));

    const int lat = hook.getLatencySamples();
    check ("latency_long_taps",
           lat == 0,
           "tapCount=512 getLatencySamples()=" + std::to_string (lat));
}

// ── Gate 3: identity impulse → zero-offset dirac output ───────────────────
static void checkIdentityImpulseOffset()
{
    HeadphoneFirHook hook;
    hook.prepare (256);
    hook.setIdentityImpulse();
    hook.setBypassed (false);

    // Drain any residual state.
    for (int i = 0; i < 16; ++i)
    {
        float l = 0.0f, r = 0.0f;
        hook.processStereoSample (l, r);
    }

    // Inject a unit dirac.
    float left = 1.0f, right = 1.0f;
    hook.processStereoSample (left, right);
    const float outputAtSample0 = left;

    // With an identity IR and a fresh-state hook the dirac should appear immediately.
    const bool pass = std::abs (outputAtSample0 - 1.0f) < 1e-5f;
    check ("identity_impulse_zero_offset",
           pass,
           "outputAtSample0=" + std::to_string (outputAtSample0));
}

// ── Gate 4: HeadphoneCalibrationChain latency == 0 with FIR active ─────────
static void checkChainLatencyFirEngine()
{
    HeadphoneCalibrationChain chain;
    chain.prepare (48000.0, 256);
    chain.setEnabled (true);
    chain.setRequestedEngineIndex (
        static_cast<int> (locusq::headphone_core::CalibrationChainEngine::FirConvolution));

    // Load a short identity IR so the FIR engine is actually ready.
    std::vector<float> ir (64, 0.0f);
    ir[0] = 1.0f;
    chain.loadFirImpulseResponse (ir.data(), static_cast<int> (ir.size()));

    const int activeEngine = chain.getActiveEngineIndex();
    const int lat = chain.getActiveLatencySamples();
    const bool engineIsFir = activeEngine == static_cast<int> (
        locusq::headphone_core::CalibrationChainEngine::FirConvolution);

    check ("chain_latency_fir_engine",
           engineIsFir && lat == 0,
           "activeEngine=" + std::to_string (activeEngine)
               + " getActiveLatencySamples()=" + std::to_string (lat));
}

// ── Gate 5: known shifted IR → impulse offset matches tap index ────────────
static void checkImpulseResponseOffset()
{
    // Place the dirac at tap position D — the measured output peak should also
    // appear D samples after the input dirac (the FIR is causal).
    constexpr int D = 4;
    constexpr int kTaps = 32;

    HeadphoneFirHook hook;
    hook.prepare (256, kTaps);

    std::vector<float> ir (kTaps, 0.0f);
    ir[D] = 1.0f;
    hook.loadImpulseResponse (ir.data(), kTaps);
    hook.setBypassed (false);

    // Drain history.
    for (int i = 0; i < kTaps * 2; ++i)
    {
        float l = 0.0f, r = 0.0f;
        hook.processStereoSample (l, r);
    }

    // Inject dirac, then collect kTaps + D output samples.
    const int kCollect = kTaps + D + 4;
    std::vector<float> out (static_cast<size_t> (kCollect), 0.0f);
    for (int i = 0; i < kCollect; ++i)
    {
        float l = (i == 0) ? 1.0f : 0.0f;
        float r = l;
        hook.processStereoSample (l, r);
        out[static_cast<size_t> (i)] = l;
    }

    // Find the peak sample.
    const auto peakIt = std::max_element (out.begin(), out.end());
    const int peakIdx = static_cast<int> (std::distance (out.begin(), peakIt));
    const float peakVal = *peakIt;

    check ("impulse_response_offset",
           peakIdx == D && std::abs (peakVal - 1.0f) < 1e-4f,
           "expectedPeakIdx=" + std::to_string (D)
               + " measuredPeakIdx=" + std::to_string (peakIdx)
               + " peakVal=" + std::to_string (peakVal));
}

// ── Gate 6: long (512-tap) identity FIR — zero-offset dirac output ─────────
static void checkLongFirIdentityOffset()
{
    constexpr int kTaps = 512;

    HeadphoneFirHook hook;
    hook.prepare (256, kTaps);

    std::vector<float> ir (kTaps, 0.0f);
    ir[0] = 1.0f;
    hook.loadImpulseResponse (ir.data(), kTaps);
    hook.setBypassed (false);

    // Drain.
    for (int i = 0; i < kTaps * 2; ++i)
    {
        float l = 0.0f, r = 0.0f;
        hook.processStereoSample (l, r);
    }

    // Inject dirac, collect output.
    float left = 1.0f, right = 1.0f;
    hook.processStereoSample (left, right);
    const float outputAtSample0 = left;

    const bool pass = std::abs (outputAtSample0 - 1.0f) < 1e-4f;
    check ("long_fir_identity_zero_offset",
           pass,
           "tapCount=512 outputAtSample0=" + std::to_string (outputAtSample0));
}

int main()
{
    checkLatencyShortTaps();
    checkLatencyLongTaps();
    checkIdentityImpulseOffset();
    checkChainLatencyFirEngine();
    checkImpulseResponseOffset();
    checkLongFirIdentityOffset();

    const int passed = static_cast<int> (
        std::count_if (checks.begin(), checks.end(), [] (const CheckResult& c) { return c.passed; }));
    const int total = static_cast<int> (checks.size());
    const int failed = total - passed;

    std::cout << "\n--- FIR Truthfulness Probe: " << passed << "/" << total << " passed";
    if (failed > 0)
        std::cout << " (" << failed << " FAILED)";
    std::cout << " ---\n";

    return failed > 0 ? 1 : 0;
}
