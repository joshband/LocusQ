Title: BL-029 Audition Platform QA Lane
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-25

# BL-029 Audition Platform QA Lane

## Purpose
Define the deterministic QA lane for BL-029 audition reactive closeout slices (G3/G4) so audition-platform behavior is validated as both:
1. A standalone showcase path.
2. A reactive/proxy metadata contract path with explicit reactive envelope, precipitation fade semantics, and fallback hardening.
3. A binaural parity path where reactive fade drivers remain perceptually stable across stereo downmix and Steam binaural modes.

This testing guide is also the QA companion contract for BL-029 Slice G6:
- Canonical plan/design contract: `Documentation/plans/bl-029-cinematic-reactive-preset-language-2026-02-24.md`
- Pre-expansion rule: cinematic preset runtime growth should not proceed until G6 acceptance IDs are represented by deterministic QA checks or explicit docs-only parity gates.

## Scope
Owned lane artifacts:
1. `qa/scenarios/locusq_audition_platform_showcase.json`
2. `scripts/qa-bl029-audition-platform-lane-mac.sh`
3. `TestEvidence/bl029_audition_reactive_qa_slice_g3_<timestamp>/...`

The lane is build-independent in execution posture: it expects prebuilt binaries and performs replay + contract checks without triggering a build itself.

## Contract Coverage
The scenario and lane enforce release-grade checks:
1. `cloud_showcase_mode`
   - Validates expected audition cloud mode family mappings are present in renderer scene-state publication logic.
2. `bound_proxy_mode_behavior`
   - Validates the UI proxy path ingests `rendererAuditionCloud` fields and maps mode/spread/coherence into `auditionEmitterTarget.cloud`.
3. `reactive_envelope_contract`
   - Validates renderer publishes reactive envelope scene-state fields and clamp/range semantics for reactive scalars.
4. `reactive_missing_block_fallback`
   - Validates fallback semantics are present when reactive block metadata is unavailable at the JS/native boundary.
5. `rain_snow_fade_semantics`
   - Validates deterministic rain/snow publication and UI fade-token semantics (line opacity + pattern/count mappings).
6. `bound_mode_contract`
   - Validates renderer scene-state bound/source authority fields and expected source/binding values.
7. `fallback_reason_contract`
   - Validates renderer fallback reason field and reason-value enumeration, plus UI fallback token coverage.
8. `deterministic_seed_replay`
   - Runs the same scenario multiple times and asserts identical `wet.wav` SHA-256 across replay runs.
9. `binaural_reactive_parity`
   - Validates parity fields under `rendererAuditionReactive` (`headphoneOutputRms`, `headphoneOutputPeak`, `headphoneParity`) remain finite/bounded and stable across stereo downmix and Steam binaural.
10. `binaural_fallback_telemetry`
   - Validates explicit fallback telemetry in `rendererAuditionReactive` (`headphoneFallback`, `headphoneFallbackReason`) matches renderer requested/active Steam diagnostics for deterministic failure triage.
11. `reactive_mode_replay_matrix`
   - Validates reliability-mode matrix coverage for key reactive families (`precipitation_rain`, `precipitation_snow`, `chime_cluster`, `impact_swarm`) across plugin mode tokens and UI pattern handlers.
12. `reactive_telemetry_bounds_contract`
   - Validates additive reactive telemetry publication includes envelope + coupling + headphone parity fields and normalized bounded variants (`*Norm` fields).

Current harness note:
- The spatial QA adapter exposes renderer/headphone/spatial profile controls but not dedicated `rend_audition_*` parameters yet.
- This lane therefore proves deterministic runtime stability via replay and validates audition-specific metadata contracts through source-token checks in renderer/UI publication paths.

## Slice P5 Reactive Geometry Mapping (Bounded)
P5 geometry/fade behavior uses `rendererAuditionReactive` as an additive driver layer on top of cloud pattern defaults.

