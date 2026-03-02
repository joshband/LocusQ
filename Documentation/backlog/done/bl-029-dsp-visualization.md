---
Title: BL-029 DSP Visualization and Tooling
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02
---

# BL-029 — DSP Visualization and Tooling

## Plain-Language Summary

This runbook tracks **BL-029** (BL-029 — DSP Visualization and Tooling). Current status: **Done (2026-02-25 promotion packet Z4; reliability hard criteria satisfied on integrated owner replays)**. In plain terms: This runbook defines a scoped change with explicit validation and evidence requirements.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-029 — DSP Visualization and Tooling |
| Why is this important? | This runbook defines a scoped change with explicit validation and evidence requirements. |
| How will we deliver it? | Use the runbook steps, validation lanes, and evidence expectations to deliver and verify the work safely. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-029-dsp-visualization.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Promotion gate table | Shows what passed/failed for closeout decisions. | `## Promotion Gate Summary` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## 1. Summary

Deliver four visualization and tooling priorities for the LocusQ WebView UI: (1) a deterministic modulation visualizer driven by an SPSC ring buffer, (2) a spectral-spatial hybrid room view, (3) first-order reflection ghost modeling, and (4) an aspirational offline ML calibration assistant. Eight implementation slices, heavy dependency chain.

| Field | Value |
|---|---|
| ID | BL-029 |
| Status | Done (2026-02-25 promotion packet Z4; reliability hard criteria satisfied on integrated owner replays) |
| Priority | P2 |
| Track | B — Scene/UI Runtime |
| Effort | Very High / XL |
| Depends | BL-025, BL-026, BL-027, BL-028, BL-031 |
| Blocks | none |
| Annex | `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md` |

---

## 1.1 Execution Snapshot (2026-02-25)

