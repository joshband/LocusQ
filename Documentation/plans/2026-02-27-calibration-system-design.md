Title: LocusQ Calibration System Design
Document Type: Design Document
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# LocusQ Calibration System Design

## Status

Approved.
This remains the canonical calibration architecture reference.

It supersedes earlier calibration-only design fragments and anchors:
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

Integrate robust headphone calibration and binaural monitoring into LocusQ with:
- personalized HRTF selection,
- model-based headphone EQ,
- head-tracked monitoring,
- deterministic profile handoff,
- and a listening-test gate that can prove value.

Primary constraints:
- no allocations or locks in `processBlock()`
- deterministic state and QA compatibility
- perceptual improvement must be measurable before Phase C expansion

## Core Architecture

### The Two-Profile Model

Every device and user flow must stay split into two profiles:

| Profile | Meaning | Example |
|---|---|---|
| user profile | HRTF or HRTF-proxy personalization | SADIE II subject match |
| headphone profile | model-specific EQ / headphone transfer compensation | AirPods Pro or WH-1000XM5 preset |

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

## Realtime Safety Invariants

1. No heap allocation in `processBlock()`.
2. No mutex lock/unlock in `processBlock()`.
3. No file I/O in `processBlock()`.
4. Coefficient and engine updates must happen through non-RT publication and atomic swap.
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

### Future, Not v1

- AirPods 1/2/3/4 EQ-only expansions
- larger device-library growth
- richer interpolation and morphing datasets

## Contract Surfaces

### Headphone Preset Contract

Each model/mode pair uses a preset file.
The active preset contract belongs in the profile and preset surfaces, not in prose duplication here.

### User Profile Contract

The user profile must identify:
- matched subject,
- SOFA reference,
- reproducibility hash,
- and creation metadata.

### `CalibrationProfile.json`

This is the plugin handoff contract.

It must carry:
- user profile fields,
- headphone profile fields,
- tracking flags and offsets,
- verification fields for listening-test outcomes.

Schema authority:
- `Documentation/plans/calibration-profile-schema-v1.md`

## Delivery Tracks

| Track | Main Focus |
|---|---|
| Plugin DSP track | virtual surround, PEQ, FIR, latency, state migration |
| Companion/profile track | renderer baseline, preset library, device detection, capture, matching, profile write |
| Integration track | plugin read path, SOFA loading, CALIBRATE UI, end-to-end smoke |
| Listening gate | randomized listening harness and decision gate |

## Program Gates

### Phase B Gate

Required before Phase C:
- at least 5 participants,
- at least 10 scenes each,
- measurable improvement in externalization or localization.

Hard gate:
- `>=20%` mean externalization improvement
  or
- `p < 0.05` localization gain

If this fails:
- improve feature extraction, capture quality, or dataset strategy first.
- do not move on to interpolation or ML-heavy expansion.

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

## Key Acceptance Themes

| Theme | Expected Truth |
|---|---|
| virtual surround | quad-to-binaural path works without RT regressions |
| FIR | latency reporting matches active engine behavior |
| state migration | profile/state upgrades stay deterministic |
| handoff | profile load/unload is stable and glitch-safe |
| listening gate | Phase B outcome decides whether deeper expansion is justified |

## Privacy Rule

Ear and face imagery is biometric-adjacent.

Keep these rules:
- processing stays on-device,
- source images are not retained after embedding unless explicitly required and consented,
- reproducibility uses hashes and profile metadata,
- any sync path must be explicit and consented.

## Supporting Inputs

- `Documentation/plans/2026-02-27-calibration-implementation-plan.md`
- `Documentation/plans/calibration-profile-schema-v1.md`
- `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`
- `Documentation/Calibration POC/README.md`
- `Documentation/Calibration POC/locusq_spatial_prototype/`

## Visual Aid Index

| Artifact | Role |
|---|---|
| legacy calibration design doc | deep architecture detail and original diagrams |
| `Documentation/plans/2026-02-27-calibration-implementation-plan.md` | active implementation contract |
| `Documentation/plans/calibration-profile-schema-v1.md` | schema reference |

## Archive Note

The original long-form calibration architecture design was preserved at:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/2026-02-27-calibration-system-design-legacy.md`

Use the archive copy when you need the full original staging tables, schema examples, and narrative detail.
Use this file as the active design authority.