| Feature | Formula (UI mapping) | Clamp | Expected range/behavior |
|---|---|---|---|
| Cloud breadth/radius morph | `sourceRadius *= 0.82 + 0.20*intensity + 0.26*spread + 0.24*physicsDensity + 0.20*physicsCoupling` | `[0.72, 1.88]` multiplier, then radius `[0.10, 6.2]` | Higher spread/density/coupling expands cloud footprint without unbounded drift. |
| Cloud height morph | `sourceHeight *= 0.78 + 0.24*envSlow + 0.22*brightness + 0.20*physicsDensity + 0.24*physicsCollision` | `[0.70, 1.84]` multiplier, then height `[0.08, 4.4]` | Height reacts to envelope and physics activity while staying bounded. |
| Global pulse size/opacity | `pointSize *= 0.86 + 0.16*onset + 0.24*collisionPulse + 0.18*physicsCoupling`; `lineOpacity *= 0.84 + 0.30*physicsCoupling + 0.34*collisionPulse` | point size `[0.05, 0.24]`, line opacity `[0.0, 0.72]` | Collision/coupling drive visible pulse depth and cinematic bloom without runaway values. |
| Rain fade + droplet tail | `lineOpacityScale *= (0.36 + 1.10*rainFadeRate) * (0.86 + 0.24*physicsCoupling + 0.28*collisionPulse)`; tail length uses `(0.62 + 0.74*rainFadeRate + 0.20*collisionPulse)` | line scale `[0.10, 1.62]` | Rain opacity/tails fade by fall progress plus `rainFadeRate`, with physics accents on impacts. |
| Snow breadth/depth morph | `snowBreadthMorph = 0.58 + 0.72*(0.52*brightness + 0.48*spread) + 0.24*physicsDensity`; `snowDepthMorph = 0.56 + 0.76*(0.34*brightness + 0.66*spread) + 0.18*physicsCoupling` | breadth `[0.54, 1.60]`, depth `[0.52, 1.64]` | Snow drift width/depth follows brightness/spread with density/coupling stabilization. |
| Snow vertical fade + size | `verticalFade = (1-verticalDrift) * (0.52+0.48*envSlow) * (0.66+0.34*coherence) * (0.68+0.32*snowFadeRate)` then opacity/size scales from `verticalFade` | verticalFade `[0.08, 1.35]`, opacity `[0.10, 1.40]`, size `[0.42, 1.66]` | Snow fades by vertical drift + slow envelope and remains smooth/deterministic. |
| Collision burst/pulse intensity | `collisionPulse = smooth(0.68*physicsCollision + 0.22*physicsCoupling + 0.10*onsetGate)` and `collisionBurst = (0.58*collisionPulse + 0.42*physicsCoupling) * (0.40 + 0.60*sin(...))` | both `[0, 1]` | Impact-style visuals (especially `bounce_cluster`) get deterministic burst energy with hysteresis smoothing. |

Fallback rules preserved:
1. Missing/invalid cloud payload keeps single-glyph fallback.
2. Non-cloud/disabled paths clear cloud draw ranges each frame (no stale cloud persistence).
3. Unknown reactive fields are ignored; mapping remains additive/backward-compatible.

## How to Run
1. Ensure binaries exist:
   - `build_local/locusq_qa_artefacts/Release/locusq_qa`
   - `build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app`
2. Execute lane (default output uses Slice G3 artifact naming):

```bash
./scripts/qa-bl029-audition-platform-lane-mac.sh
```

Optional overrides:
- `BL029_QA_BIN`
- `BL029_SCENARIO_PATH`
- `BL029_OUT_DIR`

Recommended explicit run:

```bash
BL029_OUT_DIR="TestEvidence/bl029_audition_reactive_qa_slice_g3_$(date -u +%Y%m%dT%H%M%SZ)" \
./scripts/qa-bl029-audition-platform-lane-mac.sh
```

## Command Contract
Owner validation contract for Slice G3:

```bash
cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
./scripts/qa-bl029-audition-platform-lane-mac.sh
./scripts/rt-safety-audit.sh --print-summary --output <artifact_dir>/rt_audit.tsv
./scripts/validate-docs-freshness.sh
```

