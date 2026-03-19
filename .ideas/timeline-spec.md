---
Title: LocusQ Timeline Feature Spec
Document Type: Feature Specification
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18
---

# Timeline Feature Spec

## Scope

The Timeline is the shipped production surface for deterministic, composed emitter motion. This spec covers:

1. The full current surface (Phase 2.6 shipped)
2. The track type system — what is keyframeable and how
3. Extensions that arrive when Choreography Lab production-candidates graduate
4. The bake-import contract from Choreography Lab

The Timeline does **not** cover generative, reactive, or emergent behaviors — those live in Choreography Lab (see `choreography-lab-spec.md`). The Timeline is where composed, recalled, and reproducible motion lives.

---

## Authority Chain

The Timeline occupies the second layer of the four-layer authority chain:

```
APVTS base state (DAW automation)
  + Timeline rest pose (when anim_enable=true)
    + Choreography Lab generative offset   ← ADR-0020 (see Documentation/adr/ADR-0020-...)
      + Physics additive offset
        = Final EmitterSlot position → DSP renderer
```

The Timeline never overrides DAW automation. It defines a rest pose contribution evaluated per `processBlock`. Physics applies as additive offset on top of the Timeline rest pose. When `anim_enable=false`, the Timeline contributes nothing and the APVTS base state is the sole rest input for physics.

---

## Current Surface (Phase 2.6 — Shipped)

### Data Model

Each emitter has its own `KeyframeTimeline` instance. Tracks are per-parameter. Evaluation runs in `processBlock`; all active track outputs combine to form the emitter's rest pose at the current playhead position.

```
KeyframeTimeline
├── tracks[]
│   ├── parameterId: string        APVTS parameter ID this track addresses
│   ├── trackType: enum            value | position | formation | procedural_path
│   ├── keyframes[]
│   │   ├── time: double           seconds (anim_sync=false) or beats (anim_sync=true)
│   │   ├── value: float           parameter value at this keyframe
│   │   └── curve: enum            Linear | EaseIn | EaseOut | EaseInOut | Step
│   └── interpolate(t) → float
├── duration: double               total timeline length
├── looping: bool                  forward loop; ping-pong mode also supported
├── playbackRate: float            0.1–10.0× (matches anim_speed APVTS parameter)
└── transportSync: bool            sync internal clock to DAW transport playhead
```

### APVTS Parameters (Existing)

| Parameter ID | Type | Range | Default | Description |
|---|---|---|---|---|
| `anim_enable` | bool | on/off | off | Enable/disable keyframe timeline evaluation |
| `anim_mode` | enum | DAW / Internal | DAW | Automation source: DAW or internal timeline |
| `anim_loop` | bool | on/off | off | Loop internal keyframe sequence |
| `anim_speed` | float | 0.1–10.0 | 1.0 | Playback rate multiplier (internal timeline only) |
| `anim_sync` | bool | on/off | on | Sync internal clock to DAW transport |

### Interpolation Curves (Existing)

| Curve | Behavior |
|---|---|
| `Linear` | Constant rate change between keyframes |
| `EaseIn` | Slow start, fast finish |
| `EaseOut` | Fast start, slow finish |
| `EaseInOut` | Slow start, fast finish, slow end |
| `Step` | Instant jump at keyframe time; hold until next |

### Shipped Editor Capabilities

- Add / move / delete keyframes (drag-to-edit in WebView)
- Curve cycle per keyframe (click to cycle through curve types)
- Transport controls: play / pause / rewind
- Multi-track lane view (one lane per parameter track)
- Position tracks rendered as 3D path curve in viewport

---

## Track Type System

Tracks are formally typed. The `trackType` field governs how keyframe values are interpreted and how the editor renders the track.

### Type 1 — Value Track

Keyframes a single float APVTS parameter over time. Any `emit_*`, `phys_*`, or `rend_*` float parameter is addressable. This is the generic track type that ships in Phase 2.6.

**Editor:** horizontal lane with keyframe diamonds; value axis labeled with parameter units.

### Type 2 — Position Track

A composite of three value tracks addressing `pos_x`, `pos_y`, `pos_z` (Cartesian) or `pos_azimuth`, `pos_elevation`, `pos_distance` (spherical). Rendered as a unified 3D path curve in the viewport rather than three separate lane graphs, though individual axis lanes remain editable in the track panel.

