Title: LocusQ Spatial Audio Profiles Usage Guide
Document Type: Operator Guide
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-03-01

# LocusQ Spatial Audio Profiles Usage Guide

## Purpose

Document the supported audio profile surface in LocusQ and provide practical setup steps for:
- 3D spatial audio types
- binaural
- ambisonics
- quadraphonics
- stereo
- surround 7.2.1
- mono, stereo, and multichannel output paths

## Control Surface

| Function | Parameter ID | UI/Host Surface | Notes |
|---|---|---|---|
| Headphone render mode | `rend_headphone_mode` | Production UI (Renderer rail) and host automation | `Stereo Downmix`, `Steam Binaural` |
| Headphone device profile | `rend_headphone_profile` | Production UI (Renderer rail) and host automation | `Generic`, `AirPods Pro 2`, `AirPods Pro 3`, `Sony WH-1000XM5`, `Custom SOFA` |
| Spatial output profile (3D audio type) | `rend_spatial_profile` | Host automation (parameter list), QA scenarios | Not yet exposed as a production UI dropdown |

## Spatial Profiles (3D Audio Types)

`rend_spatial_profile` is the canonical 3D audio-type selector.

| Profile | Internal enum string | Intended host output | Current behavior |
|---|---|---|---|
| Auto | `auto` | Any | Chooses best layout by channel count (13 -> 7.4.2, 10 -> 7.2.1, 8 -> 5.2.1, 4 -> quad, else stereo fallback). |
| Stereo 2.0 | `stereo_2_0` | 2ch | Stereo downmix path from quad accumulation bed. |
| Quad 4.0 | `quad_4_0` | 4ch | Direct quad output in host order `FL, FR, RL, RR`. |
| Surround 5.2.1 | `surround_5_2_1` | 8ch | 8ch bed mapping with center/LFE/top placeholders. |
| Surround 7.2.1 | `surround_7_2_1` | 10ch | 10ch bed mapping including rear surround + top center. |
| Surround 7.4.2 | `surround_7_4_2` | 13ch | 13ch bed mapping including four top channels. |
| Ambisonic FOA | `ambisonic_foa` | 4ch (or 2ch fallback) | FOA proxy encode from quad bed; decodes to stereo when host has fewer than 4 outputs. |
| Ambisonic HOA | `ambisonic_hoa` | 16ch preferred | Direct HOA flag at 16ch; falls back to FOA at 4ch, or stereo ambi decode path under 4ch. |
| Atmos Bed | `atmos_bed` | 10ch | Uses 7.2.1 bed mapping shape when direct; fallback to quad/stereo when host layout is smaller. |
| Virtual 3D Stereo | `virtual_3d_stereo` | 2ch | Stereo virtualization/crossfeed from quad bed (simulated 3D over stereo). |
| Codec IAMF | `codec_iamf` | 13ch preferred | Placeholder layout mode, currently maps to 7.4.2 bed when available. |
| Codec ADM | `codec_adm` | 13ch preferred | Placeholder layout mode, currently maps to 7.4.2 bed when available. |

## Output Layout Coverage

| Host output channels | LocusQ behavior |
|---|---|
| 1 (mono) | Sums quad bed to mono with deterministic scaling. |
| 2 (stereo) | Stereo downmix, Virtual 3D Stereo, Ambisonic decode-to-stereo, and Steam binaural path (if available). |
| 4 (quad) | Native quad rendering (`FL, FR, RL, RR`). |
| 8 (5.2.1) | Multichannel surround mapping (`Surround 5.2.1`). |
| 10 (7.2.1) | Multichannel surround mapping (`Surround 7.2.1`, `Atmos Bed`). |
| 13 (7.4.2) | Multichannel surround mapping (`Surround 7.4.2`, codec placeholders). |
| 16 (HOA lane) | HOA profile direct lane target. |

## How To Use By Mode

### Binaural (headphones)

1. Set DAW/plugin output to stereo (2ch).
2. Set plugin mode to `Renderer`.
3. In UI set `Headphone` to `Steam Binaural`.
4. Set `HP Profile` to `Generic`, `AirPods Pro 2`, `AirPods Pro 3`, `Sony WH-1000XM5`, or `Custom SOFA`.
5. Verify diagnostics in scene status:
   - `rendererHeadphoneModeRequested` / `rendererHeadphoneModeActive`
   - `rendererSteamAudioAvailable`
   - `rendererSteamAudioInitStage`

### Stereo (non-binaural)

1. Use stereo host output.
2. Set `Headphone` to `Stereo Downmix`.
3. Use `rend_spatial_profile=Stereo 2.0` or `Virtual 3D Stereo` from host automation.

### Quadraphonics (4.0)

1. Configure host output as quad/discrete-4.
2. Set `rend_spatial_profile=Quad 4.0` or `Auto`.
3. Confirm active profile/stage in diagnostics:
   - `rendererSpatialProfileActive=quad_4_0`
   - `rendererSpatialProfileStage=direct`

### Surround 7.2.1

1. Configure host output as 10ch/discrete-10.
2. Set `rend_spatial_profile=Surround 7.2.1`.
3. Confirm `rendererSpatialProfileActive=surround_7_2_1`.

### Ambisonics

1. Set `rend_spatial_profile=Ambisonic FOA` or `Ambisonic HOA` via host automation.
2. Use 4ch for FOA baseline, 16ch for HOA direct lane.
3. Verify diagnostics:
   - `rendererAmbiCompiled`, `rendererAmbiActive`
   - `rendererAmbiStage`
   - `rendererSpatialProfileStage` (`direct` or `ambi_decode_stereo` depending on host outputs)

### Mono Compatibility

1. Configure host output as mono.
2. Render/monitor with any spatial profile.
3. LocusQ deterministically collapses the scene to mono output.

## Automated Validation Commands

```bash
# Mono/stereo/quad output contract
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_mono_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_stereo_suite.json
build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_phase_2_8_output_layout_quad_suite.json

# Binaural and headphone-profile contracts
./scripts/qa-bl009-headphone-contract-mac.sh
./scripts/qa-bl009-headphone-profile-contract-mac.sh

# Ambisonic and multichannel profile matrix
./scripts/qa-bl018-ambisonic-contract-mac.sh

# REAPER host smoke (auto bootstrap + render)
./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap
```

## Current Limitations

1. `Custom SOFA` profile name exists, but full user-provided SOFA ingestion is not yet wired in renderer runtime.
2. IAMF/ADM are currently layout placeholders, not full codec encode/decode pipelines.
3. Ambisonic lanes currently use a quad-derived FOA proxy path for fallback behavior.

## Related

- `Documentation/scene-state-contract.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/backlog/index.md`
