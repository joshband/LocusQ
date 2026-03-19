Title: Calibration POC Research Prototype Index
Document Type: Research Prototype Index
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# Calibration POC Research Prototype Index

## Purpose

Keep `Documentation/Calibration POC/` as a small research-prototype surface.
Do not let it behave like a shadow spec system.

## Classification

This folder is Tier 2.

Use it for:
- retained calibration research notes,
- prototype code and tools,
- external reference material.

Do not use it for:
- backlog truth,
- release or completion claims,
- current implementation authority,
- ADR replacement,
- decision-grade evidence.

## Kept Active

### Research References

- `LocusQ Headphone Calibration Research Outline.md`
- `HRTF and Personalized Headphone Calibration.md`
- `locusq_spatial_audio_spec.md`
- `Headphone Calibration for 3D Audio.pdf`

### Prototype Tooling

- `locusq_spatial_prototype/`

## Archived On 2026-03-18

The following notes were moved to:
`Documentation/archive/2026-03-18-doc-surface-consolidation/calibration-poc/`

- `Core JUCE real-time rules (non-negotiable).md`
- `DSP_REALTIME_SPATIAL_CONSTRAINTS.md`
- `DSP_SPATIAL_INVARIANTS_CONTRACT.md`
- `LocusQ Spatial Audio Personalization Engineering Task Breakdown (Tickets, Acceptance Criteria, Sequencing).md`
- `LocusQ Spatial Personalization Phase-Gated Execution Plan (Hard Technical Milestones).md`
- `OFA HRIR loading + nearest-direction selection + binaural convolution + optional WH‑1000XM5 PEQ.md`

Reason:
These files were useful during prototype exploration, but they were unreferenced from active docs and duplicated material now better represented in plans, research docs, invariants, or archived history.

## Usage Rules

1. Promote real implementation truth into `Documentation/plans/`, `Documentation/adr/`, `Documentation/invariants.md`, backlog docs, or `TestEvidence/`.
2. Keep prototype code paths stable when active research docs depend on them.
3. Do not add raw datasets, zip bundles, or long execution diaries here.
4. Archive superseded notes once their useful parts are distilled into canonical docs.
