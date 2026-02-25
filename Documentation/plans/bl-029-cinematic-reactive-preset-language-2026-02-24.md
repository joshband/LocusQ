Title: BL-029 Cinematic Reactive Preset Language
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-25

# BL-029 Cinematic Reactive Preset Language

## Purpose
Define a pre-implementation contract for cinematic audition presets so expansion work lands against a stable, deterministic reactive language instead of ad-hoc visual tuning.

This plan is intentionally v1 -> v3 staged and must be approved before additional BL-029 code expansion.

## Pre-Code Gate (G6 Authority)
This document is the authority contract for BL-029 Slice G6. New cinematic preset code expansion is blocked unless all of the following are true:
1. Mapping tables in this document are complete for every preset family in scope.
2. Acceptance IDs and numeric thresholds are explicitly defined and testable.
3. QA lane documentation references these IDs and triage paths.
4. Implementation traceability references this document as the pre-code contract source.

## Dream Contract

### Hook
Turn Audition into a cinematic spatial language where each preset communicates a clear scene mood (weather, impact, biome, synthetic score) while staying deterministic and testable.

### Description
The preset system should behave like a compact direction system:
1. A preset name implies motion profile, reactive envelope behavior, and visual behavior.
2. The same preset produces stable results under the same seed and runtime settings.
3. Reactive behavior degrades safely when bridge metadata is missing.
4. Rain/snow semantics remain visually distinct and measurable across quality tiers.

## Inputs And Constraints

### Runtime IDs Already In Use (Do Not Rename)
Signal IDs:
- `sine_440`
- `dual_tone`
- `pink_noise`
- `rain_field`
- `snow_drift`
- `bouncing_balls`
- `wind_chimes`
- `crickets`
- `song_birds`
- `karplus_plucks`
- `membrane_drops`
- `krell_patch`
- `generative_arp`

Motion IDs:
- `center`
- `orbit_slow`
- `orbit_fast`
- `figure8_flow`
- `helix_rise`
- `wall_ricochet`

### Current Scene-State Surface (Baseline)
Published scalar contract already used by QA/UI:
- `rendererAuditionDensity`
- `rendererAuditionReactivity`
- `rendererAuditionFallbackReason`
- `rendererAuditionCloud.{pattern,mode,pointCount,emitterCount,spreadMeters,pulseHz,coherence}`

Bridge/runtime constraints:
1. Renderer remains authoritative for audition metadata.
2. Mapping semantics must remain deterministic.
3. WebView fallback behavior must remain explicit when metadata is missing.
4. No audio-thread unsafe expansion is permitted.

## Framework And Complexity Decision (Plan)

Framework decision: `webview`

Rationale:
1. BL-029 visualization behavior is dominated by Three.js scene semantics and JS/native bridge contracts.
2. Cinematic preset language requires high iteration speed on visual behavior and mapping tables.
3. Acceptance requires backend-aware behavior documentation (`WKWebView`, `WebView2`) that belongs in WebView contract surfaces.

Complexity score: `4/5`

Rationale:
1. Multiple reactive layers (seeded cloud, fallback metadata behavior, preset-specific semantics).
2. Determinism and triage requirements across runtime + UI contracts.
3. Need to coordinate DSP-facing metadata with Three.js rendering rules without destabilizing current shipping paths.

Implementation strategy: phased (`v1`, `v2`, `v3`).

## v1 -> v3 Contract

### v1: Lexicon Freeze And Baseline Semantics
Goals:
1. Freeze preset naming and cloud-mode mapping vocabulary.
2. Freeze rain/snow differentiation semantics.
3. Freeze deterministic replay lane for stable mode family.

Contract:
1. Every preset maps to one canonical `rendererAuditionCloud.mode` token.
2. Rain and snow remain distinct:
   - rain: line streak semantics enabled.
   - snow: diffuse flake semantics with no line streak semantics.
3. Deterministic lane verifies replay hash stability for stable modes.

### v2: Reactive Envelope Contract
Goals:
1. Introduce explicit reactive envelope schema as additive metadata.
2. Define clamp, smoothing, and deadband semantics per feature.
3. Keep fallback behavior deterministic when reactive block is missing.

Proposed additive scene-state block:
- `rendererAuditionReactive.rms`
- `rendererAuditionReactive.peak`
- `rendererAuditionReactive.envFast`
- `rendererAuditionReactive.envSlow`
- `rendererAuditionReactive.onset`
- `rendererAuditionReactive.brightness`
- `rendererAuditionReactive.rainFadeRate`
- `rendererAuditionReactive.snowFadeRate`
- `rendererAuditionReactive.sourceEnergy[]`

Fallback contract:
1. If reactive block missing, UI uses scalar fallback (`rendererAuditionReactivity`) and existing fallback-state reader.
2. Fallback status text remains explicit to avoid silent visual drift.

