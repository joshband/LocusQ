---
Title: LocusQ Physics DAW Automation — Output Recording & Freeze Spec
Document Type: Feature Specification
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Physics DAW Automation — Output Recording & Freeze

## Overview

Extends the physics simulation pipeline to expose physics-driven modulation values as DAW-recordable automation output. Operators can record spread and gain modulation produced by physics into DAW automation lanes, freeze individual emitters to play back recorded curves, and unfreeze to resume live simulation instantly. Position and directivityAim are captured via the Choreography Lab keyframe bake path (separate, already spec'd).

---

## Architecture

### Approach: Atomic state + APVTS mirror

PhysicsDSPBridge already maintains atomic per-slot values (`spreadMod`, `gainMod`, `gainTransient`). This spec adds:

1. A **mirror step** in `processBlock` that writes atomic values into APVTS raw parameter atomics — these are what the DAW polls and records each processing cycle.
2. A **per-slot freeze flag** (`phys_frozen_N`) that gates the mirror write and switches the DSP read source.

Physics never stops. Freeze only switches which source the DSP thread reads from. This means:
- Unfreeze is instant — no re-initialization cost
- Physics state is always current even while frozen
- LIVE→FROZEN transition snapshots the current atomic into APVTS on the audio thread to prevent a value jump

### `gainTransient` exclusion

`PerEmitterDSPValues` has three fields: `spreadMod`, `gainMod`, and `gainTransient`. Only `spreadMod` and `gainMod` are mirrored to APVTS output parameters. `gainTransient` carries one-shot collision/crossing event bursts — by nature momentary and non-recordable as meaningful automation curves. `gainTransient` continues to flow through the live DSP path regardless of freeze state; it is never suppressed by freeze.

### Component changes

```
PhysicsDSPBridge (existing)
  + mirror step: atomic spreadMod/gainMod → APVTS raw parameter atomics
  + freeze gate: reads phys_frozen_N before deciding which path DSP uses

PluginProcessor::processBlock (existing)
  + per-slot freeze check (transition detection: last_frozen[N] != current_frozen[N])
  + LIVE→FROZEN snapshot guard (audio thread — see Data Flow)
  + DSP reads spread/gain from bridge atomics (live) or APVTS raw values (frozen)

ProcessorParameterLayout (existing)
  + 24 new APVTS parameters registered per slot N=0..7 (see Parameter Contract)
  + follows existing per-slot suffix pattern: attractor_N_*, phys_flock_G_*
```

---

## Parameter Contract

24 new APVTS parameters, per emitter slot N = 0..7. These follow the existing per-slot suffix pattern already established by `attractor_N_pos_x`, `phys_flock_G_sep_weight`, etc. in `ProcessorParameterLayout.cpp`. The existing single-instance physics parameters (`phys_spring_enable`, `phys_turbulence`, etc.) remain single-instance (shared across all emitters); the new output mirror and freeze parameters are the first per-slot parameters in the emitter physics group.

### Output mirrors (physics writes, DAW records)

| ID | Type | Range | Display Name | Notes |
|---|---|---|---|---|
| `phys_out_spread_mod_N` | Float | 0..1 | `Emitter N+1 Physics Spread` | Aggregate physics spread contribution for slot N |
| `phys_out_gain_mod_N` | Float | 0..1 | `Emitter N+1 Physics Gain` | Aggregate physics gain contribution for slot N |

### Freeze toggles (operator + DAW automatable)

| ID | Type | Default | Display Name | Notes |
|---|---|---|---|---|
| `phys_frozen_N` | Bool | false | `Emitter N+1 Physics Freeze` | false = Live Physics; true = Frozen (play recorded) |

### Authority chain position

`phys_out_spread_mod_N` and `phys_out_gain_mod_N` are additive offsets in the spread/gain arbitration chain — the same position as the current physics atomics. Freezing does not change their authority chain position; it only changes which process writes the value (physics worker vs DAW playback). ADR-0020 is unaffected.

### Parameter count delta

Emitter: 55 → 79 (+24). Total: ~165 → ~189.

---

## Data Flow

### Audio-thread write pattern

All parameter writes from `processBlock` use the lock-free JUCE raw parameter atomic pattern:

```cpp
// Write (audio thread — RT-safe):
getRawParameterValue("phys_out_spread_mod_N")->store(value);

// Read (audio thread — RT-safe):
float value = getRawParameterValue("phys_out_spread_mod_N")->load();
```

`setValueNotifyingHost` is **not** called from `processBlock`. It is not RT-safe — it triggers host listener callbacks synchronously, which may allocate or hold locks. The DAW records automation by polling `getRawParameterValue` each processing cycle; no notification is required for recording to work.

Host notification (for UI refresh) is dispatched asynchronously via `AsyncUpdater::triggerAsyncUpdate()` from `processBlock`, resolved on the message thread.

### Live path (`phys_frozen_N = false`)

```cpp
// processBlock, per slot N where phys_frozen_N == false:
PerEmitterDSPValues val = dspBridge.readAtomic(N);  // single read — val used for both mirror and DSP
getRawParameterValue("phys_out_spread_mod_N")->store(val.spreadMod);  // DAW polls this
getRawParameterValue("phys_out_gain_mod_N")->store(val.gainMod);
float spreadDelta = val.spreadMod;   // DSP uses same val as mirrored to APVTS — not a second read
float gainDelta   = val.gainMod;
// gainTransient flows separately through DSP path regardless of freeze state
```

### Frozen path (`phys_frozen_N = true`)

```cpp
// processBlock, per slot N where phys_frozen_N == true:
float spreadDelta = getRawParameterValue("phys_out_spread_mod_N")->load();  // DAW playback drives this
float gainDelta   = getRawParameterValue("phys_out_gain_mod_N")->load();
// no store() call — DAW owns the value
// physics atomics keep updating silently (worker never stops)
// gainTransient continues through DSP path unaffected
```

### LIVE → FROZEN transition (snapshot guard — audio thread)

The snapshot guard executes entirely on the audio thread. The UI click only sets `phys_frozen_N` in APVTS; `processBlock` detects the false→true transition and performs the snapshot at the precise block boundary where freeze becomes effective — guaranteeing the APVTS value is seeded before the first frozen read.

```cpp
// processBlock, transition detection:
bool was_frozen = last_frozen_state[N];
bool now_frozen = getRawParameterValue("phys_frozen_N")->load() > 0.5f;
if (!was_frozen && now_frozen) {
    // LIVE→FROZEN: snapshot current atomic into APVTS before switching read source
    PerEmitterDSPValues val = dspBridge.readAtomic(N);
    getRawParameterValue("phys_out_spread_mod_N")->store(val.spreadMod);
    getRawParameterValue("phys_out_gain_mod_N")->store(val.gainMod);
}
last_frozen_state[N] = now_frozen;
```

This eliminates any race between UI toggle and audio-thread read: the first frozen `processBlock` always finds a valid seeded value.

### FROZEN → LIVE transition

No special handling. Physics atomics are always current; DSP switches to reading them immediately on the block where `phys_frozen_N` transitions to false.

---

## UI Surface

Visual design authority: `docs/superpowers/specs/2026-03-18-visualization-design.md`. All visual treatment below defers to that spec's language. Do not introduce new visual primitives here.

### Freeze toggle

Location: EMITTER panel, physics subsection — first row after `phys_enable`, before Spring/Turbulence/Angular sub-sections.

```
[LIVE]  ←→  [FROZEN]
```

- **LIVE state:** uses the §6 State Ring Physics quadrant visual language (blue `rgba(160,210,255)`, breathing animation tied to simulation activity). The toggle reads the same state that drives the ring quadrant — no separate activity polling needed.
- **FROZEN state:** Physics quadrant of state ring goes static/dim (same as physics-inactive state per §6 rules). The toggle label changes to FROZEN; no new color primitive introduced.
- **Transition click:** sets `phys_frozen_N` in APVTS only. The snapshot guard executes on the audio thread in `processBlock` at the precise freeze-effective block boundary (see Data Flow — snapshot guard).

### Activity feedback

Physics modulation activity (spread/gain actively changing) is already expressed by §4 Physics-Reactive Atmosphere layers (attractor tint B, collision flash C, etc.). No separate spread/gain meters needed. When frozen, atmosphere layers reflect the APVTS playback values — the same visual read, driven by a different source.

### DAW automation lane names

Set via `AudioProcessorParameter::getName()`:
- `phys_out_spread_mod_0..7` → `"Emitter 1 Physics Spread"` .. `"Emitter 8 Physics Spread"`
- `phys_out_gain_mod_0..7` → `"Emitter 1 Physics Gain"` .. `"Emitter 8 Physics Gain"`
- `phys_frozen_0..7` → `"Emitter 1 Physics Freeze"` .. `"Emitter 8 Physics Freeze"`

No new panel or card required — fits inside the existing physics subsection with one row added per emitter.

---

## Keyframe Bake Path

Out of scope for this spec. Position and directivityAim are captured via the Choreography Lab graduation mechanism (already spec'd in `choreography-lab-spec.md §Graduation Path`).

**Boundary:** `spread_mod` + `gain_mod` → live APVTS output (this spec). `position` + `directivityAim` → keyframe bake (Choreography Lab scope). The two paths are independent and composable: an emitter can have its spread/gain frozen from a recording while its position remains live physics, or vice versa.

---

## Acceptance Gates

- [ ] `phys_out_spread_mod_N` and `phys_out_gain_mod_N` registered in ProcessorParameterLayout for all 8 slots using `getRawParameterValue` lock-free pattern
- [ ] `phys_frozen_N` registered for all 8 slots
- [ ] `last_frozen_state[8]` array maintained in PluginProcessor for transition detection
- [ ] LIVE path: physics atomic values appear in DAW automation lane during recording (verified in Logic and Reaper)
- [ ] FROZEN path: DAW playback values drive DSP; physics atomics update silently
- [ ] LIVE→FROZEN snapshot guard: no value jump on transition (measured: |pre - post| < 0.01); guard executes on audio thread only
- [ ] FROZEN→LIVE transition: DSP switches to live atomics within one `processBlock` call
- [ ] No `setValueNotifyingHost` calls from `processBlock` (RT-safety: verified via pluginval + Reaper thread-safety mode)
- [ ] AsyncUpdater dispatches host notification from message thread after freeze state change
- [ ] `gainTransient` flows through DSP path regardless of freeze state (not mirrored, not suppressed)
- [ ] All 24 parameters added to `parameter-spec.md` and `implementation-traceability.md`
- [ ] DAW automation lane display names correct in Logic and Reaper
- [ ] No NaN or out-of-range values under adversarial physics inputs (inherits P8 DSP bridge clamp)

---

## Validation Status

Not tested — spec only.

---

## ADR Status

No new ADR required. Authority chain is unchanged (ADR-0020 remains normative). The RT-safe write pattern (`getRawParameterValue()->store()` from audio thread) is consistent with ADR-0002 (lock-free atomic contract). If any host exposes a conflict between parameter polling and the lock-free write, a new ADR must be recorded before closing the implementation phase.
