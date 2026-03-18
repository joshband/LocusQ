---
Title: LocusQ Choreography Lab — Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Choreography Lab Feature — Implementation Plan

## Dream Phase Synthesis

**Concept:** A generative motion layer that sits between the Timeline rest pose and Physics additive offset (ADR-0020 Layer 3). The operator configures rules — formation shapes, analytical paths, beat-sync quantization, audio-reactive mappings — and the system generates emitter positions. Output is non-deterministic per session unless baked. The **bake-to-Timeline** mechanism is the graduation path: exploratory Choreography sessions become deterministic Timeline assets.

**Scope boundary for first pass:** Production-candidates (Formations, Procedural Paths, Beat-Sync) before Lab features (Audio-Reactive, Multi-Emitter Coordination). Bake-to-Timeline ships with the Lab because it is the graduation enabler.

---

## Architecture (Plan Phase)

### Authority Chain Position (ADR-0020)

ChoreographyWorker is a **logical module colocated in the PhysicsWorker tick**. Each tick:

```
[PhysicsWorker tick]
  1. Audio ring buffer read → feature extraction
  2. ChoreographyWorker.compute() → writes ChoreographyOffset per emitter
  3. PhysicsWorker composes rest pose: APVTS + Timeline + ChoreographyOffset
  4. Physics integration → final EmitterSlot position
```

No new OS thread. No second atomic write path into `EmitterSlot`. The single EmitterSlot swap per tick (ADR-0002) is preserved.

### New Architectural Components

```
ChoreographyWorker (new logical module, no new thread)
├── FormationSystem       — 7 built-in shapes; morph; spread DSP hook
├── PathSystem            — 6 analytical path types; velocity → Doppler
├── BeatSyncSystem        — beat-grid quantization; teleport gain-dip; step sequencer
├── AudioFeatureExtractor — reads lock-free ring buffer; rms/peak/onset/band_energy/centroid/flux
├── ReactiveMapper        — normalize→deadband→curve→smooth→clamp per feature→target binding
├── CoordinationSystem    — leader/follower; phase-offset; mirror; proximity coupling
└── BakeRecorder          — position ring buffer; downsample; curve-fit; Timeline keyframe export

AudioRingBuffer (new, lock-free)
  — audio thread writes block data; ChoreographyWorker reads; bounded pre-allocated

ChoreographyOffset (new per-emitter struct, internal to worker tick)
  — position: Vec3 (generative offset, additive on rest pose)
  — spread_delta: float (additive, clamped before EmitterSlot write)
  — gain_delta: float (additive, clamped before EmitterSlot write)
  — velocity: Vec3 (path velocity, Doppler source)
```

### Threading Contract (ADR-0020)

- ChoreographyWorker runs at the start of each PhysicsWorker tick.
- Audio thread writes per-block audio data to `AudioRingBuffer` (lock-free, non-blocking).
- ChoreographyWorker reads `AudioRingBuffer` — consumer side, non-blocking.
- `ChoreographyOffset` is a worker-thread-local struct; no cross-thread access.
- Leader/follower history ring buffer pre-allocated at startup. No runtime allocation.
- No choreography logic on audio thread.

### DSP Mapping Pipeline (reactive-av contract)

```
raw_feature → normalize(domain) → deadband(optional) → curve(linear/sigmoid/etc)
            → smooth(attack/release ms) → clamp(0..1) → publish to ChoreographyOffset field
```

### Complexity

**Score: 5/5** — phased implementation required.

---

## Design Phase (UI for Choreography Lab Controls)

`ui_framework` is `webview`. Controls integrate into a new **CHOREOGRAPHY** panel tab following established APVTS relay → WebView binding patterns.

### New Control Surfaces

**CHOREOGRAPHY panel — Formation sub-section:**
- `choro_enable` master toggle
- Formation shape picker (7 types), spacing / radius / rows / cols knobs per type
- Morph rate knob (`choro_formation_morph_rate`), loop + ping-pong toggles
- Spread contribution meter (read-only, shows additive spread offset)

**CHOREOGRAPHY panel — Path sub-section:**
- Path type picker (6 types), per-type parameter rows (frequency ratios, radius, period, etc.)
- Path preview ghost curve in viewport (next N seconds)
- Velocity/Doppler meter

**CHOREOGRAPHY panel — Beat-Sync sub-section:**
- Beat division picker (`choro_beat_division`)
- Beat mode picker (snap / glide / teleport)
- Teleport dip depth + decay knobs
- Step sequencer grid (16 steps × hold/advance/jump)
- Beat grid markers on transport bar

