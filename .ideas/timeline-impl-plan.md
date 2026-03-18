---
Title: LocusQ Timeline — Extension Implementation Plan
Document Type: Implementation Plan
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Timeline Feature — Extension Implementation Plan

## Context

The Timeline shipped surface (Phase 2.6) is **complete and tested**. This plan covers only the new track types and extensions that arrive when Choreography Lab production-candidates graduate:

- **Type 3 — Formation Track** (Choreography Lab §1 graduation)
- **Type 4 — Procedural Path Track** (Choreography Lab §2 graduation)
- **Beat-Sync Step Mode** (Choreography Lab §4 graduation — Glide/Teleport curve types)
- **Bake Import from Choreography Lab** (architectural bridge, ships with Choreography Lab CL-P7)

These extensions are **Choreography Lab graduation outputs** — they become available as each Choreography Lab production-candidate passes its acceptance gate. The Timeline impl phases map directly to those graduation events.

---

## Architecture (Plan Phase)

### Existing Shipped Baseline (Phase 2.6)

| Component | Status |
|---|---|
| `KeyframeTimeline` — Value Track (Type 1), Position Track (Type 2) | ✅ shipped, tested |
| Interpolation curves: Linear, EaseIn, EaseOut, EaseInOut, Step | ✅ shipped |
| DAW transport sync, internal clock, loop, ping-pong | ✅ shipped |
| Double-buffer swap for keyframe data (message thread → audio thread) | ✅ shipped |
| WebView editor: add/move/delete keyframes, curve cycle, multi-track lane view | ✅ shipped |

### New Components Required

```
KeyframeTimeline extensions
├── trackType enum extensions: formation, procedural_path (new)
├── FormationTrackPayload — formation_type, formation_params, slot_assignments per keyframe
├── ProceduralPathTrackPayload — path_type, path_params per keyframe
├── CurveEnum extensions: Glide, Teleport (beat-sync modes)
└── TrackProvenance metadata struct (source, bake_timestamp, bake_params)

BeatSyncTrack subsystem (within existing audio-thread evaluation)
├── Beat-quantized keyframe time resolution (reads DAW transport, atomic, no callback)
└── Teleport gain-dip envelope — reuses choro_teleport_dip_db / choro_teleport_decay_ms

FormationGeometry cache (message thread)
├── Pre-computed per-slot positions for each Formation Track keyframe
└── Atomic swap when formation parameters change (message thread → audio thread)
```

### Threading Contract (unchanged from Phase 2.6)

- `KeyframeTimeline::evaluate(time)` runs once per `processBlock` on the audio thread.
- Evaluation is read-only against keyframe data. No allocation, no mutex, no I/O.
- Formation geometry pre-computed on message thread; swapped atomically before audio thread reads.
- Procedural path evaluation: closed-form analytical computation on audio thread — no iterative solver.
- Teleport gain-dip envelope computed on audio thread from pre-evaluated keyframe data; written to `EmitterSlot.gain` per block.
- Keyframe data mutations (add/delete/move, new track types) use the existing double-buffer swap mechanism — no changes to the swap protocol.

### Complexity

**Score: 3/5** — phased implementation. The shipped double-buffer and audio-thread evaluation infrastructure is reused; new work is payload types and editor rendering.

---

## Design Phase (UI for Timeline Extensions)

`ui_framework` is `webview`. Extensions integrate into the existing Timeline editor panel.

### New Editor Capabilities

**Formation Track (Type 3):**
- Formation shape wireframe preview in viewport at playhead position
- Morph ghost between adjacent formation keyframes in viewport
- Slot assignment UI: drag emitters to formation positions per keyframe
- Track lane: displays formation type name at each keyframe instead of a value axis
- Overlap validation error surfaced in editor (hard block — formation and position tracks must not overlap)

**Procedural Path Track (Type 4):**
- Ghost path curve drawn in viewport for the selected key range
- Parameter evolution shown as value lanes below the path track
- Ghost updates in real-time as path parameters are adjusted in the track panel

**Beat-Sync Step Mode:**
- Beat grid overlay on timeline ruler at selected `anim_beat_division` resolution
- Keyframe time snapping to beat grid when beat mode is active
- Teleport keyframes rendered with a distinct visual treatment (flash icon)
- Glide keyframes: smooth interpolation curve preview

**Bake Import:**
- Baked tracks appear with a `BAKE` badge in the track header
- Provenance tooltip: bake timestamp, density, tolerance used
- Baked tracks are fully editable after import (no lock)