**Editor:** 3D ghost path visible in viewport at current playhead; path brightens at selected keyframe handles.

### Type 3 — Formation Track *(new — Choreography Lab graduation)*

Keyframes a formation descriptor for a group of emitters simultaneously. Rather than keyframing individual emitter positions, a formation track encodes `formation_type` + formation parameters at each keyframe. Interpolation between keyframes morphs the formation shape by linearly interpolating per-slot positions between source and target formations.

**Payload per keyframe:**
```
formation_type: enum            line | arc | circle | grid | spiral | sphere_surface | custom
formation_params: struct        varies by type (radius, rows, cols, spacing, etc.)
slot_assignments: int[]         which emitter slot occupies which formation position
```

**Behavior:** when a formation track is active on an emitter, that emitter's position is computed from the formation slot it is assigned — overriding any individual position value tracks on the same emitter for the duration of the formation track. Formation tracks and individual position value tracks must not overlap in time for the same emitter (validation error if they do).

**Editor:** formation shape wireframe preview in viewport at playhead position; morph ghost between adjacent keyframes; slot assignment UI for drag-assigning emitters to formation positions.

**Formation morph model distinction:** Timeline formation track keyframes define *endpoints* (what formation at time T). The standard keyframe curve enum (Linear, EaseIn, etc.) controls per-slot position interpolation between adjacent formation keyframes — this is the same curve mechanism used by all other track types, not a separate morph rate. Continuous formation animation with a configurable rate, loop, and ping-pong belongs to the Choreography Lab domain (`choro_formation_morph_rate`, `choro_formation_morph_loop`, `choro_formation_morph_pingpong`) — see `choreography-lab-spec.md §1`. They are composable: a Choreography Lab continuous morph can be baked into a Timeline formation track, after which the Timeline curve controls its interpolation.

**Parameters [new — pending parameter-spec.md]:** `anim_formation_type` (formation shape enum per keyframe).

### Type 4 — Procedural Path Track *(new — Choreography Lab graduation)*

Keyframes a procedural path type and its parameters. The emitter follows the analytical path; path parameters are themselves value tracks (keyframeable). Physics applies as additive offset on top of the computed path position.

**Payload per keyframe:**
```
path_type: enum                 lissajous | orbit | pendulum | figure_eight | helix | random_walk
path_params: struct             varies by type (frequency ratios, radius, period, etc.)
```

**Relationship to Choreography Lab `pendulum` path:** identical closed-form analytical solution. The distinction is surface: in Choreography Lab the path runs generatively; promoted to a Path Track it becomes a composed Timeline asset with keyframeable parameter evolution.

**Editor:** ghost path curve drawn in viewport for the selected key range; parameter evolution shown as value lanes below; ghost updates in real-time as parameters are adjusted.

**Parameters [new — pending parameter-spec.md]:** `anim_path_type`, per-path-type parameter set (mirrors `choro_*` path parameters).

---

## Beat-Sync Step Mode *(new — Choreography Lab graduation)*

Any value track or position track can be set to beat-quantize mode. In this mode, keyframe times snap to the host beat grid at `anim_beat_division` resolution.

**New curve types added to the curve enum:**

| Curve | Behavior |
|---|---|
| `Glide` | Smooth interpolation; arrives exactly on the beat boundary |
| `Teleport` | Instant position change on beat; gain-dip mask applied (see below) |

**Teleport gain-dip:** when a keyframe with `curve=Teleport` is evaluated, the Timeline writes a gain-dip envelope to `EmitterSlot.gain` — identical mechanism to Choreography Lab beat-sync (same `choro_teleport_dip_db` and `choro_teleport_decay_ms` parameters reused). Finite-safe: rate-limited, gain clamped to [0..1].

**Parameters [new — pending parameter-spec.md]:** `anim_beat_division: enum {bar, beat, 1/2, 1/4, 1/8, 1/16, 1/32}`, `anim_beat_mode: enum {snap, glide, teleport}`.

---

## Bake Import from Choreography Lab

When the operator bakes a Choreography Lab session (see `choreography-lab-spec.md §6`), the result is written into the Timeline as standard value tracks (position x/y/z per emitter per bake range). No new track type is required for baked content.