| Lane | Result | Evidence | Notes |
|---|---|---|---|
| Audition Quality Slice D | PASS | `TestEvidence/bl029_audition_quality_slice_d_20260224T205057Z/status.tsv` | Native renderer audition-motion quality upgrades validated (`build`, `qa_smoke`, `rt_audit`). |
| Audition Cloud UI Slice E | PASS | `TestEvidence/bl029_audition_cloud_slice_e_20260224T205041Z/status.tsv` | Multi-emitter showcase UI path validated (`node_check`, standalone build, scoped selftest). |
| Audition Cloud Bridge Slice F | PASS | `TestEvidence/bl029_audition_cloud_slice_f_20260224T205313Z/status.tsv` | Additive `rendererAuditionCloud` metadata contract (`mode`, `emitterCount`, `emitters[]`) validated. |
| Owner integrated replay (D+E+F) | PASS | `TestEvidence/owner_bl029_post_bl026_fix_r2_20260224T214701Z/status.tsv` | Combined replay passes `node/build/qa/rt/selftest/docs` after BL-026 scoped selftest timeout remediation and RT allowlist refresh. |
| Audition Platform Slice A1 | PASS (owner gate recheck) | `TestEvidence/bl029_audition_platform_slice_a1_20260224T223946Z/status.tsv`; `TestEvidence/owner_rt_allowlist_refresh_20260224T224531Z/status.tsv` | Worker lane already had `build/qa/selftest` PASS; owner reconciled RT allowlist line drift and restored `non_allowlisted=0` gate. |
| Audition Platform Slice A2 | PASS (owner rerun) | `TestEvidence/bl029_audition_platform_slice_a2_20260224T224500Z/status.tsv`; `TestEvidence/owner_bl029_slice_a2_recheck_20260224T224946Z/status.tsv`; `TestEvidence/owner_bl029_selftest_stability_20260224T224740Z/status.tsv` | Worker lane saw transient `app_exited_before_result`; owner rerun and stability loop passed (`selftest_bl029` 5/5). |
| Audition Platform Slice A3 | PASS | `TestEvidence/bl029_audition_platform_slice_a3_20260224T225104Z/status.tsv` | Deterministic replay-hash lane, proxy/contract checks, RT audit, and docs-freshness lane are green. |
| Audition Platform Slice B1 | PASS (owner gate recheck) | `TestEvidence/bl029_audition_platform_slice_b1_20260224T230303Z/status.tsv`; `TestEvidence/owner_rt_allowlist_refresh_20260224T231523Z/status.tsv` | Worker lane had `build/qa/selftest/docs` PASS and failed only on RT line-map drift; owner refreshed allowlist baseline (`non_allowlisted=0`). |
| Audition Platform Slice C1 | PASS (owner rerun) | `TestEvidence/bl029_audition_platform_slice_c1_20260224T230341Z/status.tsv`; `TestEvidence/locusq_production_p0_selftest_20260224T231504Z.json`; `TestEvidence/owner_rt_allowlist_refresh_20260224T231523Z/status.tsv` | Worker lane had one transient scoped selftest abort and RT line-map drift; owner reruns pass (`selftest_bl029` 3/3) and RT gate restored. |
| Audition Platform Slice E1 | PASS (owner gate recheck) | `TestEvidence/bl029_audition_platform_slice_e1_20260224T230316Z/status.tsv`; `TestEvidence/owner_rt_allowlist_refresh_20260224T231523Z/status.tsv` | QA lane and docs freshness passed in worker run; only RT baseline drift remained and was resolved by owner allowlist refresh. |
| Audition Reactive Envelope Bridge Slice G1 | FAIL (worker gate) | `TestEvidence/bl029_audition_reactive_bridge_slice_g1_20260224T232748Z/status.tsv` | `build`, `qa_smoke`, `selftest_bl029`, and docs freshness pass; fails only on RT allowlist drift (`non_allowlisted=87`). |
| Audition Reactive Cloud UI Slice G2 | FAIL (worker gate) | `TestEvidence/bl029_audition_reactive_ui_slice_g2_20260224T232957Z/status.tsv` | `node --check` + standalone build pass; scoped selftest failed in worker lane with transient `app_exited_before_result (134)`. |
| Reactive Contract QA Slice G3 | FAIL (worker gate) | `TestEvidence/bl029_audition_reactive_qa_slice_g3_20260224T232807Z/status.tsv` | QA lane and docs freshness pass; fails only on RT allowlist drift (`non_allowlisted=87`). |
| Binaural Reactive Parity Slice G4 | FAIL (worker gate) | `TestEvidence/bl029_audition_binaural_parity_slice_g4_20260225T000805Z/status.tsv` | `build`, `selftest_bl029`, `selftest_bl009`, and BL-009 headphone contract lane pass; fails on RT allowlist drift (`non_allowlisted=91`). |
| Physics-Reactive Audition Coupling Slice G5 | FAIL (worker gate) | `TestEvidence/bl029_audition_physics_coupling_slice_g5_20260225T001225Z/status.tsv` | `build` + QA smoke pass; worker saw transient `selftest_bl029` abort (`app_exited_before_result`) and RT allowlist drift (`non_allowlisted=110`). |
| Cinematic Reactive Preset Language Slice G6 | PASS | `TestEvidence/bl029_cinematic_reactive_preset_language_slice_g6_20260225T000954Z/status.tsv` | Dream/Design/Plan contract docs landed with acceptance IDs and docs-freshness gate pass. |
| Reactive UI/QA Consolidation Slice Z3 | PASS | `TestEvidence/bl029_reactive_ui_z3_20260225T004027Z/status.tsv` | `node_check`, standalone build, scoped selftest (5/5), and docs-freshness lane pass with acceptance checks `UI-P1-029A/B/C`. |
| Owner G-phase triage reruns | PARTIAL PASS | `TestEvidence/owner_bl029_g123_triage_20260225T000428Z/status.tsv`; `TestEvidence/owner_rt_allowlist_refresh_20260225T000548Z/status.tsv`; `TestEvidence/owner_rt_allowlist_refresh_20260225T000647Z/status.tsv` | Owner rerun shows scoped `selftest_bl029` pass and docs freshness pass after metadata fix; RT gate remains a moving target while concurrent edits continue to shift line maps. |
| Owner G4 triage replay | PARTIAL PASS | `TestEvidence/owner_bl029_g4_triage_20260225T001418Z/status.tsv` | Owner replay confirms G4 functional lanes pass (`build`, both scoped selftests, BL-009 QA); RT gate still drifts and docs gate briefly failed from G5 evidence metadata omission (now fixed). |
| Owner G5 triage replay | PARTIAL PASS | `TestEvidence/owner_bl029_g5_triage_20260225T001834Z/status.tsv` | Owner replay clears worker selftest abort concern (`selftest_bl029` passes 3/3); RT gate still fails from global allowlist drift (`non_allowlisted=110`). |
| Owner Z2 replay (post-handoff) | PARTIAL PASS | `TestEvidence/owner_bl029_z2_replay_20260225T013944Z/status.tsv` | Owner replay clears worker abort concern (`selftest_bl029` 3/3 PASS, `selftest_bl009` PASS, `qa_bl009_headphone_contract` PASS); RT gate still fails after line-map movement (`non_allowlisted=103`). |
| Owner Z2+Z3 integrated replay | PARTIAL PASS | `TestEvidence/owner_bl029_z2_z3_integrated_20260225T014638Z/status.tsv` | All functional lanes pass (`node/build/qa/smoke/selftest_bl029/selftest_bl009/qa_bl009/docs`); RT gate fails on drift (`non_allowlisted=103`) before final reconciliation. |
| Owner RT finalizer post-Z2/Z3 | PASS | `TestEvidence/owner_bl029_rt_finalize_z2z3_20260225T014808Z/status.tsv` | Freeze guard pass and RT gate stabilized (`non_allowlisted=0`) with docs freshness pass. |
| Reliability Hardening Slice R1 | FAIL | `TestEvidence/bl029_reliability_native_r1_20260225T031132Z/status.tsv` | Build/smoke pass and `selftest_bl029` passes 5/5, but BL009 scoped selftest lane is flaky (`1/3` fail in required set). |
| Selftest Harness Robustness Slice R2 | FAIL | `TestEvidence/bl029_selftest_harness_r2_20260225T030714Z/status.tsv` | Diagnostics improved, but runtime remains unstable in soak (`bl029 0/10`, `bl009 0/5`, repeated `app_exited_before_result`, `exit 134`, `ABRT`). |
| Reliability Soak + Go/No-Go Slice R3 | FAIL (NO-GO) | `TestEvidence/bl029_reliability_soak_r3_20260225T030749Z/status.tsv` | QA contract lane deterministic PASS, but soak hard gates fail (`selftest_bl029 1/10`, `selftest_bl009 1/5`), decision `NO-GO`. |
| Selftest Serialization Slice S1 | PASS | `TestEvidence/bl029_selftest_serialization_s1_20260225T033732Z/status.tsv` | Reliability harness serialization/locking lane now passes full matrix (`selftest_bl029 10/10`, `selftest_bl009 10/10`, wrapper lane pass, docs pass). |
| ABRT Root-Cause Probe Slice S2 | PASS (diagnostic lane) | `TestEvidence/bl029_abrt_probe_s2_20260225T033613Z/status.tsv` | Probe tooling and taxonomy capture pass; captures earlier crash signature (`appkit_registration`, `app_exited_before_result`) for historical triage. |
| BL009 UI Determinism Fix Slice S3 | PASS | `TestEvidence/bl029_bl009_ui_determinism_s3_20260225T033550Z/status.tsv` | BL009 compact-rail determinism lane now passes soak matrix (`selftest_bl009 10/10`, `selftest_bl029 5/5`) and records fail-rate reduction from R3 baseline (`BL009 80% -> 0%`, `BL029 90% -> 0%` in scoped set). |
| Owner Reliability Replay (post-S1/S2) | PASS (GO) | `TestEvidence/owner_bl029_reliability_resume_20260225T150335Z/status.tsv` | Current-branch replay meets R3 hard criteria (`qa lane PASS`, `selftest_bl029 10/10`, `selftest_bl009 5/5`, docs pass), decision `GO`. |
| Reliability Gate Runner Slice P4 | PASS | `TestEvidence/bl029_reliability_gate_p4_20260225T152220Z/status.tsv`; `TestEvidence/bl029_reliability_gate_p4_20260225T152907Z/status.tsv` | Deterministic reliability wrapper lane passes hard criteria in one command (`build`, `qa lane`, `selftest_bl029 10/10`, `selftest_bl009 5/5`, docs freshness); owner replay confirms pass on integration tree. |
| Reactive Geometry Mapping Slice P5 | FAIL (worker gate; owner rerun PASS) | `TestEvidence/bl029_reactive_geometry_p5_20260225T152336Z/status.tsv`; `TestEvidence/owner_bl029_p5p6_reconcile_20260225T152901Z/selftest_bl029_runs.tsv` | Worker lane reported scoped selftest failures (`app_exited_before_result`, `app_exit_code=127`), but owner replay on current integration tree passes scoped selftest stability (`5/5`, `status=pass`, `ok=true`). |
| Native Reactive Envelope Bridge Slice P6 | PASS (owner gate recheck) | `TestEvidence/bl029_native_reactive_p6_20260225T152204Z/status.tsv`; `TestEvidence/owner_bl029_p5p6_reconcile_20260225T152901Z/status.tsv` | Worker lane passed functional gates and failed only RT drift (`non_allowlisted=118`); owner reconciliation refreshed allowlist and restored RT gate (`non_allowlisted=0`). |
| Promotion Packet Slice Z4 | PASS | `TestEvidence/bl029_promotion_packet_z4_20260225T153637Z/status.tsv` | Promotion decision packet records DONE disposition with explicit hard-criteria references and docs-freshness gate pass. |

