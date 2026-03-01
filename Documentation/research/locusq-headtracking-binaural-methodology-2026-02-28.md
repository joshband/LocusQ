Title: LocusQ Head-Tracking + Binaural Methodology (Canonical)
Document Type: Research Methodology
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-01

# LocusQ Head-Tracking + Binaural Methodology (Canonical)

## Purpose

Create one canonical, research-grade methodology that unifies:
- head-tracking signal ingestion,
- quaternion filtering and safety constraints,
- plugin-side orientation application,
- companion visualization diagnostics,
- offline/real-time binaural rendering strategy,
- perceptual evaluation methodology,
- backlog execution and prioritization.

This document is a Tier 2 reference methodology and should be used to align BL-053..BL-060 execution.

## Scope

In scope:
- End-to-end orientation path: AirPods motion -> companion -> UDP bridge -> plugin renderer.
- Math and implementation contracts for stable quaternion handling.
- Virtual binaural monitoring behavior contracts.
- Test harness approach (objective + subjective).
- Prioritized implementation and documentation TODOs.

Out of scope:
- Product marketing content.
- Unscoped architecture rewrites.
- Immediate migration to a completely new rendering engine.

## Current Baseline Snapshot

Implemented baseline (as of 2026-02-28):
- Bridge packet v2 with angular velocity and sensor flags exists (`HeadTrackingBridge`, `HeadPoseInterpolator`).
- Yaw recenter and drift telemetry path exists.
- BL-053 orientation plumbing exists and structural lane passes.
- Calibration POC prototype exists under `Documentation/Calibration POC/locusq_spatial_prototype/` (offline SOFA render, profile selection, listening tools).

Critical runtime finding fixed in this changeset:
- Monitoring path previously accepted `listenerOrientation` but ignored it in `renderVirtualSurroundForMonitoring`.
- Orientation-derived quad rotation has been restored in that path.
- Companion/WebView top-view head-tracking arrow now derives from quaternion forward projected to XZ plane (instead of serialized yaw-only path), reducing pitch-to-lateral visualization artifacts.
- Head/arrow orientation visuals are now gated on fresh pose (`activePose`) so stale packets do not keep animating orientation state.

## Calibration POC Provenance Map

The `Documentation/Calibration POC/` directory is treated as foundational research input for this methodology, not a side artifact.

Primary extraction points:
- `Documentation/Calibration POC/locusq_spatial_prototype/README.md`
  - offline SOFA render baseline and usage contract.
- `Documentation/Calibration POC/locusq_spatial_prototype/tools/render.py`
  - deterministic offline binaural rendering reference lane.
- `Documentation/Calibration POC/locusq_spatial_prototype/tools/listening_test.py`
  - randomized blind listening test scaffold.
- `Documentation/Calibration POC/locusq_spatial_prototype/tools/analyze_results.py`
  - statistical summary and profile verification write-back baseline.
- `Documentation/Calibration POC/locusq_spatial_prototype/locusq/profile_select.py`
  - nearest-neighbor profile selection baseline (ear embedding -> subject rank).

Policy for this methodology:
- Keep POC logic as the experimental truth source for fast research iteration.
- Promote only validated, scoped slices into plugin/companion production lanes via backlog items.

## Documentation Surface Coverage (Explicit)

This methodology is maintained against the following additional documentation surfaces to avoid drift:

- Backlog governance and lifecycle:
  - `Documentation/backlog/index.md`
  - `Documentation/backlog/_template-intake.md`
  - `Documentation/backlog/_template-runbook.md`
- Execution runbooks and release operations:
  - `Documentation/runbooks/device-rerun-matrix.md`
  - `Documentation/runbooks/release-checklist-template.md`
  - `Documentation/runbooks/backlog-execution-runbooks.md` (deprecated, historical context only)
