Title: Spatial Layout and Codec Notes
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# Layout and Codec Notes

## Layout Baselines for LocusQ Work
- `Stereo (2.0)`: compatibility baseline for every host and fallback lane.
- `Quad (4.0)`: current v1 spatial baseline in LocusQ.
- `5.1` and `7.1`: monitor/output-target layouts for future expansion lanes.
- `7.4.2`: visualization and monitoring scope in BL-018; do not treat as v1 DSP contract change unless explicitly approved.

## Ambisonic Conventions
Before implementing or validating ambisonic paths, pin and document:
- Channel order convention (for example ACN).
- Normalization convention (for example SN3D/FuMa).
- Encode/decode matrix assumptions for test vectors.

Do not mix conventions across lanes without explicit conversion and test evidence.

## Binaural and Headphone Notes
- Binaural render acceptance should include objective divergence checks versus stereo fallback.
- Consumer headphone branding support does not imply plugin-level control of personalized/head-tracked renderers in DAW hosts.
- Treat AirPods Pro 2 and WH-1000XM5 as monitored stereo endpoints unless a host API bridge explicitly provides additional control and telemetry.

## Atmos and NGA-Aware Guidance
- Use ADM/BW64 metadata tooling for interchange and renderer validation where needed.
- Keep plugin-internal DSP contracts independent from file-format export contracts.
- Use IAMF/ADM research as planning input unless shipping requirements explicitly demand codec integration.

## Deterministic QA Expectations
- Fixed input + fixed seed + fixed parameter timeline must produce stable hashes/metrics.
- Spatial transforms and decode stages must remain finite (`NaN`/`Inf` guarded).
- Validate mirrored-source and elevation edge cases for directional logic.