Z3 reactive UI consolidation contract (repeatability/stability lane):

```bash
for run in 1 2 3 4 5; do
  LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh
done
```

Expected BL-029 Z3 acceptance IDs in self-test payload:
1. `UI-P1-029A`: defensive schema-additive cloud/reactive parsing + rain transform boundedness.
2. `UI-P1-029B`: startup/invalid-payload fallback preserves single-glyph path (no throw, cloud disabled).
3. `UI-P1-029C`: reactive binaural parity telemetry (`headphoneOutputRms`, `headphoneOutputPeak`, `headphoneParity`) is finite and bounded.

R3 reliability soak + go/no-go contract:

```bash
BL029_OUT_DIR="TestEvidence/bl029_reliability_soak_r3_<timestamp>" \
./scripts/qa-bl029-audition-platform-lane-mac.sh

for run in 1 2 3 4 5 6 7 8 9 10; do
  LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh
done

for run in 1 2 3 4 5; do
  LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh
done
```

## Hard Pass Criteria (R3 Go/No-Go)
`GO` requires all of the following:
1. `qa-bl029-audition-platform-lane-mac.sh` is `PASS` with zero contract failures.
2. Deterministic replay hash checks pass with configured replay run count for key reactive families.
3. Reactive telemetry bounds contract passes (envelope + coupling + parity fields and normalized variants present).
4. BL-029 self-test soak pass-rate is `100%` (`10/10` runs pass with no `app_exited_before_result`).
5. BL-009 parity soak pass-rate is `100%` (`5/5` runs pass with `UI-P1-009` green).

Any violation yields `NO-GO`.

## S3 UI Determinism Guard (BL009 Compact Rail)
S3 adds a UI-side determinism guard for transient startup layout races that could produce false negatives in compact/tight emitter rail assertions:
1. Self-test rail measurements use an authoritative layout variant override (`base`/`compact`/`tight`) instead of transient viewport-only dimensions.
2. `UI-P1-025E` uses a bounded settle window with strict timeout and stable-sample requirements before asserting rail shrink ordering.
3. No unbounded loops are permitted in this path; timeout exits fail fast with explicit `responsive layout settle failed (...)` detail.
4. BL-029 reactive cloud and fallback semantics remain unchanged by this guard.

## Expected Artifacts
Per run:
- `status.tsv` (machine-readable check status)
- `qa_lane.log` (full lane execution log)
- `scenario_result.log` (replay/hash summary)
- `lane_contract.md` (human-readable lane snapshot)
- scenario run copies (`scenario_run_*.result.json`, `scenario_run_*.wet.wav`)

Follow-up validation commands for closeout lanes may add:
- `rt_audit.tsv`
- `docs_freshness.log`

R3 soak evidence bundle should include:
- `status.tsv`
- `soak_runs.tsv`
- `qa_lane.log`
- `scenario_result.log`
- `go_no_go.md`
- `docs_freshness.log`

## G6 Acceptance Alignment
The lane must preserve traceable alignment to BL-029 Slice G6 acceptance IDs:

| G6 ID | QA Contract Mapping |
|---|---|
| `G6-V1-01` | cloud mode + preset vocabulary checks (`cloud_showcase_mode`) |
| `G6-V1-02` | rain/snow semantic differentiation checks (`rain_snow_fade_semantics`) |
| `G6-V1-03` | deterministic replay hash checks (`deterministic_seed_replay`) |
| `G6-V2-01` | reactive envelope field presence/finite checks (`reactive_envelope_contract`) |
| `G6-V2-02` | reactive range validation checks (`reactive_envelope_contract*_range`) |
| `G6-V2-03` | missing reactive block fallback checks (`reactive_missing_block_fallback`) |
| `G6-V2-04` | precipitation fade-rate boundedness checks (`rain_snow_fade_semantics*`) |
| `G6-V3-04` | explicit per-check diagnostics in `status.tsv` + deterministic evidence outputs |

