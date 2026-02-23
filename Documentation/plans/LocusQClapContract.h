// LocusQClapContract.h
// Contract header for LocusQ CLAP integration.
// This file encodes the normative DSP/adapter contract so implementation cannot drift.
//
// Real-time + determinism invariants:
// - No heap allocation in the audio thread
// - No locks in the audio thread
// - Deterministic behavior for identical event streams
// - Capability negotiation is immutable per session
//
// Intended usage:
// - Included by CLAP adapter and (optionally) QA harness utilities
// - Spatial engine consumes adapter-produced events in a format-agnostic way

#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <type_traits>

namespace locusq::clap_contract {

//==============================================================================
// Versioning (manual bump when breaking contract-level behavior)
//==============================================================================
struct ContractVersion final
{
    static constexpr uint32_t major = 1;
    static constexpr uint32_t minor = 0;
    static constexpr uint32_t patch = 0;
};

//==============================================================================
// Capability negotiation
//==============================================================================

struct Capabilities final
{
    bool hasParams         = false;
    bool hasNotePorts      = false;
    bool hasVoiceInfo      = false;
    bool hasNoteExpression = false;
    bool hasPolyMod        = false;
};

enum class RuntimeMode : uint8_t
{
    PolyVoice = 0,  // voice IDs + per-voice modulation/expression
    VoiceOnly = 1,  // voice IDs only; no per-voice modulation/expression
    GlobalOnly = 2  // no voice IDs; global-only behavior
};

struct NegotiationResult final
{
    Capabilities caps {};
    RuntimeMode  mode = RuntimeMode::GlobalOnly;
};

// Deterministic mode selection rule.
// NOTE: Adapter MUST compute this once at init/activation and treat as immutable.
[[nodiscard]] constexpr RuntimeMode selectRuntimeMode(const Capabilities& c) noexcept
{
    if (c.hasVoiceInfo && (c.hasNoteExpression || c.hasPolyMod))
        return RuntimeMode::PolyVoice;

    if (c.hasVoiceInfo)
        return RuntimeMode::VoiceOnly;

    return RuntimeMode::GlobalOnly;
}

//==============================================================================
// Deterministic per-voice mapping
//==============================================================================

constexpr int32_t kInvalidVoiceId = -1;

// Default voice capacity for LocusQ.
// You MAY override by defining LOCUSQ_CLAP_KMAXVOICES before including this header.
#ifndef LOCUSQ_CLAP_KMAXVOICES
static constexpr uint16_t kMaxVoices = 64;
#else
static constexpr uint16_t kMaxVoices = static_cast<uint16_t>(LOCUSQ_CLAP_KMAXVOICES);
#endif

enum class VoiceStage : uint8_t
{
    Off = 0,
    Active = 1,
    Release = 2
};

// Voice slot model. MUST remain POD/trivially-copyable for deterministic storage/QA.
// Spatial fields are intentionally minimal; the engine may extend via separate state.
struct VoiceSlot final
{
    VoiceStage stage = VoiceStage::Off;
    int32_t    clapVoiceId = kInvalidVoiceId;  // opaque host-provided voice id
    uint32_t   age = 0;                        // monotonic allocation counter for deterministic eviction

    // Canonical spatial state for adapter/engine handshake.
    // Range conventions:
    // - x, y in [-1, +1]
    // - gain in [0, +inf) (engine may clamp)
    float      x = 0.0f;
    float      y = 0.0f;
    float      gain = 1.0f;
};

static_assert(std::is_trivially_copyable_v<VoiceSlot>, "VoiceSlot must be trivially copyable");
static_assert(std::is_standard_layout_v<VoiceSlot>, "VoiceSlot must be standard layout");

//==============================================================================
// Sample-accurate modulation
//==============================================================================

// Modulation semantics choice.
// Adapter/engine MUST agree on exactly one of these for a session.
// Default is Delta (base + accumulated deltas).
enum class ModulationSemantics : uint8_t
{
    Delta = 0,     // effective = base + sum(deltas)
    Absolute = 1   // effective = value (optionally plus base if you define that; must be documented)
};

// Parameter namespace for targets used by expression routing and modulation.
// Keep small and stable; map these to APVTS/parameter IDs at the adapter boundary.
enum class SpatialTarget : uint16_t
{
    X = 0,
    Y = 1,
    Gain = 2,