Current owner disposition: BL-029 is `Done` as of 2026-02-25. Promotion is based on the latest integrated hard-criteria proofs (`owner_bl029_reliability_resume_20260225T150335Z`, `bl029_reliability_gate_p4_20260225T152907Z`, `owner_bl029_p5p6_reconcile_20260225T152901Z`, and `bl029_promotion_packet_z4_20260225T153637Z`). Earlier `NO-GO` evidence (R1/R2/R3) remains preserved as historical regression context.

---

## 1.2 Audition Platform Expansion (Hybrid + Cinematic)

User intent for the next tranche is explicit:
1. Hybrid audition role (demo/showcase + diagnostic utility).
2. Significant quality upgrade on existing controls.
3. Cinematic immersive behavior as the default experience posture.

Authority decision:
1. Renderer remains audition DSP authority.
2. Emitter/choreography/physics panels provide cross-mode proxy controls ("Audition This").
3. Deterministic standalone behavior is a first-class requirement.

Normative references for this expansion:
1. Plan: `Documentation/plans/bl-029-audition-platform-expansion-plan-2026-02-24.md`
2. ADR: `Documentation/adr/ADR-0013-audition-authority-and-cross-mode-control.md`

---

## 2. Objective

The LocusQ Three.js UI currently renders a static scene graph without DSP-reactive feedback. This item makes the scene data-driven:

