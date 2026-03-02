Title: BL-077 Unified Visual Capture Harness Planning Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-02

# BL-077 Unified Visual Capture Harness Planning Packet

## Scope Baseline

- Existing seed script: `scripts/capture-headtracking-rotation-mac.sh`.
- Current strengths: guided countdown, configurable cue density, frame extraction, summary markdown.
- Current gaps blocking broader backlog velocity:
  - no profile schema/versioning contract,
  - no deterministic session manifest/hash lane,
  - no standard QA wrapper (`qa-bl077-*`) for replay cadence,
  - limited first-class ergonomics for two-app synchronized capture and targeted inversion triage.

## Complexity and Delivery Strategy

- Complexity score: `4/5` (multi-process orchestration + deterministic evidence + operator UX constraints).
- UI framework decision: `webview-neutral CLI-first`.
- Rationale: this lane is a QA platform workflow, not a plugin runtime UI surface. A stable CLI + artifact contract is the safest integration point for both local operators and automation harness consumers.
- Implementation strategy: phased (wave-based) delivery with replay gates.

## Architecture Components

1. Capture Session Orchestrator
- Parses profile and run arguments, initializes workspace, validates permissions/dependencies.

2. Profile Contract Loader
- Loads `scripts/capture_profiles/<name>.json` and resolves canonical timing/checkpoint schema.

3. Cue Engine
- Emits terminal cues, optional speech cues, and checkpoint markers with deterministic timestamps.

4. Recorder Backend
- Wraps macOS capture tooling (`ffmpeg` avfoundation primary, optional fallback path) with strict log capture.

5. Post-Processing Pipeline
- Extracts frames, labels checkpoints, creates contact sheets and cue-window clips.

6. Evidence Manifest Writer
- Publishes `status.tsv`, `session_manifest.json`, `artifact_schema_inventory.tsv`, and replay hash rows.

7. QA Lane Adapter
- `scripts/qa-bl077-capture-harness-mac.sh` executes contract and execute modes across replay tiers.

## Processing Contract

```text
Profile + CLI args
-> preflight validation
-> countdown + cue schedule
-> recording
-> post-process artifacts
-> deterministic manifest + replay hashes
-> lane verdict (PASS/WARN/FAIL)
```

## Profile Schema (v1 Draft)

| Key | Type | Required | Example | Notes |
|---|---|---|---|---|
| `schema` | string | yes | `locusq-capture-profile-v1` | Contract version guard. |
| `session_name` | string | yes | `headtracking_dense` | Human-readable profile ID. |
| `duration_sec` | number | yes | `70` | Total recording duration. |
| `fps` | number | yes | `30` | Recorder frame rate. |
| `extract_every_sec` | number | yes | `0.25` | Frame extraction cadence. |
| `countdown_sec` | number | yes | `5` | Pre-roll countdown. |
| `cue_points` | array | yes | `[{"t":0,"label":"center_sync"}]` | Ordered checkpoint contract. |
| `windows` | array | no | `[{"app":"LocusQ"}]` | Optional target/validation hints. |
| `artifact_pack` | object | yes | `{"contact_sheet":true,"cue_clips":true}` | Output toggles with defaults. |

## Wave Plan

1. Wave A: Contract + preflight hardening
- Add profile loader and schema validation.
- Add deterministic manifest + replay hash writer.
- Exit: contract-only replay lane (`--contract-only`) passes T1 (3/3).

2. Wave B: Post-processing packager
- Add checkpoint labeling, contact sheets, cue-window clips, artifact inventory TSV.
- Exit: execute replay lane (`--execute`) passes T1 and produces complete schema rows.

3. Wave C: Lane integration and extension handoff
- Add `scripts/qa-bl077-capture-harness-mac.sh`.
- Integrate at least one active backlog lane as a consumer.
- Publish extension contract for `audio-plugin-coder` and `audio-dsp-qa-harness`.
- Exit: T2 candidate run (5/5) plus owner handoff packet.

4. Wave D: Promotion packet
- T3 replay (10 runs), flake taxonomy, owner decision artifact.
- Exit: promotion-ready status with deterministic evidence set.

## Risk Matrix and Mitigations

| Risk | Level | Impact | Mitigation |
|---|---|---|---|
| macOS capture permission or device drift | High | run failure | preflight device probing + explicit fail-fast messaging + fallback selection hints |
| Artifact bloat from dense capture | Medium | storage/noise | profile-level retention controls and clip/window budgets |
| Two-app timing mismatch during manual motion | Medium | analysis ambiguity | richer cue density + timestamped cue markers + optional spoken prompts |
| Replay non-determinism in packaging | High | promotion blocked | manifest hash contract and deterministic naming/ordering policy |
| Cross-project adoption drift | Medium | duplicated effort | publish schema contract + integration examples in extension section |

## Validation and Evidence Contract

Primary lane script (to author): `scripts/qa-bl077-capture-harness-mac.sh`

Required artifacts per execute run:
- `status.tsv`
- `session_manifest.json`
- `capture_contract_matrix.tsv`
- `artifact_schema_inventory.tsv`
- `replay_hashes.tsv`
- `integration_consumers.tsv`
- `lane_notes.md`

Replay targets:
- T1: 3 runs (`--contract-only`)
- T2: 5 runs (`--execute`)
- T3: 10 runs (`--execute`, owner packet)

## Extension Contract (audio-plugin-coder / audio-dsp-qa-harness)

- Preserve CLI-first orchestration so external harnesses can call BL-077 without plugin-internal imports.
- Keep profile schema fully declarative (no executable logic embedded in profile files).
- Emit machine-readable manifests first; human summaries remain additive.
- Reserve optional `consumer_metadata` block for downstream harness-specific tags.

## Recommended Immediate Backlog Sequencing

1. Promote BL-077 from planning intake to implementation kickoff once contract/schema baseline is accepted.
2. Start Wave A immediately (highest leverage, lowest integration risk).
3. Gate dependent visual-validation-heavy lanes on BL-077 execute readiness where practical.