**CHOREOGRAPHY panel — Audio-Reactive sub-section:**
- Audio source bus selector
- Band-count + band-edge frequency inputs
- Feature meter strip (rms, peak, onset, band_energy[N], centroid, flux)
- Per-binding mapping rows (source → target, deadband, curve, smooth)

**CHOREOGRAPHY panel — Coordination sub-section:**
- Leader emitter selector; follower delay + offset controls per follower
- Phase-offset step input
- Mirror plane picker
- Proximity coupling radius knob

**CHOREOGRAPHY panel — Bake section:**
- Bake range (start / end transport positions)
- Keyframe density knob
- Curve-fit tolerance input
- Bake preview button (ghost curve in viewport)
- Commit bake button → writes to Timeline

**Visualization additions (viewport layer — read-only, no DSP mutation):**
- Formation wireframe shape + slot-to-emitter assignment lines
- Path preview ghost curve (next N seconds)
- Beat grid markers on transport bar; emitter pulse ring on each quantized step
- Per-emitter RMS/onset indicator rings (`rend_viz_reactive` toggle)
- Bake preview ghost (original captured vs curve-reduced)

**Design deliverables:**
- `Design/choreography-lab-ui-spec.md` — control placement and layout
- `Design/choreography-lab-style-guide.md` — formation/path overlay visual treatment
- JS additions to `Source/ui/public/js/index.js` — relay bindings for all `choro_*` parameters

---

## Implementation Plan (Impl Phase)

Phased, complexity=5. Each phase must pass its acceptance gate before advancing.
Production-candidates (CL-P2, CL-P3, CL-P4) land before Lab features (CL-P5, CL-P6).

---

### Phase CL-P1 — Infrastructure: ChoreographyWorker Module + AudioRingBuffer

**Goal:** ChoreographyWorker module integrated into PhysicsWorker tick per ADR-0020; lock-free audio ring buffer; `ChoreographyOffset` struct; `choro_enable` gate.

**Files to create/modify:**
- `Source/ChoreographyWorker.h/.cpp` — module shell, `compute()` entry point, `ChoreographyOffset` per-emitter output, `choro_enable` gate
- `Source/AudioRingBuffer.h` — lock-free bounded ring buffer; pre-allocated at startup; written by audio thread, read by ChoreographyWorker
- `Source/PhysicsWorker.h/.cpp` — integrate ChoreographyWorker at start of tick; compose rest pose from APVTS + Timeline + ChoreographyOffset before physics step

**New APVTS params:** `choro_enable`

**Acceptance gate:**
- [ ] PhysicsWorker tick executes ChoreographyWorker.compute() before physics step; verified via deterministic tick log
- [ ] AudioRingBuffer: write → read round-trip produces identical data under concurrent audio thread writes
- [ ] `choro_enable=false`: ChoreographyOffset is zero; EmitterSlot output identical to pre-choreography baseline
- [ ] No allocation on audio thread or worker thread during steady-state operation

---

### Phase CL-P2 — Formation Patterns

**Goal:** 7 built-in formation shapes; per-group morph animation; spread DSP hook.

**Files:**
- `Source/FormationSystem.h/.cpp` — geometry computation for all 7 types; per-slot position output; morph interpolation (per-slot linear between source/target formations)
- `Source/ChoreographyWorker.h/.cpp` — integrate FormationSystem; publish position offsets and spread delta to `ChoreographyOffset`

**New APVTS params:** per-formation-type params (radius, rows, cols, spacing, arc_angle, turns, height_rise, point_count, phase_offset); `choro_formation_morph_rate`, `choro_formation_morph_loop`, `choro_formation_morph_pingpong`; `choro_formation_type` enum

All **[pending parameter-spec.md addition before impl]**

**DSP hook:** formation spread (avg inter-emitter distance / max) → `ChoreographyOffset.spread_delta` (additive, clamped in PhysicsWorker before EmitterSlot write)

**Acceptance gate:**
- [ ] Formation morph: per-slot position error < 1mm vs analytical target at morph completion
- [ ] `choro_formation_morph_pingpong`: confirmed reversal at boundary
- [ ] All 7 formation types produce distinct non-degenerate geometries at N=2..8 emitters
- [ ] Spread delta: no out-of-range values under adversarial spacing inputs