    // Phase 2 (reserve stable IDs now to avoid ABI churn):
    Rotation = 3,
    Distance = 4,
    Diffusion = 5
};

// Expression sources the adapter may translate into modulation.
enum class ExprSource : uint16_t
{
    // Generic poly mod slots (host-specific mapping handled in adapter):
    PolyParam0 = 0,
    PolyParam1 = 1,
    PolyParam2 = 2,
    PolyParam3 = 3,

    // Note expression concepts (exact availability is host/extension dependent):
    NotePressure   = 100,
    NoteTimbre     = 101,
    NoteBrightness = 102,
    PitchBend      = 103
};

// Curve mapping used in routing.
// Keep as an enum (not function pointer) for determinism and serialization.
enum class Curve : uint8_t
{
    Linear = 0,
    Exp = 1,
    Log = 2,
    SCurve = 3
};

// Routing entry: explicit, documented, serializable.
// Adapter uses these to translate ExprSource events into ModEvents for targets.
struct Route final
{
    ExprSource    source = ExprSource::PolyParam0;
    SpatialTarget target = SpatialTarget::X;

    float amount = 0.0f;   // scale
    float offset = 0.0f;   // bias

    Curve curve = Curve::Linear;

    // Optional smoothing (time constant) for UI-configurable routes.
    // Adapter MUST implement smoothing without heap allocation.
    uint8_t smoothingMs = 0;
};

static_assert(std::is_trivially_copyable_v<Route>, "Route must be trivially copyable");
static_assert(std::is_standard_layout_v<Route>, "Route must be standard layout");

// Modulation event produced by adapter and consumed by engine.
// - frameOffset is sample-accurate within current block.
// - slot is internal voice slot index [0..kMaxVoices-1] (or 0 for GlobalOnly).
// - paramId is a stable target identifier (see SpatialTarget) OR your internal param enum.
//   If you use SpatialTarget directly, set paramId = static_cast<uint16_t>(SpatialTarget::X), etc.
struct ModEvent final
{
    uint16_t frameOffset = 0;  // 0..blockSize-1
    uint16_t paramId     = 0;  // stable param id namespace (typically SpatialTarget)
    uint8_t  slot        = 0;  // 0..kMaxVoices-1
    float    value       = 0.0f; // delta or absolute depending on ModulationSemantics
};

static_assert(std::is_trivially_copyable_v<ModEvent>, "ModEvent must be trivially copyable");
static_assert(std::is_standard_layout_v<ModEvent>, "ModEvent must be standard layout");

// A deterministic overflow policy for bounded queues/rings.
enum class OverflowPolicy : uint8_t
{
    DropOldest = 0,
    DropNewest = 1
};

// Default capacities. You MAY override at compile time.
#ifndef LOCUSQ_CLAP_KMAXMODEVENTS
static constexpr uint32_t kMaxModEvents = 4096;
#else
static constexpr uint32_t kMaxModEvents = static_cast<uint32_t>(LOCUSQ_CLAP_KMAXMODEVENTS);
#endif

//==============================================================================
// Telemetry (DSP -> UI) contract structures
//==============================================================================

struct alignas(32) VoiceTelemetryEvent final
{
    int32_t  clapVoiceId = kInvalidVoiceId; // host voice id where available
    uint8_t  slot        = 0;               // internal slot index
    uint16_t frameOffset = 0;               // within block for sample-accurate alignment

    float x = 0.0f;
    float y = 0.0f;
    float gain = 1.0f;