### v3: Cinematic Authoring Language
Goals:
1. Add per-preset cinematic profile descriptors and quality-tier semantics.
2. Define profile-level acceptance thresholds and QA IDs.
3. Require backend parity notes (`WKWebView`, `WebView2`) before release promotion.

Proposed profile descriptor fields:
- `presetFamily`
- `mood`
- `kineticStyle`
- `densityBand`
- `reactiveProfile`
- `fadeProfile`
- `qualityProfile`

## Versioned Delivery Contract

| Version | Contract Deliverables | Determinism Rule | Release Gate |
|---|---|---|---|
| `v1` | Lexicon freeze, weather semantic separation, baseline deterministic replay lane | same mode + seed + scenario must keep replay hash stable | required before adding new preset IDs |
| `v2` | Additive reactive envelope schema, fallback behavior, bounded ranges | all reactive scalars finite and clamped in `[0,1]` | required before reactive-driven visuals are expanded |
| `v3` | Cinematic dictionary (`family/mood/kinetic/fade`) + backend parity notes + QA diagnostics | same family + mode + seed resolves to the same profile tokens | required before preset-authoring UI/UX expansion |

## Cinematic Mapping Table (v1 baseline -> v3 target)

| Preset Family | Signal IDs | Reactive Inputs | Transform Contract | Visual Targets (Three.js) | Acceptance Threshold |
|---|---|---|---|---|---|
| Tonal Core | `sine_440`, `dual_tone` | `rendererAuditionReactivity`, `pulseHz` | clamp `[0,1]`, EMA fast 120 ms, deadband `0.02` | centroid glow, low-density orbit points | deterministic hash stable across 2 runs |
| Noise Atmosphere | `pink_noise` | `density`, `coherence` | clamp `[0,1]`, coherence floor `0.15` | diffuse halo with soft opacity modulation | no `NaN/Inf`; coherence always finite |
| Rain Weather | `rain_field` | `rainFadeRate`, `envFast`, `onset` | fade rate clamp `[0,1]`, onset hysteresis `0.05` | line streaks + downward flow | rain line-opacity baseline > snow by at least `0.20` |
| Snow Weather | `snow_drift` | `snowFadeRate`, `envSlow` | fade rate clamp `[0,1]`, slow envelope tau >= 300 ms | diffuse flakes, no streak lines | snow line-opacity remains `0.0` in profile contract |
| Impact Swarm | `bouncing_balls`, `membrane_drops` | `peak`, `onset`, `sourceEnergy[]` | peak clamp `[0,1]`, onset threshold `>=0.12` | bounce arcs + collision flashes | wall-hit visual cues occur without exceeding opacity cap `0.92` |
| Chime/Bio Cluster | `wind_chimes`, `crickets`, `song_birds` | `brightness`, `envSlow` | brightness clamp `[0,1]`, slow smoothing 250-400 ms | canopy points, sparse connective lines | no jitter bursts > 2 consecutive frames at 60 Hz target |
| Cinematic Synthesis | `krell_patch`, `generative_arp`, `karplus_plucks` | `envFast`, `brightness`, `sourceEnergy[]` | fast envelope tau 80-140 ms, brightness gamma `1.2` | lattice/spiral animated structures | deterministic mode + seed reproduces identical source layout |

## Preset Language Dictionary (v3 target)

| Family Token | Mood Token | Kinetic Token | Fade Token | Default Motion |
|---|---|---|---|---|
| `weather_rain` | `storm_tension` | `downward_sheet` | `precip_fast` | `helix_rise` |
| `weather_snow` | `quiet_cold` | `drift_diffuse` | `precip_slow` | `orbit_slow` |
| `impact_cluster` | `kinetic_alert` | `ricochet_burst` | `impact_decay` | `wall_ricochet` |
| `bio_canopy` | `organic_air` | `swarm_flutter` | `bio_breathe` | `figure8_flow` |
| `synth_cinema` | `speculative_motion` | `spiral_lattice` | `synth_swell` | `orbit_fast` |

## Reactive Feature Mapping (v2/v3)

| Feature | Source | Normalize | Smoothing | Deadband/Hysteresis | Visual Parameter |
|---|---|---|---|---|---|
| `rms` | renderer reactive block | clamp `[0,1]` | EMA 220 ms | `0.01` | base cloud opacity |
| `peak` | renderer reactive block | clamp `[0,1]` | hold 45 ms, decay 180 ms | none | impact flash amplitude |
| `envFast` | renderer reactive block | clamp `[0,1]` | EMA 100 ms | `0.015` | pulse intensity |
| `envSlow` | renderer reactive block | clamp `[0,1]` | EMA 320 ms | `0.01` | drift amplitude |
| `onset` | renderer reactive block | clamp `[0,1]` | n/a (event-like) | trigger `>=0.12`, release `<0.06` | transient burst spawn |
| `brightness` | renderer reactive block | clamp `[0,1]` | EMA 180 ms | `0.02` | color lift/tint mix |
| `rainFadeRate` | renderer reactive block | clamp `[0,1]` | EMA 140 ms | `0.02` | rain streak alpha decay |
| `snowFadeRate` | renderer reactive block | clamp `[0,1]` | EMA 320 ms | `0.02` | snow point alpha decay |
| `sourceEnergy[]` | renderer reactive block | clamp per source `[0,1]` | EMA 120 ms | `0.03` | per-source point scale/alpha |

