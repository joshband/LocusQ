Title: Calibration POC Research Prototype Index
Document Type: Research Prototype Index
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# Calibration POC Research Prototype Index

## Purpose

Define the role of `Documentation/Calibration POC/` so it stays useful without acting like a shadow product-spec or shadow backlog system.

## Classification

This folder is a Tier 2 research prototype surface.

It is:
- allowed to contain exploratory notes, prototype scripts, and reference PDFs,
- valid as supporting input for calibration and listening-methodology work,
- intentionally non-authoritative for backlog status, release state, and implementation completion claims.

It is not:
- a source of backlog truth,
- a replacement for ADRs,
- a replacement for `Documentation/backlog/index.md`,
- a replacement for current implementation specs under `Documentation/plans/`,
- or a replacement for decision-grade evidence under `TestEvidence/`.

## Subsections

- research notes and reference writeups:
  - `LocusQ Headphone Calibration Research Outline.md`
  - `HRTF and Personalized Headphone Calibration.md`
  - `LocusQ Spatial Audio Personalization Engineering Task Breakdown (Tickets, Acceptance Criteria, Sequencing).md`
  - `LocusQ Spatial Personalization Phase-Gated Execution Plan (Hard Technical Milestones).md`
  - `locusq_spatial_audio_spec.md`
- prototype contracts and constraints:
  - `Core JUCE real-time rules (non-negotiable).md`
  - `DSP_REALTIME_SPATIAL_CONSTRAINTS.md`
  - `DSP_SPATIAL_INVARIANTS_CONTRACT.md`
- prototype tooling:
  - `locusq_spatial_prototype/`
- external reference PDF:
  - `Headphone Calibration for 3D Audio.pdf`

## Usage Rules

1. If a prototype idea becomes implementation truth, promote it into:
   - `Documentation/plans/`,
   - `Documentation/invariants.md`,
   - `Documentation/adr/`,
   - or backlog/TestEvidence surfaces as appropriate.
2. Keep filenames stable when scripts or evidence lanes depend on them.
3. Do not store large raw datasets or zip bundles here.
4. Archive superseded notes once their content has been distilled into canonical specs or ADRs.