    // Optional expanded observability (keep, but engine may choose not to populate):
    float azimuth = 0.0f;
    float distance = 0.0f;
};

static_assert(std::is_trivially_copyable_v<VoiceTelemetryEvent>, "VoiceTelemetryEvent must be trivially copyable");
static_assert(std::is_standard_layout_v<VoiceTelemetryEvent>, "VoiceTelemetryEvent must be standard layout");

//==============================================================================
// Lock-free SPSC ring buffer (bounded, deterministic overflow)
//==============================================================================

template <size_t CapacityPow2, OverflowPolicy Policy = OverflowPolicy::DropOldest>
class VoiceTelemetryRing final
{
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0,
                  "CapacityPow2 must be a power of two");

public:
    VoiceTelemetryRing() noexcept { clear(); }

    // Producer: audio thread only
    bool push(const VoiceTelemetryEvent& e) noexcept
    {
        const uint32_t w = writeIndex_.load(std::memory_order_relaxed);
        const uint32_t r = readIndex_.load(std::memory_order_acquire);

        if ((w - r) >= CapacityPow2)
        {
            if constexpr (Policy == OverflowPolicy::DropNewest)
            {
                // Deterministically drop the incoming event
                return false;
            }
            else
            {
                // Drop oldest by advancing read index deterministically by 1
                readIndex_.store(r + 1, std::memory_order_release);
            }
        }

        buffer_[w & mask_] = e;
        writeIndex_.store(w + 1, std::memory_order_release);
        return true;
    }

    // Consumer: UI thread only
    bool pop(VoiceTelemetryEvent& out) noexcept
    {
        const uint32_t r = readIndex_.load(std::memory_order_relaxed);
        const uint32_t w = writeIndex_.load(std::memory_order_acquire);

        if (r == w)
            return false;

        out = buffer_[r & mask_];
        readIndex_.store(r + 1, std::memory_order_release);
        return true;
    }

    void clear() noexcept
    {
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] constexpr size_t capacity() const noexcept { return CapacityPow2; }

private:
    static constexpr uint32_t mask_ = static_cast<uint32_t>(CapacityPow2 - 1);

    alignas(64) std::atomic<uint32_t> writeIndex_ {0};
    alignas(64) std::atomic<uint32_t> readIndex_ {0};

    alignas(64) VoiceTelemetryEvent buffer_[CapacityPow2] {};
};

//==============================================================================
// Helper mapping functions (deterministic, no state)
//==============================================================================

// Curve mapping: deterministic, no allocations.
// NOTE: For Exp/Log/SCurve, you may implement approximations elsewhere;
// this function exists to formalize the contract, not enforce a specific curve math.
[[nodiscard]] inline float applyCurve(Curve c, float v01) noexcept
{
    // Clamp to [0,1] deterministically without <algorithm>.
    if (v01 < 0.0f) v01 = 0.0f;
    if (v01 > 1.0f) v01 = 1.0f;

    switch (c)
    {
        case Curve::Linear: return v01;

        case Curve::Exp:
            // Simple exp-ish curve: v^2 (deterministic, fast)
            return v01 * v01;

        case Curve::Log:
            // Simple log-ish curve: sqrt(v) (deterministic)
            // Avoid <cmath> dependency here; engine may replace with real sqrt.
            // Approx sqrt via one Newton step from v (good enough for UI mapping).
            if (v01 <= 0.0f) return 0.0f;
            {
                float x = v01;           // initial
                x = 0.5f * (x + v01 / x);
                return x;
            }

        case Curve::SCurve:
            // Smoothstep: 3t^2 - 2t^3
            return v01 * v01 * (3.0f - 2.0f * v01);
    }
    return v01;
}

// Map an expression value in [0,1] to a routed modulation value.
[[nodiscard]] inline float mapExpression(const Route& r, float v01) noexcept
{
    const float shaped = applyCurve(r.curve, v01);
    return shaped * r.amount + r.offset;
}

//==============================================================================
// Sanity checks for contract drift
//==============================================================================

static_assert(kMaxVoices > 0, "kMaxVoices must be > 0");
static_assert(kMaxModEvents > 0, "kMaxModEvents must be > 0");

} // namespace locusq::clap_contract