## Acceptance Threshold Ledger (Release-Gating)

| ID | Version | Threshold | Pass Condition |
|---|---|---|---|
| `G6-V1-01` | `v1` | `13/13` canonical signal IDs map to known family/mode vocabulary | no unknown or missing preset family mapping |
| `G6-V1-02` | `v1` | rain line opacity baseline minus snow line opacity baseline `>= 0.20` | weather presets remain visually distinct |
| `G6-V1-03` | `v1` | replay hash equality across `>=2` deterministic runs | no hash drift for stable modes |
| `G6-V2-01` | `v2` | all required reactive fields present and finite | no `NaN/Inf` values in contract block |
| `G6-V2-02` | `v2` | scalar bounds: `0.0 <= value <= 1.0` for clamp-governed fields | no out-of-range telemetry |
| `G6-V2-03` | `v2` | fallback activates within one UI update when reactive block missing | no silent contract failure |
| `G6-V2-04` | `v2` | `rainFadeRate` and `snowFadeRate` remain bounded `[0,1]` and diverge by profile | precipitation profiles do not collapse to identical behavior |
| `G6-V3-01` | `v3` | `100%` preset families resolve to canonical mood/kinetic/fade tokens | no unresolved cinematic profiles |
| `G6-V3-02` | `v3` | mapping table complete for all cinematic families | no `TBD` entries in visual mapping contract |
| `G6-V3-03` | `v3` | backend notes completed for `WKWebView` and `WebView2` | parity deltas documented before promotion |
| `G6-V3-04` | `v3` | QA lane emits explicit per-check PASS/FAIL diagnostics + deterministic seed evidence path | no opaque aggregate-only QA status |

## Acceptance Matrix

### v1 Acceptance (Lexicon Freeze)
1. `G6-V1-01`: all required cloud mode tokens present in renderer source.
2. `G6-V1-02`: rain/snow UI pattern aliases present.
3. `G6-V1-03`: deterministic replay hash stable for stable-mode set.

### v2 Acceptance (Reactive Envelope)
1. `G6-V2-01`: reactive envelope fields present and finite.
2. `G6-V2-02`: reactive scalar range validity (`0.0 <= value <= 1.0`).
3. `G6-V2-03`: fallback semantics active when reactive block missing.
4. `G6-V2-04`: rain/snow fade-rate fields publish deterministic bounded values.

### v3 Acceptance (Cinematic Authoring)
1. `G6-V3-01`: each preset family resolves to canonical mood/kinetic/fade tokens.
2. `G6-V3-02`: Three.js profile mapping table fully defined for all cinematic families.
3. `G6-V3-03`: backend parity note completed for `WKWebView` and `WebView2`.
4. `G6-V3-04`: QA lane emits per-check diagnostics and deterministic seed evidence bundle.

## QA Lane Impact (Pre-Code Expansion)

Required lane checks to preserve during implementation:
1. `app_exited_before_result` detection.
2. Reactive contract field/range checks.
3. Missing-block fallback checks.
4. Rain/snow fade semantics checks.
5. Deterministic replay hash checks.

Evidence bundle pattern:
- `TestEvidence/bl029_audition_reactive_qa_slice_g3_<timestamp>/` for current runtime QA.
- `TestEvidence/bl029_cinematic_reactive_preset_language_slice_g6_<timestamp>/` for this docs/design contract.

## Risks

| Risk | Severity | Mitigation |
|---|---|---|
| Preset semantics drift between renderer and UI | High | lock dictionary in this plan; enforce token checks in QA lane |
| Reactive envelope over-coupled to frame timing | High | define smoothing/deadband contract before implementation |
| Rain/snow visual convergence over time | Medium | hard threshold on line opacity and fade profile divergence |
| Backend-specific behavior mismatch | Medium | explicit parity notes for `WKWebView` and `WebView2` before v3 sign-off |

## Implementation Slices After G6 Approval
1. `G6-A`: add additive `rendererAuditionReactive` publication contract to scene-state.
2. `G6-B`: implement UI consumption with strict fallback path and bounded transforms.
3. `G6-C`: bind cinematic preset dictionary to deterministic Three.js profile handlers.
4. `G6-D`: extend QA lane with v2/v3 checks and seed replay expansion.
5. `G6-E`: closeout docs + evidence sync after runtime implementation.

## Exit Criteria
This plan is complete when:
1. v1/v2/v3 contracts are approved as pre-implementation authority.
2. Mapping tables are used as the only accepted source for reactive/preset semantics.
3. Acceptance matrix IDs are adopted by QA lane and closeout evidence workflows.
4. Expansion tasks cite this contract before source-level feature growth resumes.
