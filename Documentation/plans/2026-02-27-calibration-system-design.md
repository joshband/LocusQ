Title: LocusQ Calibration System Design
Document Type: Design Document
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# LocusQ Calibration System Design

## Status

Approved.
This is the canonical calibration design authority.

It anchors:
- BL-052
- BL-053
- BL-054
- BL-055
- BL-056
- BL-057
- BL-058
- BL-059
- BL-060
- BL-061

Legacy deep design:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-27-calibration-system-design-legacy.md`

## Objective

Build the calibration system as one measurable pipeline:
- personalized HRTF selection
- model-based headphone EQ
- head-tracked monitoring
- deterministic profile handoff
- perceptual proof before expansion

Hard constraints:
- no allocations or locks in `processBlock()`
- deterministic state and QA compatibility
- measurable improvement before Phase C expansion

## Core Architecture

### Two-Profile Model

Keep the profiles separate.

| Profile | Meaning | Example |
|---|---|---|
| user profile | HRTF or HRTF-proxy personalization | SADIE II subject match |
| headphone profile | model-specific EQ / transfer compensation | AirPods Pro or WH-1000XM5 preset |

Rule:
- do not collapse user and headphone concerns into one blob.

### Runtime Boundary

| Surface | Responsibility |
|---|---|
| companion | capture, device detection, profile selection, profile write |
| `CalibrationProfile.json` | handoff contract |
| plugin | read profile, apply monitoring chain, report verification state |

### Monitoring Paths

| Mode | Path |
|---|---|
| `speakers` | pass-through |
| `steam_binaural` | quad bed -> Steam Audio virtual surround -> PEQ -> FIR -> stereo |
| `virtual_binaural` | bypass Steam Audio; optional PEQ path |

## Key Invariants

1. No heap allocation in `processBlock()`.
2. No mutex lock/unlock in `processBlock()`.
3. No file I/O in `processBlock()`.
4. Coefficient and engine updates must publish off the RT thread and swap atomically.
5. FIR latency changes must update host latency reporting.
6. Non-finite output must be classified and surfaced through the validation contract.

## Device Scope

### v1 Scope

| Device | Head Tracking | HRTF Personalization | Headphone EQ |
|---|---|---|---|
| AirPods Pro 1 | yes | yes | yes |
| AirPods Pro 2 | yes | yes | yes |
| AirPods Pro 3 | yes | yes | yes |
| Sony WH-1000XM5 | no | baseline/personalized path only | yes |
| generic/custom | no | baseline/custom path | optional |

### Not v1

- AirPods 1/2/3/4 EQ-only expansions
- larger device-library growth
- richer interpolation and morphing datasets

## Phase Gate

### Phase B Gate

Required before Phase C:
- at least 5 participants
- at least 10 scenes each
- measurable improvement in externalization or localization

Hard gate:
- `>=20%` mean externalization improvement, or
- `p < 0.05` localization gain

If this fails:
- improve feature extraction, capture quality, or dataset strategy first
- do not move on to interpolation or ML-heavy expansion

### Phase C

Only after Phase B passes:
- HRIR interpolation
- crossfaded filter updates
- wider device-library expansion

## Backlog Mapping

| BL | Focus |
|---|---|
| BL-052 | virtual surround and quad layout |
| BL-053 | head-tracking orientation injection |
| BL-054 | PEQ cascade integration |
| BL-055 | FIR engine |
| BL-056 | state migration and latency contract |
| BL-057 | device preset library |
| BL-058 | companion profile acquisition |
| BL-059 | profile integration handoff |
| BL-060 | Phase B listening harness |
| BL-061 | interpolation and crossfade follow-on |

## Supporting Inputs

- `Documentation/plans/2026-02-27-calibration-implementation-plan.md`
- `Documentation/plans/calibration-profile-schema-v1.md`
- `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`
- `Documentation/Calibration POC/README.md`
- `Documentation/Calibration POC/locusq_spatial_prototype/`

## Milestone Snapshot

| Area | Status | Why it matters | Evidence |
|---|---|---|---|
| design authority | approved | current canonical calibration reference | this file |
| implementation plan | active | short execution surface for BL-054..BL-061 | `Documentation/plans/2026-02-27-calibration-implementation-plan.md` |
| schema contract | active | plugin/companion handoff boundary | `Documentation/plans/calibration-profile-schema-v1.md` |
| methodology | supporting | research and evaluation baseline | `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md` |
| POC | supporting | prototype truth source, not status authority | `Documentation/Calibration POC/README.md` |

## Visual Aid Index

| Artifact | Role |
|---|---|
| archive copy | deep history and original staging tables |
| implementation plan | active execution contract |
| schema reference | handoff boundary reference |

## Archive Note

The long-form calibration architecture design was preserved at:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-27-calibration-system-design-legacy.md`

Use the archive copy when you need the original staging tables or narrative detail.
Use this file as the active design authority.