- Design and implementation planning baselines:
  - `Documentation/plans/2026-02-27-calibration-system-design.md`
  - `Documentation/plans/2026-02-27-calibration-implementation-plan.md`
  - `Documentation/plans/calibration-profile-schema-v1.md`
  - `Documentation/plans/bl-045-head-tracking-fidelity-v11-spec-2026-02-26.md`
- Architecture and risk reviews:
  - `Documentation/reviews/2026-02-26-full-architecture-review.md`
  - `Documentation/reviews/LocusQ Repo Review 02262026.md`
  - `Documentation/reviews/2026-03-01-headtracking-research-backlog-reconciliation.md`
- Generated report snapshots (governance-controlled):
  - `Documentation/reports/` (for extraction/audit only; do not treat as canonical long-term source)
- Research tier indexing:
  - `Documentation/research/README.md`

Interpretation policy:
- `Documentation/backlog/index.md` remains authoritative for lifecycle state and replay cadence policy.
- `Documentation/research/` captures methods and rationale.
- `Documentation/reviews/` captures risk findings that must be converted into actionable backlog tasks or acceptance checks.
- `Documentation/plans/` captures intended architecture/slice design; backlog items should reference relevant plans explicitly.
- `Documentation/reports/` remains scratch/provenance-oriented and should not become the sole source for acceptance claims.

## Non-Negotiable Contracts

1. Realtime safety:
- No allocations, no locks, no blocking I/O in `processBlock()`.
- Any heavy state build/precompute must happen off audio thread.

2. Determinism:
- Same input + same pose sequence must produce equivalent output.
- Stale or invalid pose packets must deterministically fall back to identity behavior.

3. Coordinate conventions:
- Steam canonical listener basis: +X right, +Y up, -Z ahead.
- Quad source/mix ordering must be explicit and consistent at every boundary.

4. Governance:
- Canonical promotion evidence under `TestEvidence/`.
- Backlog lifecycle and replay cadence follow `Documentation/backlog/index.md`.

## End-to-End Orientation Path

### A. Companion capture and packetization

Data source:
- `CMHeadphoneMotionManager` quaternion + rotationRate + sensorLocation.

Packet contract:
- v2 packet includes qx/qy/qz/qw, timestamp, seq, angVx/angVy/angVz, sensorLocationFlags.

Required checks before send:
- quaternion finite + normalized,
- monotonic `seq`,
- bounded timestamp age metrics for debug telemetry.

### B. Bridge ingest and snapshot publication

Bridge responsibilities:
- validate packet magic/version/size,
- parse v1/v2 compatibly,
- atomically publish latest snapshot,
- track invalid packet count.

Audio thread consumer responsibilities:
- read latest snapshot atomically,
- never block for network/IPC.

### C. Interpolation and motion stabilization

Current plugin baseline:
- shortest-arc slerp,
- bounded angular-velocity prediction,
- sensor-switch crossfade.

Recommended v1.2 stabilization mode:
- adaptive cutoff low-pass on quaternion path using SLERP alpha from One-Euro-style velocity adaptation.

## Quaternion Filtering Methodology

### Rationale

Quaternion space filtering avoids Euler singularities and axis-coupling artifacts. Adaptive filtering reduces idle jitter while limiting motion lag.

### Recommended algorithm

Per frame:
1. Ensure shortest arc (`dot(q_prev, q_sample) >= 0`).
2. Compute delta angle and angular speed.
3. Clamp extreme angular spikes.
4. Compute adaptive cutoff: `fc = min_cutoff + beta * |omega_filtered|`.
5. Convert to discrete alpha: `alpha = 1 - exp(-2*pi*fc*dt)`.
6. Output: `q_out = slerp(q_prev, q_sample, alpha)`.

### Pseudocode