1. **Modulation Visualizer** — a base-vs-applied DSP overlay driven by a lock-free SPSC ring buffer that is written by processBlock() and consumed by the UI poll. Deterministic and RT-safe.
2. **Spectral-Spatial Room View** — a spectral centroid/rolloff/flux computation in processBlock() feeding a spatial "heat map" overlay on the Three.js scene.
3. **Reflection Ghost Modeling** — first-order image-source reflection geometry computed on the message thread and rendered as translucent ghost emitters in the Three.js viewport.
4. **Offline ML Calibration Assistant** — aspirational: an export path from the plugin session and an offline Python/CLI tool that suggests calibration parameters from the exported data.

RT invariant is absolute: Slices A, C, E, and G touch processBlock() or introduce new producer paths; all must be RT-safe. Slices B, D, F, and H are UI-side and must not block the audio thread.

---

## 3. Normative References

- `Source/PluginProcessor.cpp` — processBlock() RT producer path, bridge serialization
- `Source/SpatialRenderer.h` — spatial profile and renderer domain types
- `Source/ui/public/js/index.js` — Three.js scene, UI event handlers, bridge calls
- `Source/EarlyReflections.h` — early reflection geometry (if present), image source model
- `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md` — full slice spec
- `Documentation/invariants.md` — RT invariant: no alloc/lock/blocking in processBlock()
- `Documentation/scene-state-contract.md` — bridge payload schema
- `Documentation/adr/ADR-0006.md` — device profile authority
- `Documentation/adr/ADR-0012.md` — renderer domain exclusivity

---

## 4. Entry Criteria

### Global Entry
- BL-025 (scene graph and bridge foundation) merged.
- BL-026 Slice A (alias dictionary) merged.
- BL-027 Slice E (RENDERER cross-panel coherence) merged.
- BL-028 Slice D (domain/tracking telemetry in bridge payload) merged.
- BL-031 (tempo-locked visual token scheduler) merged or stub is available.
- `status.json` reflects all dependencies in done/verified state.
- No open RT-safety violations in `TestEvidence/build-summary.md`.

### Per-Slice Entry
| Slice | Entry Gate |
|---|---|
| A | Global entry; no prior slice dependency |
| B | Slice A merged; ModulationTraceRing compiles and passes unit test |
| C | Slices A-B merged or running in parallel (C is independent of B) |
| D | Slice C merged; spectral fields in bridge payload |
| E | Slices A-D merged; EarlyReflections.h (or stub) present |
| F | Slice E merged; reflection geometry in bridge payload |
| G | Slices A-F merged |
| H | Slice G merged; aspirational — defer if timeline slips |

---

## 5. Slices

### Slice A — SPSC Ring Buffer for Modulation Trace

**Goal:** Implement a lock-free single-producer/single-consumer ring buffer (`ModulationTraceRing`) in a new header `Source/ModulationTraceRing.h`. processBlock() writes modulation trace frames (base_value, applied_value, parameter_id, sample_offset) at the audio thread rate. The UI poll thread reads available frames without blocking the audio thread.

**Files:** `Source/PluginProcessor.cpp`, new `Source/ModulationTraceRing.h`

**Ring design constraints:**
- Fixed capacity (power of two, e.g., 4096 frames).
- Single producer (audio thread), single consumer (message/poll thread).
- No dynamic allocation after construction.
- Write drops frame silently if ring is full (non-blocking producer).
- Reader drains available frames between poll cycles.

**Trace frame struct:**
```cpp
struct ModulationTraceFrame {
    uint16_t parameter_id;
    float    base_value;
    float    applied_value;
    uint32_t sample_offset; // offset within current buffer
};
```

**Acceptance:**
- ModulationTraceRing compiles with `-Wall -Wextra` and no warnings.
- Unit test: producer writes 8192 frames, consumer reads all; no deadlock, no lost frames up to ring capacity.
- processBlock() writes frames without any lock or allocation.
- UI-P2-029A lane passes.

---

### Slice B — Modulation Visualizer UI

**Goal:** A Three.js overlay in the EMITTER panel showing a scrolling time-series of base (grey) vs applied (colored by parameter) DSP values. The visualizer consumes frames from the bridge payload's `modulation_trace` array. Scrolls at the current tempo or at a fixed 60 fps fallback.

**Files:** `Source/ui/public/js/index.js`

**Bridge payload addition:**
```json
{
  "modulation_trace": [
    { "parameter_id": 3, "base_value": 0.5, "applied_value": 0.63, "sample_offset": 0 },
    ...
  ]
}
```

**Acceptance:**
- Visualizer renders without frame drops at 60 fps on reference hardware (M1 MacBook Air).
- Base vs applied lines are visually distinct.
- Visualizer is toggled on/off via a UI control without reloading the scene.
- UI-P2-029B lane passes.

---

### Slice C — Spectral Centroid/Rolloff/Flux Computation

