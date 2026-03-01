---
name: hrtf-rendering-validation-lab
description: Build and validate deterministic HRTF rendering lanes spanning offline SOFA truth renders, realtime partitioned convolution behavior, interpolation/crossfade safety, and parity evidence for BL-055/BL-061.
---

Title: HRTF Rendering Validation Lab Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# HRTF Rendering Validation Lab

Use this skill when validating HRTF render correctness across offline and realtime paths.

## Scope
- Offline SOFA truth-render workflows and deterministic repeatability.
- Realtime FIR/partitioned convolution validation and latency contracts.
- Crossfade behavior during profile or direction updates (no zipper/click artifacts).
- Parity checks between baseline nearest-neighbor and interpolation paths.

## Workflow
1. Establish offline truth first.
   - Freeze input signal, pose timeline, and SOFA selection.
   - Generate deterministic stereo output artifact(s).
2. Validate realtime rendering contract.
   - Confirm latency/reporting path.
   - Confirm no RT allocation, lock, or blocking I/O in audio path.
3. Validate transition safety.
   - Exercise profile/direction swaps and measure artifact-free crossfades.
4. Run parity checks.
   - Compare offline and realtime summary metrics for equivalent scenarios.
   - Compare interpolation path vs nearest-neighbor baseline.
5. Publish evidence packet and gate decision.

## Required Evidence
- `status.tsv`
- `latency_contract.tsv`
- `crossfade_artifact_check.tsv`
- `offline_parity_summary.md`
- `mode_parity.tsv`

## Cross-Skill Routing
- Pair with `spatial-audio-engineering` for renderer architecture and channel/coordinate contracts.
- Pair with `steam-audio-capi` when Steam fallback/requested-vs-active behavior is involved.
- Pair with `skill_testing` for replay-tier execution and taxonomy reporting.

## References
- `references/offline-truth-lane.md`
- `references/realtime-convolver-contract.md`
- `references/parity-and-crossfade-gates.md`

## Deliverables
- File-level change list and rationale.
- Explicit pass/fail matrix for latency/parity/crossfade gates.
- Validation status: `tested`, `partially tested`, or `not tested`.