```cpp
Quaternion filterPose(Quaternion qPrev, Quaternion qSample, float dt) {
    if (dot(qPrev, qSample) < 0.0f) qSample = -qSample;

    Quaternion dq = normalize(inverse(qPrev) * qSample);
    float angle = 2.0f * acos(clamp(dq.w, -1.0f, 1.0f));
    float omega = dt > 0 ? angle / dt : 0.0f;

    omega = clamp(omega, 0.0f, omegaMax);
    float omegaF = oneEuroOmega.filter(omega, dt);

    float fc = minCutoff + beta * abs(omegaF);
    float alpha = clamp(1.0f - exp(-2.0f * PI * fc * dt), 0.0f, 1.0f);

    return normalize(slerp(qPrev, qSample, alpha));
}
```

Default starting values:
- `minCutoff`: 1.5..2.5 Hz
- `beta`: 0.08..0.2
- `omegaMax`: 6.28..12.56 rad/s

## Plugin-Side Orientation Application

### Required behavior

1. Convert filtered pose to listener orientation basis.
2. Apply yaw offsets:
- profile offset (`tracking.hp_yaw_offset_deg`),
- runtime recenter offset (`Set Forward`).
3. For `virtual_binaural` monitoring:
- rotate quad bed with orientation-derived speaker mix,
- render binaural,
- fall back to identity when stale/unavailable.

### Regression class to prevent

"Orientation plumbed but ignored" regressions:
- Function signatures include orientation pointer,
- Callers build and pass pointer,
- Renderer path discards pointer.

This class must have an explicit lane check in BL-053 acceptance.

## Companion Three.js Visualization Methodology

### Visualization goals

The companion visualization is a diagnostic instrument, not a product UI effect.

Must prove:
- axis mapping is coherent,
- quaternion stream is finite and stable,
- sensor-location switches do not produce apparent axis remapping without explicit indicator.

### Current anomaly hypothesis

Observed odd behavior (e.g., up/down causing unexpected lateral arrow motion) is consistent with one or more of:
- sensor-location frame shift (left/right bud active source change) without explicit remap,
- reference-frame mismatch between displayed basis and user mental model,
- smoothing path divergence between runtime packet stream and display stream.

### Companion hardening checks

1. Add explicit on-screen frame contract text (+X/+Y/-Z) and active sensor location badge.
2. Log frame transform and sensor transitions with timestamps.
3. Add synthetic mode axis sanity suite:
- pure yaw drive,
- pure pitch drive,
- pure roll drive,
- assert principal-arrow movement dominance.
4. Add optional remap matrix for left/right sensor harmonization and compare before/after.

## Binaural Rendering Strategy

### Offline reference lane (already available in prototype)

Use offline SOFA renderer to establish correctness and perceptual baselines:
- deterministic output,
- HRIR selection validation,
- listening-test asset generation.

### Realtime lane

Use partitioned convolution and crossfaded filter transitions for long FIR/HRIR paths:
- direct FIR for short taps,
- partitioned FFT for long taps,
- crossfade outputs during profile/HRIR swaps.

## Perceptual Evaluation Methodology

### Phase B minimum protocol

- Blind 2x2 design:
  - generic vs personalized HRTF,
  - no EQ vs device EQ.
- At least 5 participants and 10 scenes.
- Record:
  - localization accuracy,
  - front/back confusion,
  - externalization,
  - preference.

Gate condition:
- >=20% mean externalization improvement OR p < 0.05 localization gain.

## Backlog Alignment (BL-053..BL-060)

| ID | Intent | Current Posture | Methodology Alignment |
|---|---|---|---|
| BL-053 | Head-tracking orientation injection | In Validation | Keep structural lane + complete manual acceptance with orientation-restore verification |
| BL-054 | PEQ RT integration | Open | Apply RT-safe coefficient swap and deterministic bypass parity |
| BL-055 | FIR engine integration | Open | Apply latency contract + crossfade-safe engine swap |
| BL-056 | State migration + latency | Open | Enforce idempotent state migration and latency reset invariants |
| BL-057 | Device preset library | Open | Validate per-model/mode preset quality and sweep checks |
| BL-058 | Companion profile acquisition | Open | Harden capture privacy and deterministic profile selection path |
| BL-059 | Calibration profile handoff | Open | Atomic handoff and glitch-free runtime profile reload contract |
| BL-060 | Listening harness | Open | Execute blinded test protocol and record gate decision |

