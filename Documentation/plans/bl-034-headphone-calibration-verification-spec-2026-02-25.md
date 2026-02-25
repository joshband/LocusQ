Title: BL-034 Headphone Calibration Verification and Profile Spec
Document Type: Specification
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-034 Headphone Calibration Verification and Profile Spec

## Purpose

Define the verification and profile-governance layer for headphone calibration in LocusQ after BL-033 core landing.

## Inputs

- `Documentation/research/LocusQ Headphone Calibration Research Outline.md`
- `Documentation/research/Headphone Calibration for 3D Audio.pdf`
- `Documentation/testing/bl-029-audition-platform-qa.md`
- `Documentation/runbooks/release-checklist-template.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Scope Contract

1. Device/profile catalog contract:
   - profile identity for generic, AirPods Pro 2, Sony WH-1000XM5, custom SOFA reference.
   - deterministic fallback behavior when referenced SOFA/EQ data is unavailable.
2. Verification contract:
   - reproducible quick checks for front/back discrimination, elevation perception proxy, and externalization confidence.
   - persisted scalar verification scores in deterministic range.
3. QA contract:
   - machine-readable lane output for headphone mode pathing, diagnostics, and fallback taxonomy.
   - repeatability expectation: same seed/input yields same status + diagnostics.
4. Release governance alignment:
   - explicit evidence hooks suitable for BL-030 release checklist gates.

## Minimal Slice Targets

- S1: profile library contract + serialization invariants
- S2: verification scoring workflow + status persistence contract
- S3: deterministic QA lane definitions + failure taxonomy
- S4: release governance linkage and acceptance evidence map

## Evidence Expectations

- per-profile verification matrix (`status.tsv` + profile outcomes)
- diagnostics snapshots for requested/active/fallback reasons
- deterministic replay hashes for verification lanes
- docs freshness + traceability updates

