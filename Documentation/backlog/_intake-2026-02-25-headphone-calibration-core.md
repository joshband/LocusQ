Title: Headphone Calibration Core Path Intake
Document Type: Backlog Intake
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# Intake: Headphone Calibration Core Path

## Origin

| Field | Value |
|---|---|
| Source | Research (`Documentation/research/LocusQ Headphone Calibration Research Outline.md`, `Documentation/research/Headphone Calibration for 3D Audio.pdf`) |
| Discovered | 2026-02-25 |
| Reporter | APC Codex |

## Description

Research indicates LocusQ needs a deterministic internal headphone calibration core path that separates measurable system compensation (EQ/FIR) from listener-dependent HRTF/BRIR selection and publishes explicit diagnostics for requested vs active monitoring mode. This intake proposes the core implementation backlog work for `steam_binaural` integration, PEQ/FIR chain ordering, SOFA reference handling, and latency contract publication under RT-safe constraints.

## Proposed Priority

P2 — extends established BL-009/BL-026/BL-028 monitoring contracts with meaningful user impact, but not release-critical P1 scope.

## Dependency Guesses

- Likely depends on: BL-009, BL-017, BL-026, BL-028
- Likely blocks: BL-034

## Proposed Track

Track A — Runtime Formats

## Next Step

- [x] Triage: assign BL/HX ID and validate dependencies (`BL-033`)
- [x] Promote: convert to full runbook in `Documentation/backlog/`
- [x] Add row to `Documentation/backlog/index.md`
- [ ] Archive this intake doc