**Design deliverables:**
- `Design/timeline-extensions-ui-spec.md` — track type rendering, editor chrome for new types
- JS additions to `Source/ui/public/js/index.js` — new relay bindings for `anim_beat_division`, `anim_beat_mode`, `anim_formation_type`, `anim_path_type`

---

## Implementation Plan (Impl Phase)

Phased, complexity=3. Each phase maps to a Choreography Lab graduation event. Phases may begin only after their corresponding Choreography Lab acceptance gate passes.

---

### Phase TL-P1 — Track Type Infrastructure Refactor

**Goal:** Formalize `trackType` enum; add Type 3 and Type 4 skeleton payloads; confirm double-buffer swap handles new payload sizes; no behavioral change to shipped tracks.

**Files to modify:**
- `Source/KeyframeTimeline.h` — add `trackType` enum (`value`, `position`, `formation`, `procedural_path`); add `FormationTrackPayload` and `ProceduralPathTrackPayload` struct stubs; extend `Keyframe` union to hold new payload types
- `Source/KeyframeTimeline.cpp` — `evaluate()` dispatch on `trackType`; new cases return zero contribution (stubs, no behavior yet)

**Prerequisite:** Choreography Lab CL-P1 merged (infrastructure baseline established)

**Acceptance gate:**
- [ ] All Phase 2.6 tests pass with no regression after struct extension
- [ ] Double-buffer swap handles maximum formation payload size without overflow
- [ ] `trackType=formation` and `trackType=procedural_path` stubs: evaluate() returns zero contribution; no crash

---

### Phase TL-P2 — Formation Track (Type 3)

**Goal:** Full Formation Track evaluation; per-slot linear interpolation between adjacent formation keyframes; overlap validation.

**Files:**
- `Source/KeyframeTimeline.h/.cpp` — complete `FormationTrackPayload` (formation_type enum, formation_params struct, slot_assignments array); evaluate() for `trackType=formation`: per-slot position interpolation using existing curve enum
- `Source/FormationGeometryCache.h` — message-thread geometry pre-computation for all Formation Track keyframes; atomic swap before evaluate() reads
- `Source/PluginEditor.h/.cpp` — overlap detection validation: formation track and position track on same emitter must not overlap in time (surfaced as editor error before playback)

**New APVTS params:** `anim_formation_type` enum (line | arc | circle | grid | spiral | sphere_surface | custom)

**[Pending parameter-spec.md addition before impl]**

**Prerequisite:** Choreography Lab CL-P2 (Formation Patterns) acceptance gate passed

**Acceptance gate:**
- [ ] Formation morph: per-slot position error < 1mm vs analytical target at morph completion
- [ ] Overlap validation: hard error surfaced in editor before playback; does not require playback to detect
- [ ] Formation geometry swap: no audio glitch on formation parameter change during playback (tested in pluginval with concurrent UI edits)
- [ ] Slot assignment round-trips through save/restore without reordering

---

### Phase TL-P3 — Procedural Path Track (Type 4)

**Goal:** Procedural path type payload; closed-form path evaluation on audio thread; path parameters as value lanes.

**Files:**
- `Source/KeyframeTimeline.h/.cpp` — complete `ProceduralPathTrackPayload` (path_type enum, path_params struct); evaluate() for `trackType=procedural_path`: closed-form position from current playhead time using path type + params; path parameters themselves addressable as sub-value tracks
- `Source/PathEvaluator.h` — shared analytical path computation (reuses PathSystem logic from ChoreographyWorker; no code duplication — reference the same implementation)

**New APVTS params:** `anim_path_type` enum; per-path-type parameter set (mirrors `choro_*` path params under `anim_*` namespace)

**[Pending parameter-spec.md addition before impl]**

**Prerequisite:** Choreography Lab CL-P3 (Procedural Paths) acceptance gate passed

**Acceptance gate:**
- [ ] CPU overhead < 0.5ms/emitter at 240Hz evaluation rate for all 6 path types
- [ ] Path position matches ChoreographyWorker PathSystem output within floating-point tolerance for identical parameters (shared evaluator confirmed)
- [ ] Path parameters as value lanes: sub-track keyframing produces smooth parameter evolution over time

---

### Phase TL-P4 — Beat-Sync Step Mode

**Goal:** Glide and Teleport curve enum extensions; beat-quantized keyframe times; teleport gain-dip (reusing choreography parameters).