**Track provenance metadata** (non-functional, editor-display only):
```
track.source: enum              manual | choreography_bake | daw_import
track.bake_timestamp: string    ISO 8601 timestamp of the bake operation
track.bake_params: struct       bake_kf_density, bake_curve_fit_tolerance used
```

**Operator interaction:** baked tracks appear in the Timeline editor with a visual badge (`BAKE`). They are fully editable after import — operator can add, move, or delete keyframes, change curves, and extend beyond the baked range. The `source` metadata is immutable after bake and serves as provenance only.

**Authority after import:** baked tracks follow the standard Timeline authority rules — they define rest pose; physics applies on top; DAW automation overrides.

---

## Conflict Resolution Rules

When two tracks in the same timeline address the same parameter for the same emitter at the same time (overlapping ranges), the last-added track wins. The editor warns on overlap creation. Formation tracks and individual position tracks must not overlap — this is a hard validation error surfaced in the editor before playback.

When `anim_mode=DAW`, the internal Timeline does not evaluate regardless of `anim_enable`. DAW automation is sole authority.

When `anim_mode=Internal` and `anim_enable=true`, the Timeline rest pose is evaluated and written to `EmitterSlot`. If a parameter has no active track at the current playhead position, the APVTS base value is used for that parameter (no track = no contribution).

---

## Threading Contract

- `KeyframeTimeline::evaluate(time)` is called once per `processBlock` on the audio thread.
- Evaluation is read-only against the keyframe data. No allocation, no mutex, no I/O.
- Keyframe data mutations (add/delete/move) happen on the message thread (UI) and are double-buffered: the editor writes to a pending copy, and the audio thread swaps to the new copy at a safe point between blocks.
- Formation track evaluation: per-slot position interpolation computed on the audio thread from pre-computed formation geometry. Formation geometry is computed on the message thread when formation parameters change and swapped atomically.
- Procedural path evaluation: closed-form analytical computation on the audio thread. No iterative solver; no divergence risk.

---

## Parameter Namespace

New parameters introduced by Timeline extensions:

| Parameter | Type | Track type | Description |
|---|---|---|---|
| `anim_beat_division` | enum | beat-sync | beat grid resolution |
| `anim_beat_mode` | enum | beat-sync | snap / glide / teleport |
| `anim_formation_type` | enum | formation | formation shape at keyframe |
| `anim_path_type` | enum | procedural path | path type |

All **[new — pending parameter-spec.md]** under the `anim_*` namespace.

---

## Containment Governance (BL-094)

| Feature | Source | Current state | Promotion trigger |
|---|---|---|---|
| Value Track | Phase 2.6 | Shipped | — |
| Position Track | Phase 2.6 | Shipped | — |
| Formation Track | Choreography Lab §1 graduation | Production-candidate | Mix utility in ≥3 sessions |
| Procedural Path Track | Choreography Lab §2 graduation | Production-candidate | Operator requests in production |
| Beat-Sync Step Mode | Choreography Lab §3 graduation | Production-candidate | Validates in ≥1 DAW host |
| Bake Import | Choreography Lab §6 graduation bridge | Ships with Lab | Enables all graduation paths |

---

## Validation Status

Not tested — spec only (extensions).

Phase 2.6 (shipped surface) validation status: **tested** — all Phase 2.6 acceptance criteria passed (see `.ideas/plan.md` §Phase 2.6). CPU: `perf_avg_block_time_ms=0.304`, allocation-free pass, host matrix (44.1k/48k/96k × 256/512/1024) all pass.

**Extension acceptance gates (before production promotion):**

- Formation Track: morph position error < 1mm vs analytical target at morph completion; no overlap validation error under adversarial concurrent track creation
- Procedural Path Track: CPU overhead < 0.5ms/emitter at 240Hz evaluation rate; path position matches analytical reference within floating-point tolerance
- Beat-Sync Step Mode: keyframe snap timing within ±2ms of beat boundary across 4 DAW hosts; Teleport gain-dip finite-safe under 100 rapid sequential teleports
- Bake Import: round-trip fidelity — reconstructed path RMS error < `bake_curve_fit_tolerance` meters; provenance metadata intact after save/restore cycle
- Double-buffer swap: no audio glitch on keyframe edit during playback (tested in pluginval with concurrent UI edits)