---

### Phase CL-P3 — Procedural Paths

**Goal:** 6 closed-form analytical path types; velocity → Doppler hook.

**Files:**
- `Source/PathSystem.h` — 6 path types (lissajous, orbit, pendulum, figure_eight, helix, random_walk); closed-form position computation; velocity as positional delta between ticks
- `Source/ChoreographyWorker.h/.cpp` — integrate PathSystem; publish position and velocity to `ChoreographyOffset`

**New APVTS params:** `choro_path_type` enum; per-path-type params (frequency ratios `a:b:c`, amplitudes, phase δ, `radius_x/z`, `height`, `period`, `length`, `amplitude`, `plane`, `scale`, `pitch`, `direction`, `step_size`, `bounds`, `seed`)

All **[pending parameter-spec.md addition before impl]**

**DSP hook:** path velocity at current position → `ChoreographyOffset.velocity` (Doppler via existing `EmitterSlot.velocity` field)

**Acceptance gate:**
- [ ] Procedural path CPU: < 0.5ms/emitter at 240Hz worker rate (all 6 types measured)
- [ ] Lissajous: closed-curve topology verified for integer frequency ratios
- [ ] `random_walk`: stays within configured `bounds` under 1000-tick soak; reproducible from same `seed`
- [ ] Velocity Doppler: `ChoreographyOffset.velocity` finite for all path types under adversarial parameter values

---

### Phase CL-P4 — Beat-Sync Choreography

**Goal:** Beat-grid quantization; snap/glide/teleport modes; teleport gain-dip envelope; 16-step pattern sequencer.

**Files:**
- `Source/BeatSyncSystem.h/.cpp` — DAW transport position atomic read; beat-division tick trigger; snap/glide/teleport position interpolation; gain-dip envelope computation; 16-step pattern sequencer (hold/advance/jump)
- `Source/ChoreographyWorker.h/.cpp` — integrate BeatSyncSystem; gain-dip written to `ChoreographyOffset.gain_delta`

**New APVTS params:** `choro_beat_division` enum, `choro_beat_mode` enum, `choro_teleport_dip_db`, `choro_teleport_decay_ms`

All **[pending parameter-spec.md addition before impl]**

**Acceptance gate (hard blocking gate):**
- [ ] Position snap timing within ±2ms of beat boundary across 4 DAW hosts
- [ ] Teleport gain-dip: finite-safe under 100 rapid sequential teleports; no NaN; gain never > 0dB (no amplification)
- [ ] Beat-sync reads DAW transport position atomically — no DAW callback on audio thread (confirmed via RT audit)
- [ ] Pattern sequencer: step advance on each configured beat division; jump-to-slot index produces correct formation slot

---

### Phase CL-P5 — Audio-Reactive Choreography (Lab)

**Goal:** Lock-free audio feature extraction; 6 feature types; reactive behavior mappings per reactive-av contract.

**Files:**
- `Source/AudioFeatureExtractor.h/.cpp` — rms, peak, onset (spectral flux threshold), band_energy[N], spectral_centroid, spectral_flux; reads from `AudioRingBuffer`; configurable `choro_audio_smooth_ms`
- `Source/ReactiveMapper.h` — normalize→deadband→curve→smooth→clamp per binding; multiple simultaneous source→target bindings
- `Source/ChoreographyWorker.h/.cpp` — integrate extractor and mapper; publish reactive offsets to `ChoreographyOffset`

**New APVTS params:** `choro_audio_src_bus`, `choro_audio_band_count`, `choro_audio_band_hz[N]`, `choro_audio_smooth_ms`, `choro_audio_buffer_len_ms`

All **[pending parameter-spec.md addition before impl]**

**Lab acceptance gate (required before production promotion):**
- [ ] Onset detection latency ≤10ms from transient (measured with known test signal)
- [ ] Feature extractor: deterministic output for identical input stream replay (same buffer contents → same feature values)
- [ ] No audio-thread contention: audio thread writes to ring buffer only; feature computation on worker thread only
- [ ] `rms` → orbit radius: emitter displacement proportional to RMS under sweep test

---

### Phase CL-P6 — Multi-Emitter Coordination (Lab)

**Goal:** Leader/follower; phase-offset ensemble; mirror/symmetry; proximity coupling.

