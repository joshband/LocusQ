Title: BL-029 Audition Platform Expansion Plan
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# BL-029 Audition Platform Expansion Plan

## Purpose
Define a concrete next-step architecture for LocusQ Audition so it can behave like emitter-driven, physics-reactive, choreography-reactive, and standalone demo content while preserving renderer correctness and deterministic repeatability.

## Product Intent (Dream Contract)

### Hook
Turn Audition into a cinematic immersive spatial stage: high-quality synthetic/procedural sources, deterministic motion, and multi-point emitter clouds that can showcase full 3D rendering even without a DAW signal.

### Description
Audition should support a hybrid role:
1. Demonstration content (rain, snow, chimes, impacts, drones, tonal references).
2. Diagnostic content (chirps/noise impulses and controlled sweeps).
3. Reactive behavior (physics/choreography/audio-reactive bindings) with deterministic replay.
4. Standalone-first operation when no host track or hardware input is present.

## Scope

In scope:
1. Renderer-authoritative audition engine with cross-mode control bindings.
2. Deterministic source generation and deterministic motion replay.
3. Multi-emitter cloud metadata and binding contracts for UI/runtime.
4. Standalone-safe operation with no external input dependency.

Out of scope:
1. Replacing emitter rendering as the main production audio path.
2. Audio-thread ML inference or unbounded procedural synthesis.
3. Breaking APVTS IDs already used by existing sessions.

## Parameter Evolution (Plan-Level)

Existing controls to retain and strengthen:
| ID | Current Role | Planned Upgrade |
|---|---|---|
| `rend_audition_enable` | Global audition gate | Keep as authoritative master gate; explicit fail-safe default `off`. |
| `rend_audition_signal` | Content selector | Expand to content families and quality tiers while preserving existing IDs. |
| `rend_audition_motion` | Motion preset | Extend to deterministic choreography/physics bindings. |
| `rend_audition_level` | Fixed level presets | Keep stepped presets; add deterministic gain staging profile mapping. |

Planned additive controls (non-breaking):
| ID (proposed) | Type | Purpose |
|---|---|---|
| `rend_audition_source_mode` | Choice | `single`, `cloud`, `bound_emitter`, `bound_choreography`, `bound_physics`, `input_reactive`. |
| `rend_audition_binding_target` | Choice/String | Which emitter/lane/physics profile to follow when bound modes are active. |
| `rend_audition_seed` | Int | Deterministic source and cloud layout seeding. |
| `rend_audition_transport_sync` | Bool | Lock to timeline/tempo token transport when available. |
| `rend_audition_density` | Choice | Controls point/source count envelope under fixed caps. |
| `rend_audition_reactivity` | Choice | Sensitivity/smoothing profile for reactive modes. |

## Architecture Decision (Plan)
Authoritative rendering remains in Renderer domain; other modes expose control surfaces that write to renderer audition state.

Signal/control model:

```text
Emitter/Choreo/Physics UI actions
  -> Audition Binding Resolver (message thread)
  -> Renderer Audition State (APVTS + runtime snapshot)
  -> SpatialRenderer Audition Engine (audio thread, RT-safe)
  -> Scene-State Telemetry (message thread)
  -> Three.js audition visualization
```

Core runtime components:
1. `AuditionBindingResolver` (message thread): resolves requested source bindings to stable runtime descriptors.
2. `AuditionGeneratorBank` (audio thread, preallocated): deterministic source generators and procedural fields.
3. `AuditionCloudMixer` (audio thread, fixed-capacity): bounded multi-point source accumulation.
4. `AuditionDeterminismState` (shared snapshot): seed, transport phase, repeatable profile IDs.
5. `AuditionTelemetryPublisher` (message thread): additive scene-state metadata for UI and selftest.

## Determinism Contract
1. Every audition run has explicit seed + profile ID.
2. Timebase derives from sample counter and optional transport lock; never from wall-clock randomness.
3. Cloud source positions are generated from seeded deterministic functions and bounded fixed arrays.
4. Reactive input modes must include smoothing profile IDs and deterministic fallback when input is absent.

## Standalone Contract
1. Audition must function with zero host emitters and zero DAW signal.
2. If hardware input is unavailable, reactive modes degrade to deterministic synthetic fallback instead of failing startup.
3. UI status must clearly indicate active source mode and fallback path.

## Complexity Assessment
Score: 4 (Expert)

Rationale:
1. Cross-domain control authority plus renderer-domain exclusivity constraints.
2. Deterministic procedural synthesis and multi-point spatial mixing.
3. Tight RT-safety requirements with richer telemetry and selftest coverage.
4. Backward compatibility with existing APVTS/session contracts.

## Implementation Strategy (Phased)

### Phase A: Contract and Authority Hardening
1. Lock authority model and additive schema (`rendererAuditionCloud` + source mode fields).
2. Add deterministic seed/transport fields in scene-state payload.
3. Expand selftest assertions for scope `bl029`.

### Phase B: Source Binding Runtime
1. Add binding resolver for emitter/choreography/physics ownership without duplicating renderer DSP paths.
2. Add explicit fallback semantics for unavailable binding targets.

### Phase C: Generator Quality Expansion
1. Upgrade source models (rain/snow/chimes/impacts/tones/noise) with quality tiers.
2. Add fixed-capacity cloud mixer and decorrelation profiles.

### Phase D: Cross-Mode UX
1. Add Emitter/Choreo/Physics "Audition This" controls that write renderer audition bindings.
2. Keep renderer panel as canonical status/authority display.

### Phase E: Validation + Release Hardening
1. Add deterministic QA lane for audition-cloud showcase and binding replay.
2. Run build, smoke, scoped selftests, RT audit, docs freshness.

## Risks and Mitigations
| Risk | Severity | Mitigation |
|---|---|---|
| Authority confusion across panels | High | Renderer-owned state only; cross-mode controls are proxies with clear status badges. |
| RT regressions from richer generators | High | Fixed-capacity buffers, preallocation, no locks/allocs in audio thread. |
| Nondeterministic showcase behavior | Medium | Seed + transport lock + deterministic fallback contracts. |
| Backward compatibility drift | Medium | Additive schema only, legacy signal/motion paths remain valid. |

## Validation Matrix (Required)
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
3. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh`
4. `./scripts/rt-safety-audit.sh --print-summary --output <artifact.tsv>`
5. `./scripts/validate-docs-freshness.sh`

## Evidence Contract
Use a per-slice bundle:
`TestEvidence/bl029_audition_platform_<slice>_<timestamp>/`
with:
1. `status.tsv`
2. `build.log`
3. `qa_smoke.log` (when native changes are included)
4. `selftest.log`
5. `contract_diff.md`