## Skill Routing Plan (Codex/Claude)

Use specialist skills deliberately and repeatedly for each slice:

| Skill | Use When | Primary Surfaces |
|---|---|---|
| `skill_plan` | Defining execution order, dependencies, and phased scope before edits | `Documentation/backlog/index.md`, `Documentation/plans/*.md` |
| `skill_docs` | Updating runbooks/index/research and evidence/governance wording | `Documentation/backlog/*`, `Documentation/research/*`, `Documentation/README.md`, `Documentation/standards.md` |
| `spatial-audio-engineering` | Spatial contracts, orientation path, binaural strategy, listening-test protocol | `Source/SpatialRenderer.h`, `scripts/qa-bl0*.sh`, `TestEvidence/*` |
| `steam-audio-capi` | Steam runtime/fallback behavior and requested vs active monitoring diagnostics | `Source/SteamAudioVirtualSurround.h`, Steam QA lanes, scene diagnostics |
| `threejs` | Companion/WebView scene frame mapping, axis sanity, render-loop behavior | `Source/ui/public/js/index.js`, companion visualization flow |
| `skill_impl` | Production code edits in plugin/companion with RT-safety constraints | `Source/*`, `companion/Sources/*` |
| `skill_troubleshooting` | Repro-driven defect triage when behavior regresses (for example head-tracking orientation anomalies) | runbooks + targeted scripts + issue notes |
| `skill_test` / `skill_testing` | Formal lane execution and replay cadence enforcement | `scripts/qa-*.sh`, `TestEvidence/*`, backlog replay tables |

Current task-set application:
- Used `skill_plan` to structure BL-053..BL-060 and cross-surface review.
- Used `skill_docs` for canonical methodology + runbook/index alignment.
- Used `spatial-audio-engineering` and `steam-audio-capi` for the orientation path restore and monitoring-path assertions.
- Routed companion math/visual oddities under `threejs` + `skill_troubleshooting` as explicit P0 checks.

## Prioritized TODOs

### P0 (Immediate stability)

1. Verify restored BL-053 orientation behavior with manual listening checklist.
2. Add regression assertion for "orientation pointer provided and consumed" in BL-053 lane.
3. Add companion axis sanity diagnostics (pure yaw/pitch/roll synthetic sweeps).
4. Run companion math/Three.js regression drill against review findings (frame mapping, sensor-location transitions, up-vector rotation behavior) and capture evidence under `TestEvidence/`.
5. Add regression check that stale pose cannot keep rotating visualization state (active-pose gating).

### P1 (Execution posture + documentation)

1. Keep this document as canonical methodology for BL-053..BL-060 planning discussions.
2. Reconcile backlog index rows/dependency graph/material map for BL-053..BL-060.
3. Align runbooks with this methodology where acceptance wording is ambiguous.
4. Add explicit yaw-axis convention audit task (ZYX extraction + offset composition vs intended Y-up mental model).

### P2 (Capability hardening)

1. Add optional One-Euro adaptive quaternion smoothing mode in plugin/companion.
2. Complete BL-055 partitioned FIR and swap crossfade validation.
3. Complete BL-060 listening study and publish gate outcome in evidence.
4. Harden bridge sequence restart handling so companion relaunch cannot silently pin stale pose.

### Finalization TODO (user-requested)

- At the very end of this task set (after all requested edits and validations):
  - stage,
  - commit,
  - push.

## Validation Checklist for This Methodology Document

- [x] References current code path contracts and known regressions.
- [x] Provides implementable pseudocode for quaternion filtering.
- [x] Separates immediate fixes from medium-term hardening.
- [x] Maps directly to BL-053..BL-060 backlog execution.
- [x] Includes explicit end-of-workflow repository operation TODO.