**Goal:** In processBlock(), compute three spectral features per buffer: spectral centroid (Hz), spectral rolloff (Hz at 85% energy), and spectral flux (frame-to-frame magnitude change). Write results to a lock-free atomic snapshot readable by the bridge serialization path on the message thread.

**Files:** `Source/PluginProcessor.cpp`

**Implementation notes:**
- Use a simple DFT or magnitude approximation; full FFT is acceptable if using a fixed-size power-of-two window already available.
- Spectral computation must not allocate. Pre-allocate the magnitude buffer at construction time.
- Atomic snapshot pattern: write to a double-buffer or std::atomic struct, message thread reads the last complete frame.

**Acceptance:**
- Spectral features are present in bridge payload as `spectral.centroid_hz`, `spectral.rolloff_hz`, `spectral.flux`.
- No allocation in processBlock() for spectral computation path.
- Feature values are stable (non-NaN, non-Inf) for silence and for a 440 Hz sine input.
- UI-P2-029C lane passes.

---

### Slice D — Spectral-Spatial Room View UI

**Goal:** A Three.js "heat map" overlay on the 3D room view that colors spatial zones by spectral centroid value. Low centroid = cool (blue), high centroid = warm (red/orange). Rolloff and flux drive opacity and animation speed respectively.

**Files:** `Source/ui/public/js/index.js`

**Acceptance:**
- Heat map updates within one frame of a spectral bridge payload change.
- Heat map is rendered as a semi-transparent mesh overlay, not as DOM elements.
- Toggled on/off independently of the modulation visualizer.
- UI-P2-029D lane passes.

---

### Slice E — Reflection Ghost Geometry Computation

**Goal:** Compute first-order image-source reflection positions for up to six room surfaces on the message thread (not in processBlock()). Each reflection is represented as a `GhostEmitter` with position (x, y, z), surface normal, attenuation, and delay_ms. Results are written to the bridge payload as `reflections` array.

**Files:** `Source/EarlyReflections.h` (or new file if absent), new `Source/ReflectionGhostMapper.h`

**GhostEmitter struct:**
```cpp
struct GhostEmitter {
    float x, y, z;
    float nx, ny, nz;  // surface normal of reflection wall
    float attenuation; // 0.0 - 1.0
    float delay_ms;
};
```

**Computation trigger:** Room geometry change event (APVTS listener) or emitter position change. Not triggered every processBlock() call. Debounced to at most 30 Hz.

**Acceptance:**
- Up to 6 first-order ghosts per emitter for a rectangular room.
- Ghost positions are geometrically correct (image source method).
- Bridge payload includes `reflections` array.
- Computation does not occur in processBlock().
- UI-P2-029E lane passes.

---

### Slice F — Reflection Ghost Viewport Rendering

**Goal:** Render GhostEmitter objects in the Three.js viewport as translucent spheres with direction arrows pointing toward the listener. Opacity driven by attenuation; sphere scale by delay_ms (farther = slightly larger ghost). Ghost emitters animate with a subtle pulse at 1 Hz.

**Files:** `Source/ui/public/js/index.js`

**Acceptance:**
- Ghosts are rendered in a separate Three.js layer (not occluded by room walls).
- Ghost opacity and scale reflect attenuation and delay_ms values from bridge payload.
- Toggle on/off independently.
- Frame rate remains >= 60 fps with 6 ghosts active.
- UI-P2-029F lane passes.

---

### Slice G — Export Session Data for Offline Analysis

**Goal:** Add a "Export Session" action to the EMITTER panel that writes a JSON snapshot of the current session to a user-selected file path. Snapshot includes: room geometry, emitter positions, active profile, spectral features (last 60 seconds of 1-Hz samples), and head tracking history.

**Files:** `Source/PluginProcessor.cpp`, `Source/ui/public/js/index.js`

**Export payload schema (abbreviated):**
```json
{
  "schema_version": "1.0",
  "exported_at": "ISO8601",
  "profile": { "id": "airpods_pro_2", "label": "AirPods Pro 2" },
  "room": { "width": 5.0, "height": 3.0, "depth": 4.0 },
  "emitters": [...],
  "spectral_history": [...],
  "tracking_history": [...]
}
```

**Acceptance:**
- Export completes without audio dropout (write is on message thread, not audio thread).
- File is valid JSON parseable by Python `json.loads()`.
- Export action is accessible via keyboard shortcut (Ctrl/Cmd+E) and UI button.
- UI-P2-029G lane passes.

---

### Slice H — Offline ML Calibration Assistant (Aspirational)

**Goal:** A standalone Python CLI tool (`tools/calibration_assistant/calibrate.py`) that reads an exported session JSON and recommends calibration parameter adjustments using a simple regression or heuristic model. Outputs a parameter patch JSON compatible with the PluginProcessor parameter schema.

**Files:** new `tools/calibration_assistant/` directory

**Status:** Aspirational. Defer if timeline slips. Does not block closeout of BL-029 if remaining slices A-G pass.

