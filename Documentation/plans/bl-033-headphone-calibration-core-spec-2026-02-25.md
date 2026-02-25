Title: BL-033 Headphone Calibration Core Spec
Document Type: Specification
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-033 Headphone Calibration Core Spec

## Purpose

Define the implementation contract for a deterministic headphone calibration core path in LocusQ:
- quad bed to binaural monitoring (`steam_binaural`)
- headphone EQ/FIR post-chain
- SOFA reference handling
- latency publication and diagnostics parity

## Inputs

- `Documentation/research/LocusQ Headphone Calibration Research Outline.md`
- `Documentation/research/Headphone Calibration for 3D Audio.pdf`
- `Documentation/scene-state-contract.md`
- `Documentation/spatial-audio-profiles-usage.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Core Contract

1. Monitoring path contract:
   - `speakers` remains existing pass-through path.
   - `steam_binaural` routes renderer bed through internal Steam binaural wrapper.
   - `virtual_binaural` bypasses internal Steam wrapper.
2. Headphone calibration data contract:
   - Primitive control values in APVTS (`hp_eq_mode`, `hp_hrtf_mode`, etc.).
   - Non-primitive blobs in state (`hp_sofa_ref`, `hp_fir_coeffs_f32`, metadata payloads).
3. Processing order for `steam_binaural`:
   - renderer bed -> Steam binaural -> PEQ (optional) -> FIR (optional) -> stereo out.
4. RT safety:
   - no allocations, locks, or blocking I/O in `processBlock`.
   - expensive HRTF/FIR (re)build work off the audio thread, switched atomically.
5. Latency contract:
   - FIR engine latency explicitly published via host latency API.
   - bypass and engine swaps update latency deterministically.

## Minimal Slice Targets

- S1: state + parameter contract and migration additions
- S2: Steam virtual surround wrapper + monitoring routing integration
- S3: PEQ/FIR chain integration with deterministic engine selection
- S4: scene-state diagnostics publication (`requested`, `active`, `stage`, fallback reason)
- S5: QA harness lane and evidence contract updates

## Evidence Expectations

- build + smoke pass bundle
- RT safety audit bundle
- headphone path contract lane results
- scene-state diagnostics snapshot for `speakers`/`steam_binaural`/`virtual_binaural`
- latency assertion evidence for FIR bypass/direct/partitioned modes