**Files:**
- `Source/KeyframeTimeline.h/.cpp` — extend `CurveEnum` with `Glide` and `Teleport`; beat-quantized time resolution in evaluate() (reads DAW transport position atomically, same as BeatSyncSystem); Teleport keyframe: write gain-dip envelope to `EmitterSlot.gain` (finite-safe: rate-limited, clamped)
- Reuses `choro_teleport_dip_db` and `choro_teleport_decay_ms` parameters (no new params for the envelope itself)

**New APVTS params:** `anim_beat_division` enum, `anim_beat_mode` enum

**[Pending parameter-spec.md addition before impl]**

**Prerequisite:** Choreography Lab CL-P4 (Beat-Sync Choreography) acceptance gate passed

**Acceptance gate:**
- [ ] Keyframe snap timing within ±2ms of beat boundary across 4 DAW hosts
- [ ] Teleport gain-dip: finite-safe under 100 rapid sequential teleports; no NaN; gain never > 0dB
- [ ] `Glide` curve: position arrives exactly at keyframe value on beat boundary (measured)
- [ ] Beat-sync reads DAW transport atomically — no DAW callback on audio thread

---

### Phase TL-P5 — Bake Import from Choreography Lab

**Goal:** Track provenance metadata struct; BAKE badge in editor; round-trip fidelity gate; fully editable after import.

**Files:**
- `Source/KeyframeTimeline.h` — add `TrackProvenance` struct (source enum, bake_timestamp string, bake_params struct); attached to tracks written by BakeRecorder
- `Source/ui/public/js/index.js` — BAKE badge display in track header; provenance tooltip on hover
- `Source/ui/public/index.html` — track header rendering for `source=choreography_bake`

**Prerequisite:** Choreography Lab CL-P7 (Bake to Timeline) acceptance gate passed

**Acceptance gate:**
- [ ] Bake round-trip: reconstructed path RMS error < `bake_curve_fit_tolerance` meters
- [ ] Provenance metadata intact after save/restore cycle (serialize/deserialize round-trip)
- [ ] Baked tracks are editable after import: add, move, delete keyframes; change curves; extend range — all functional
- [ ] `source` field is immutable after bake (no UI path to modify it)

---

### Phase TL-P6 — WebView Controls (Editor UI for New Track Types)

**Goal:** Formation wireframe preview in viewport; path ghost curve; beat-sync indicators; BAKE badge display; full editor integration for all new track types.

**Files:**
- `Source/ui/public/js/index.js` — relay bindings for `anim_beat_division`, `anim_beat_mode`, `anim_formation_type`, `anim_path_type`; viewport overlay: formation wireframe, path ghost curve, beat grid ruler overlay, teleport flash indicator
- `Source/ui/public/index.html` — editor chrome for Formation Track lane (formation type label, slot assignment UI), Path Track lane (parameter sub-lanes), Beat-Sync mode picker on existing track controls
- `Source/PluginEditor.h/.cpp` — relay declarations in correct member order

**Acceptance gate:**
- [ ] WebView 8-point checklist passes (member order, resource provider, backend explicit)
- [ ] Formation wireframe preview renders correctly for all 7 formation types at playhead
- [ ] Path ghost curve updates in real-time as parameters are adjusted
- [ ] BAKE badge visible on baked tracks; provenance tooltip shows correct metadata
- [ ] Overlay layer: no DSP/scene state mutation confirmed via RT audit

---

## Parameter Traceability

All new `anim_*` parameters must be added to `parameter-spec.md` and logged in `Documentation/implementation-traceability.md` before the phase that introduces them begins. Parameters marked **[pending parameter-spec.md]** in this plan are blockers for their respective phases.

---

## Validation Status

`not tested` — plan only. Phase 2.6 shipped surface validation status: **tested** — all Phase 2.6 acceptance criteria passed (CPU: `perf_avg_block_time_ms=0.304`, allocation-free pass, host matrix all pass). Extension acceptance gates are the promotion criteria per `.ideas/timeline-spec.md §Extension Acceptance Gates`.

---

## Execution Order

**TL-P1 → TL-P2 → TL-P3 → TL-P4 → TL-P5 → TL-P6**

Each phase is gated on the corresponding Choreography Lab acceptance gate. TL-P2 cannot begin until CL-P2 passes; TL-P3 until CL-P3 passes; etc.

**Next command:** TL-P1 begins after CL-P1 is complete. Use `/impl` — extend `Source/KeyframeTimeline.h` with `trackType` enum and payload struct stubs.