**Acceptance (if pursued):**
- CLI accepts `--input session.json` and `--output patch.json`.
- Patch JSON is structurally valid (schema validated by a bundled JSON schema).
- At least one calibration heuristic is implemented (e.g., spectral centroid rolloff suggests room absorption adjustment).

---

## 6. ADR Obligations

| ADR | Obligation |
|---|---|
| ADR-0006 | Device profile authority: spectral and reflection computations must not change the active device profile |
| ADR-0012 | Renderer domain exclusivity: reflection computation is domain-agnostic but must not activate a different domain |
| ADR-0013 | Audition remains renderer-authoritative; cross-mode controls are proxy writers only |

If the spectral computation or reflection ghost paths require a new architectural decision (e.g., whether spectral features are part of the RT data path or message-thread-only), record a new ADR.

---

## 7. RT-Safety Checklist

For every code change touching `Source/PluginProcessor.cpp` in Slices A, C, and G:

- [ ] No `new` / `delete` / `malloc` / `free` in processBlock()
- [ ] No `std::mutex`, `std::lock_guard`, or any blocking primitive in processBlock()
- [ ] ModulationTraceRing write path is lock-free (atomic index compare-and-swap)
- [ ] Spectral computation magnitude buffer pre-allocated at construction time
- [ ] No file I/O in processBlock() (Slice G export is on message thread only)
- [ ] Reflection ghost computation (Slice E) triggered from APVTS listener, not processBlock()
- [ ] Bridge payload serialization (all slices) occurs on message thread

---

## 8. Validation Lanes

| Lane | Trigger | Pass Criteria |
|---|---|---|
| UI-P2-029A | Slice A merge | ModulationTraceRing unit test passes; no lock in processBlock() write path |
| UI-P2-029B | Slice B merge | Modulation visualizer renders at >= 60 fps; toggle works |
| UI-P2-029C | Slice C merge | Spectral features in bridge payload; no NaN/Inf for silence and 440 Hz sine |
| UI-P2-029D | Slice D merge | Heat map renders and updates on spectral change |
| UI-P2-029E | Slice E merge | 6 ghost positions correct for a 5x3x4m rectangular room |
| UI-P2-029F | Slice F merge | Ghosts render at >= 60 fps; opacity/scale driven by attenuation/delay |
| UI-P2-029G | Slice G merge | Export JSON is valid; no audio dropout during export |
| SCHEMA | Each payload-adding slice | scene-state-contract.md updated to document new fields |
| FRESHNESS | Each slice merge | `./scripts/validate-docs-freshness.sh` exits 0 |

---

## 9. Risks and Mitigations

| Risk | Severity | Mitigation |
|---|---|---|
| Heaviest dependency chain in P2 — any upstream slip blocks entry | High | Start Slices A and C in parallel once global entry criteria are met; they are independent |
| SPSC ring overflow under high modulation rate | High | Ring drops frames silently; UI visualizer interpolates gaps; document drop rate in diagnostics |
| Slice H is aspirational; timeline pressure may cut it | Medium | Slices A-G are standalone complete; H is explicitly gated as aspirational |
| Spectral FFT size impacts RT budget | Medium | Use a fixed 512-sample magnitude approximation first; full FFT only if quality requires it |
| Reflection ghost geometry diverges from Steam Audio's own early reflection model | Medium | Document that ghosts are visualization-only; do not feed ghost positions back to Steam Audio |
| BL-031 tempo scheduler not ready at BL-029 start | Medium | Modulation visualizer falls back to fixed 60 fps; integrate tempo sync in a follow-up |

---

## 10. Effort and Sequencing

| Slice | Effort | Parallelizable | Notes |
|---|---|---|---|
| A | M | Yes (with C) | RT-safe foundation |
| B | M | After A | UI consumer of A |
| C | M | Yes (with A) | Independent RT producer |
| D | M | After C | UI consumer of C |
| E | L | After A-D | Most complex geometry |
| F | M | After E | UI consumer of E |
| G | S | After A-F | Export path |
| H | XL | After G | Aspirational |

Recommended approach: Run A and C in the same sprint. Run B and D in the next sprint. E and F in the following. G at closeout. H is post-v1.

---

## 11. Agent Mega-Prompts

### Slice A — Skill-Aware Prompt

