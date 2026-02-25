Title: Section 0 Integration Recommendations for LocusQ
Document Type: Research Integration Recommendations
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Section 0 Integration Recommendations for LocusQ

## Baseline Compared
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md` Section 0 research findings.
- `Source/SpatialRenderer.h` current renderer chain (quad accumulation, stereo matrix downmix, room FX stage).
- `Source/FDNReverb.h` current late reverb (4x4 Hadamard FDN, static delays).
- `.ideas/architecture.md` design intent (post-draft target includes richer room/viewport behavior).

## Recommendations (Opinionated)

| ID | What to integrate (library/pattern) | What it replaces or augments | LOC estimate | Risk assessment | Recommended timing |
|---|---|---|---:|---|---|
| R1 | **Steam Audio C API** binaural path (`iplBinauralEffectApply`) for headphone mode | Augments current stereo downmix in `SpatialRenderer` with true HRTF render mode; keep existing stereo matrix as fallback | 300-550 | **Medium**: emitter-count CPU scaling, third-party dependency management, strict non-RT lifecycle rules for Steam objects | **v1.1** |
| R2 | **8x8 modulated FDN pattern** (SAF/SPARTA-inspired topology, implemented locally) | Replaces current 4x4 static `FDNReverb` tail with higher-diffusion late reverb aligned to architecture intent | 450-800 | **High**: RT stability/tuning risk (tail buildup, modulation artifacts), higher CPU budget, more QA matrix surface | **v2** |
| R3 | **`clap-juce-extensions`** integration for CLAP format output | Augments current VST3/AU-only distribution with CLAP build target and host-validation lane | 250-500 | **Medium**: CI/release complexity increase and extra host-compatibility permutations | **v2** |
| R4 | **Audio-reactive viewport telemetry pattern** (renderer RMS -> WebView uniforms/overlays) | Augments current visualization path with live speaker-energy feedback; does not alter DSP math | 180-320 | **Low-Medium**: bridge payload discipline and UI frame-rate throttling required to avoid UI-side regressions | **v1.1** |
| R5 | **PHASE exclusion guardrail** (do not integrate PHASE inside plugin DSP path) | Replaces ad hoc evaluation with explicit architecture rule: plugin keeps internal renderer; PHASE only reconsidered via standalone companion route | 40-80 | **Low** if enforced; **Very High** if ignored (PHASE audio-graph ownership conflicts with plugin `processBlock`) | **never** |

## Priority Order
1. R1 (headphone quality lift with bounded scope).
2. R4 (viewport value with low DSP risk).
3. R2 (reverb quality expansion after hardening cycle).
4. R3 (format/distribution expansion once DSP roadmap stabilizes).
5. R5 (codify and keep as an invariant now).