**Files:**
- `Source/CoordinationSystem.h/.cpp` — leader/follower position history ring buffer (pre-allocated, max 2000ms delay); phase-offset per slot index; geometric mirror (3 axis planes + arbitrary normal); proximity coupling midpoint attraction within `choro_couple_radius`
- `Source/ChoreographyWorker.h/.cpp` — integrate CoordinationSystem

**New APVTS params:** `choro_follow_delay_ms`, `choro_follow_offset: Vec3`, `choro_phase_step`, `choro_mirror_plane` enum, `choro_mirror_origin: Vec3`, `choro_couple_radius`

All **[pending parameter-spec.md addition before impl]**

**Lab acceptance gate:**
- [ ] Leader/follower delay accuracy: ±1ms at configured `choro_follow_delay_ms` (measured against captured position log)
- [ ] Runtime change to `choro_follow_delay_ms`: adjusts read pointer only — no reallocation confirmed via allocation tracker
- [ ] Mirror: reflected position matches analytical geometric reflection within floating-point tolerance
- [ ] Proximity coupling: no allocation or lock on worker thread under 64 emitters

---

### Phase CL-P7 — Bake to Timeline

**Goal:** Record mode position capture; downsample; curve-fit; Timeline keyframe export.

**Files:**
- `Source/BakeRecorder.h/.cpp` — position ring buffer at `rend_phys_rate` Hz for configured transport range; downsample to `bake_kf_density` kf/s; curve-fit per track (max positional error bounded by `bake_curve_fit_tolerance`); write keyframe tracks to `KeyframeTimeline`
- `Source/ChoreographyWorker.h/.cpp` — integrate BakeRecorder; record mode activation/deactivation

**New APVTS params:** `bake_start` (transport pos), `bake_end` (transport pos), `bake_kf_density`, `bake_curve_fit_tolerance`

All **[pending parameter-spec.md addition before impl]**

**Acceptance gate:**
- [ ] Reconstructed path RMS error < `bake_curve_fit_tolerance` meters across a 30-second bake
- [ ] Baked keyframe tracks appear in Timeline editor with `source=choreography_bake` provenance metadata
- [ ] Baked tracks are fully editable after import (add/move/delete keyframes, change curves)
- [ ] BakeRecorder allocates only at record-start; no runtime allocation during capture

---

### Phase CL-P8 — WebView Controls + Visualization Overlays

**Goal:** Wire all `choro_*` params through WebView relays; add CHOREOGRAPHY panel; add viewport overlays.

**Files:**
- `Source/ui/public/js/index.js` — relay bindings for all CL-P1–CL-P7 parameters
- `Source/ui/public/index.html` — CHOREOGRAPHY panel tab with Formation, Path, Beat-Sync, Audio-Reactive, Coordination, Bake sub-sections
- `Source/PluginEditor.h/.cpp` — relay declarations in correct member order (Relays → WebView → Attachments)
- Viewport overlay layer in Three.js scene: formation wireframe, path ghost curve, beat grid markers, emitter pulse rings, RMS/onset indicator rings, bake preview ghost

**Acceptance gate:**
- [ ] WebView 8-point checklist passes (member order, resource provider, backend explicit)
- [ ] All `choro_*` parameters appear in UI with correct range and label
- [ ] Overlay layer: no DSP/scene state mutation confirmed via RT audit
- [ ] `rend_viz_reactive` toggle: overlay visible/hidden without DSP side-effect

---

## Parameter Traceability

All new `choro_*` and `bake_*` parameters must be added to `parameter-spec.md` and logged in `Documentation/implementation-traceability.md` before the phase that introduces them begins. Parameters marked **[pending parameter-spec.md]** in this plan are blockers for their respective phases.

---

## Validation Status

`not tested` — plan only. Production-candidate acceptance gates (CL-P2, CL-P3, CL-P4) and Lab acceptance gates (CL-P5, CL-P6) are the promotion criteria per `.ideas/choreography-lab-spec.md §Containment Governance`.

---

## Execution Order

**CL-P1 → CL-P2 → CL-P3 → CL-P4 → CL-P7 → CL-P8** (production-candidates + bake)
then
**CL-P5 → CL-P6** (Lab features, after production-candidate gate)

CL-P7 (Bake) ships alongside CL-P4 because it is the graduation enabler; it does not require CL-P5/CL-P6.

**Next command:** Begin Phase CL-P1 with `/impl` — create `Source/ChoreographyWorker.h/.cpp`, `Source/AudioRingBuffer.h`, and extend `Source/PhysicsWorker` tick integration.