```
Skills: $reactive-av $skill_impl

Context:
- You are implementing BL-029 Slice A in the LocusQ JUCE spatial audio plugin.
- RT invariant: processBlock() must never allocate, lock, or block.
- The modulation trace ring must be a fixed-capacity SPSC (single-producer
  single-consumer) ring buffer with no dynamic allocation after construction.
- Source/PluginProcessor.cpp is the audio processor; processBlock() is the producer.
- The UI poll thread (message thread timer) is the consumer.

Task:
1. Create Source/ModulationTraceRing.h with:
   - ModulationTraceFrame struct: { uint16_t parameter_id; float base_value;
     float applied_value; uint32_t sample_offset; }
   - ModulationTraceRing<N> template (N must be a power of two):
     - write(ModulationTraceFrame): lock-free, drops if full, never blocks.
     - read_available(ModulationTraceFrame* out, int max_count) -> int: reads up to
       max_count frames, returns actual count read. Lock-free. Called from consumer.
     - size_t available() const: number of unread frames.
   - Use std::atomic<size_t> for head and tail indices.
   - Memory ordering: producer uses release on tail update; consumer uses acquire
     on tail read.

2. In Source/PluginProcessor.cpp:
   a. Add a member: ModulationTraceRing<4096> m_modulation_ring;
   b. In processBlock(), after applying modulation to each parameter, write a
      ModulationTraceFrame per modulated parameter.
   c. Confirm: no new/delete/malloc, no mutex, no lock_guard in processBlock().

3. Write a unit test (inline or in a test harness file):
   - Producer writes 8192 frames; consumer reads all; verify no frames lost up
     to ring capacity (4096), and that overflow beyond capacity results in silent
     drops only.
   - Test compiles and passes.

4. Run UI-P2-029A validation lane.
5. Update status.json: BL-029 slice_a = "done".
6. List changed files, test result, validation result.

Constraints:
- ModulationTraceRing.h must include no JUCE headers (portable C++17).
- No dynamic allocation in the ring implementation.
- write() must return immediately even if full.
```

### Slice A — Standalone Fallback Prompt

```
Context (no skills loaded):
- Repository: LocusQ — JUCE VST3/AU/CLAP spatial audio plugin.
- processBlock() in Source/PluginProcessor.cpp is the audio thread hot path.
  It must never allocate, lock, or block.
- You need a fixed-capacity SPSC ring buffer for modulation trace frames.

Task:
1. Create Source/ModulationTraceRing.h.
   Include only standard C++ headers (atomic, cstdint, cstddef).
   Define:
     struct ModulationTraceFrame {
         uint16_t parameter_id;
         float base_value;
         float applied_value;
         uint32_t sample_offset;
     };
   Define a template class ModulationTraceRing<size_t N> where N is power of two.
   Implement:
     bool write(const ModulationTraceFrame& f); // returns false if full, never blocks
     int read_available(ModulationTraceFrame* out, int max_count); // consumer thread
   Use std::atomic<size_t> for head_ and tail_ with appropriate memory ordering.

2. In Source/PluginProcessor.cpp:
   Add member: ModulationTraceRing<4096> m_modulation_ring;
   In processBlock(), call m_modulation_ring.write(...) for each modulated parameter.
   Do not call any allocating or locking functions in processBlock().

3. Describe in a comment block at the top of ModulationTraceRing.h:
   - Memory ordering rationale (why acquire/release on tail).
   - Drop behavior (silent, non-blocking).

4. List all files changed with a one-sentence description per file.
```

### Slice B — Agent Prompt

```
Skills: $reactive-av $threejs $juce-webview-runtime

Context:
- BL-029 Slice A is merged. ModulationTraceRing is live.
- The bridge payload now includes modulation_trace array (populated by message
  thread draining ModulationTraceRing on each poll).
- Source/ui/public/js/index.js hosts the Three.js scene and EMITTER panel.

Task:
1. In PluginProcessor.cpp message-thread timer, drain m_modulation_ring and
   add frames to the bridge payload as modulation_trace array (max 256 frames
   per poll to avoid large payloads).
2. In index.js, implement a ModulationVisualizer component:
   - Renders a canvas overlay in the EMITTER panel.
   - Draws a scrolling time-series: grey line = base_value, colored line =
     applied_value. Color is derived from parameter_id (HSL hue = parameter_id * 30°).
   - Scrolls at BL-031 tempo (if available) or 60 fps fixed fallback.
   - Toggle button hides/shows the overlay without disposing the component.
3. Confirm frame rate >= 60 fps with 6 parameter traces active on reference hardware.
4. Run UI-P2-029B lane.
5. List changed files and validation result.
```

### Slice C — Agent Prompt

```
Skills: $physics-reactive-audio $skill_impl

Context:
- BL-029 Slices A-B are merged (or C can run in parallel with A if independent).
- processBlock() in PluginProcessor.cpp processes audio buffers.
- Spectral features must be computed in processBlock() but written via atomic
  snapshot to avoid locking. The message thread reads the snapshot for bridging.
- RT invariant: no alloc in processBlock().

Task:
1. In Source/PluginProcessor.cpp:
   a. Pre-allocate a magnitude buffer (e.g., float m_mag_buf[512]) as a member.
   b. In processBlock(), compute magnitude spectrum from the first 512 samples
      (or the full buffer if < 512 samples) using a simple DFT or magnitude
      estimation. Do not allocate.
   c. Compute: centroid_hz, rolloff_hz (85% energy threshold), flux (L1 norm of
      frame-to-frame magnitude delta).
   d. Write results to a double-buffer atomic snapshot:
      struct SpectralSnapshot { float centroid_hz; float rolloff_hz; float flux; };
      std::atomic<int> m_spectral_write_idx { 0 };
      SpectralSnapshot m_spectral_buf[2];
      After writing buf[write_idx], atomically increment write_idx & 1.
2. In the message-thread bridge serialization, read the last complete snapshot and
   add spectral.centroid_hz, spectral.rolloff_hz, spectral.flux to the payload.
3. Validate: for silence input, all three values are 0.0 (or defined sentinel).
   For a 440 Hz sine, centroid_hz is approximately 440.
4. Run UI-P2-029C lane.
5. List changed files and validation result.
```