Docs-only gates:
- `G6-V3-01`, `G6-V3-02`, and `G6-V3-03` are currently enforced by plan/design contract review and backend parity notes until dedicated runtime checks are added.

## Pass/Fail Rules
PASS requires:
1. Scenario execution success with `result.status=PASS`.
2. All expected cloud/proxy contract tokens present.
3. Reactive envelope + missing-block fallback contract checks pass.
4. Rain/snow fade semantic checks pass.
5. Bound-mode and fallback-reason contract checks pass.
6. Replay hash equality across configured runs.
7. Binaural reactive parity/fallback telemetry checks pass (when BL-009 selftest scope is enabled).

FAIL triggers include:
1. Missing QA binary or scenario file.
2. `app_exited_before_result` signature in scenario run log.
3. Missing expected `rendererAuditionCloud` mode/token contracts.
4. Missing or invalid reactive envelope contract fields/range semantics.
5. Missing reactive fallback semantics when reactive metadata is absent.
6. Missing rain/snow semantic tokens in renderer or UI.
7. Missing bound-mode source/binding contract values.
8. Missing fallback-reason field/reason values/UI tokens.
9. Replay hash mismatch.
10. Missing scenario artifacts (`result.json`, `wet.wav`).
11. Missing/invalid `rendererAuditionReactive` headphone parity fields.
12. Missing/invalid headphone fallback telemetry fields or inconsistent values vs renderer Steam diagnostics.

## Failure Triage
Use `status.tsv` first, then inspect referenced artifacts:
1. `scenario_run_*_app_exited_before_result=FAIL`
   - Run terminated before scenario artifacts were published.
   - Inspect `scenario_run_*.log` first, then re-run locally in an interactive host session if needed.
2. `scenario_run_*_exec=FAIL`
   - Open `scenario_run_*.log` and `qa_output/locusq_spatial/<scenario>/result.json`.
   - Common causes: invalid scenario contract, missing QA binary, adapter preparation errors.
3. `reactive_envelope_contract*=FAIL` or `reactive_missing_block_fallback=FAIL`
   - Reactive contract is missing or range/metadata semantics changed.
   - Confirm `rendererAuditionReactivity` publication and `[0..1]` clamp token in `Source/PluginProcessor.cpp`.
   - Confirm fallback tokens remain in `Source/ui/public/js/index.js`.
4. `rain_snow_fade_semantics*=FAIL`
   - Rain/snow deterministic semantics drifted between renderer publication and UI mapping.
   - Confirm rain/snow pattern aliases and fade token assignments in `Source/ui/public/js/index.js`.
5. `cloud_showcase_mode` or `bound_proxy_mode_behavior=FAIL`
   - Contract drift between scene-state publication and UI ingestion.
   - Confirm expected tokens still exist in `Source/PluginProcessor.cpp` and `Source/ui/public/js/index.js`.
6. `bound_mode_contract*=FAIL`
   - Bound/source authority fields or expected values changed.
   - Confirm `rendererAuditionSourceMode` and `rendererAuditionBindingTarget` publication.
7. `fallback_reason_contract*=FAIL`
   - Fallback-reason keys/enumeration changed or UI fallback messaging path drifted.
   - Confirm `rendererAuditionFallbackReason` and expected reason literals are still present.
8. `deterministic_seed_replay=FAIL`
   - Nondeterministic runtime path or changed render contract.
   - Compare per-run `wet.wav` hashes and `scenario_run_*.result.json` metric diffs.
9. `binaural_reactive_parity=FAIL` or `binaural_fallback_telemetry=FAIL`
   - Reactive parity bridge drifted between stereo and Steam binaural paths.
   - Confirm `rendererAuditionReactive.headphoneParity` and fallback-reason publication in `Source/PluginProcessor.cpp` and mode-resolution logic in `Source/SpatialRenderer.h`.

## Evidence Path
`TestEvidence/bl029_audition_reactive_qa_slice_g3_<timestamp>/`

This path is the canonical evidence bundle for BL-029 Reactive Contract QA Slice G3 runs.