### Slices D-H — Agent Prompts (Abbreviated)

```
Slice D — Skills: $reactive-av $threejs
Context: Slice C merged; spectral fields in bridge payload.
Task: In index.js, implement a Three.js mesh overlay on the 3D room view.
- Map spectral.centroid_hz to a cool-to-warm color ramp (blue=low, red=high).
- Map spectral.flux to mesh animation speed.
- Map spectral.rolloff_hz to mesh opacity.
- Toggle independently of modulation visualizer.
- Run UI-P2-029D lane.

---

Slice E — Skills: $spatial-audio-engineering $skill_impl
Context: Slices A-D merged; EarlyReflections.h present or stubbed.
Task: Create Source/ReflectionGhostMapper.h.
- Implement image-source first-order reflection for 6 room surfaces.
- GhostEmitter struct per runbook section 5, Slice E.
- Trigger from APVTS listener (room geometry / emitter position change).
- Debounce to 30 Hz maximum.
- Write up to 6 GhostEmitters to bridge payload as reflections array.
- Computation must not occur in processBlock().
- Run UI-P2-029E lane with a 5x3x4m room, single emitter at center.

---

Slice F — Skills: $threejs $reactive-av
Context: Slice E merged; reflections array in bridge payload.
Task: In index.js, render GhostEmitters as translucent Three.js spheres.
- Opacity = attenuation field.
- Scale = delay_ms mapped to 0.1-0.5 range.
- Direction arrow (ArrowHelper) pointing from ghost to listener position.
- Pulse animation at 1 Hz using a sine envelope on opacity.
- Render in a separate Three.js layer (renderOrder or layers bitmask).
- Toggle on/off independently.
- Run UI-P2-029F lane.

---

Slice G — Skills: $juce-webview-runtime $skill_ship
Context: Slices A-F merged.
Task: Add "Export Session" action.
- UI: button in EMITTER panel + Ctrl/Cmd+E keyboard shortcut.
- PluginProcessor: on export trigger, snapshot room geometry, emitter positions,
  profile, last 60s of spectral samples (1 Hz), tracking history.
  Write to user-selected file path on message thread via juce::FileChooser.
  Do not block processBlock().
- File is valid JSON (verify with JSON.parse in JS test).
- Run UI-P2-029G lane.
- Update status.json: BL-029 status = "done" (H is aspirational).

---

Slice H — Skills: $skill_impl (aspirational, post-v1)
Context: Slice G merged. Session export JSON available.
Task: Create tools/calibration_assistant/calibrate.py.
- CLI: python calibrate.py --input session.json --output patch.json
- Implement at least one heuristic: if spectral_history mean centroid_hz > 4000,
  suggest increasing room absorption coefficient by 0.1.
- Validate patch.json against a bundled JSON schema.
- Run CLI on a sample session.json; confirm output is valid JSON patch.
```

---

## 12. Closeout Criteria

- [x] Reliability hard criteria reached on integrated owner lanes (`bl029 selftest 10/10`, `bl009 selftest 5/5`, deterministic QA lane pass).
- [x] Deterministic wrapper gate is green: `TestEvidence/bl029_reliability_gate_p4_20260225T152907Z/status.tsv`.
- [x] Owner reconciliation replay is green with RT gate restored: `TestEvidence/owner_bl029_p5p6_reconcile_20260225T152901Z/status.tsv`.
- [x] `status.json` updated to mark BL-029 done with promotion-evidence pointers.
- [x] `TestEvidence/validation-trend.md` and `TestEvidence/build-summary.md` include BL-029 tranche records through P4/P5/P6.
- [x] Promotion packet evidence captured: `TestEvidence/bl029_promotion_packet_z4_20260225T153637Z/`.
- [x] Docs freshness gate passes (`./scripts/validate-docs-freshness.sh`).
- [x] Slice H (offline ML calibration assistant) remains aspirational/deferred and is documented as non-blocking for BL-029 closeout.


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-029-dsp-visualization.md` |

### Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | Legacy closeout documented | `Evidence References` |
| Lane replay/parity | Legacy closeout documented | `Evidence References` |
| RT safety | Legacy closeout documented | `Evidence References` |
| Docs freshness | Legacy closeout documented | `Evidence References` |
| Status schema | Legacy closeout documented | `Evidence References` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | Required for modern promotions; legacy packets may predate marker | `Evidence References` |

### Backlog/Status Sync Checklist

- [x] Runbook archived under `Documentation/backlog/done/`
- [x] Backlog index links the done runbook
- [x] Historical evidence references retained
- [ ] Legacy packet retrofitted to modern owner packet template (`_template-promotion-decision.md`) where needed
- [ ] Legacy closeout fully normalized to modern checklist fields where needed
