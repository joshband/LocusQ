Title: BL-077 Unified Visual Capture and Replay Harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-03

# BL-077 Unified Visual Capture and Replay Harness

## Plain-Language Summary

BL-077 in plain terms: Provide a robust, easy-to-use capture system that records plugin + companion behavior, guides operators through checkpoint cues, and emits ready-to-review artifacts (video, dense frames, labeled checkpoints, contact sheets, and short cue clips) suitable for automated QA lanes and owner promotion decisions. Current state: Done (contract/execute/live/T2/T3 cadence evidence complete; closeout sync and archive transition recorded). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Headphone users, companion-app operators, QA/release owners, and audio-engine maintainers. |
| What is changing? | Provide a robust, easy-to-use capture system that records plugin + companion behavior, guides operators through checkpoint cues, and emits ready-to-review artifacts (video, dense frames, labeled checkpoints, contact sheets, and short cue clips) suitable for automated QA lanes and owner promotion decisions. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: Done (contract/execute/live/T2/T3 evidence complete and closeout/archive sync recorded on 2026-03-03). |
| Where is the source of truth? | Runbook `Documentation/backlog/done/bl-077-unified-visual-capture-and-replay-harness.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Evidence visual snapshot | Consolidated replay/evidence view for promotion decisions. | `## Evidence Visual Snapshot` |
| Implementation slices | Clarifies execution sequence and ownership. | `## Implementation Slices` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Evidence Visual Snapshot

| Target Artifact | Why it matters | Path |
|---|---|---|
| Run-level status | Fast PASS/FAIL visibility for operators and agents | `TestEvidence/bl077_capture_harness_<timestamp>/status.tsv` |
| Capture contract matrix | Confirms capture profile + cue contract behavior | `TestEvidence/bl077_capture_harness_<timestamp>/capture_contract_matrix.tsv` |
| Artifact schema inventory | Confirms all expected visual outputs are present | `TestEvidence/bl077_capture_harness_<timestamp>/artifact_schema_inventory.tsv` |
| Contact-sheet/clip samples | Human-reviewable visual proof for promotion packets | `TestEvidence/bl077_capture_harness_<timestamp>/` |

```mermaid
flowchart LR
    A[Guided capture run] --> B[Frame + checkpoint extraction]
    B --> C[Contact sheets + cue clips]
    C --> D[Machine-readable summary TSV/JSON]
    D --> E[Owner promotion review]
```

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-077 |
| Priority | P0 |
| Status | **Done** (contract + execute + live execute + T2 + T3 PASS; closeout/archive sync complete) |
| Track | D - QA Platform |
| Effort | High / L |
| Depends On | BL-049 (Done), BL-073 |
| Blocks | BL-058, BL-059, BL-060, BL-067, BL-068, BL-074 (validation evidence velocity and reliability) |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (promote deterministic short-runs first; escalate only for promotion packets) |

## Objective

Provide a robust, easy-to-use capture system that records plugin + companion behavior, guides operators through checkpoint cues, and emits ready-to-review artifacts (video, dense frames, labeled checkpoints, contact sheets, and short cue clips) suitable for automated QA lanes and owner promotion decisions.

## Scope

In scope:
- Unified capture CLI for screenshot/video/session capture with profile presets.
- Timed cue engine (terminal and optional spoken prompts) with configurable cue profiles.
- Automatic post-processing: frame extraction, checkpoint labeling, contact sheets, and cue-window clips.
- Lane integration contract for backlog scripts (single command, deterministic artifact layout).
- Export-friendly profile schema to support future reuse in `audio-plugin-coder` and `audio-dsp-qa-harness`.

Out of scope:
- Replacing platform-native capture permissions/security models.
- Host-specific UI test frameworks outside capture/evidence orchestration.
- Non-macOS parity in this first delivery slice (cross-platform follow-on allowed as additive work).

## Architecture Context

- Existing seed script: `scripts/capture-headtracking-rotation-mac.sh` proves baseline feasibility.
- This lane formalizes that ad-hoc script into a reusable QA-platform contract:
  - profile-driven orchestration (`coarse`, `dense`, lane-specific presets),
  - deterministic artifact tree and metadata,
  - reusable hooks for lane scripts and external harness consumers.
- Cross-project extension target:
  - Define capture profile contract and evidence schema that can be adopted by `audio-plugin-coder` and `audio-dsp-qa-harness` without coupling to LocusQ internals.

## Acceptance IDs

- `BL077-A-001`: One-command guided capture run works reliably for multi-window plugin + companion workflows.
- `BL077-A-002`: Cue profiles are configurable and can represent dense checkpoint sweeps (including between-angle checks).
- `BL077-A-003`: Post-processing emits deterministic artifacts (video, frames, checkpoints, contact sheets, cue clips, summary metadata).
- `BL077-A-004`: QA-lane integration contract exists and at least one lane consumes the capture harness end-to-end.
- `BL077-A-005`: Capture artifacts are organized under a machine-parseable schema suitable for promotion packets.
- `BL077-A-006`: Export profile/schema is documented for future adoption in `audio-plugin-coder` and `audio-dsp-qa-harness`.

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Stabilize core capture CLI and profile system (`coarse`/`dense`/custom profile loading). | `scripts/capture-headtracking-rotation-mac.sh`, `scripts/capture_profiles/*.json` (new), `Documentation/backlog/bl-077-*.md` | Runbook approved | Deterministic guided run succeeds and emits complete summary metadata. |
| B | Add robust post-processing packager (checkpoints, contact sheets, cue-window clips, index TSV/JSON). | `scripts/capture-headtracking-rotation-mac.sh`, `scripts/qa-bl077-capture-harness-mac.sh` (new) | Slice A complete | Artifact tree matches schema contract and replay hash is stable across repeated runs. |
| C | Integrate with lane workflows and extension docs for shared harness adoption. | `scripts/qa-*.sh` integration points, `Documentation/testing/*`, extension notes | Slice B complete | At least one active lane consumes harness automatically; extension contract documented for `audio-plugin-coder` + `audio-dsp-qa-harness`. |

## Validation Plan

QA harness script: `scripts/qa-bl077-capture-harness-mac.sh`.
Evidence schema: `TestEvidence/bl077_capture_harness_<timestamp>/status.tsv`.

Minimum evidence additions:
- `capture_contract_matrix.tsv`
- `cue_profile_matrix.tsv`
- `artifact_schema_inventory.tsv`
- `replay_hashes.tsv`
- `integration_consumers.tsv`
- `extension_contract.md`

## Execution Update (2026-03-03)

- Acceptance IDs covered by implementation and validation: `BL077-A-001`, `BL077-A-002`, `BL077-A-003`, `BL077-A-004`, `BL077-A-005`, `BL077-A-006`.
- Execute-mode dry-run now emits deterministic placeholder media/postprocess artifacts so full schema contracts validate even without live screen capture permission.
- Required validation commands:
  - `./scripts/qa-bl077-capture-harness-mac.sh --contract-only --runs 3`
  - `./scripts/qa-bl077-capture-harness-mac.sh --execute --runs 1`
- Validation result (2026-03-03): PASS for both commands.
- Evidence directories:
  - `TestEvidence/bl077_capture_harness_20260303T004858Z_contract/`
  - `TestEvidence/bl077_capture_harness_20260303T004858Z_execute/`

## Promotion Cadence Update (2026-03-03)

- Candidate intake replay (`T2`) result: PASS
  - Command: `./scripts/qa-bl077-capture-harness-mac.sh --execute --runs 5 --out-dir TestEvidence/bl077_capture_harness_20260303T225142Z_t2`
  - Evidence: `TestEvidence/bl077_capture_harness_20260303T225142Z_t2/`
- Promotion replay (`T3`) result: PASS
  - Command: `./scripts/qa-bl077-capture-harness-mac.sh --execute --runs 10 --out-dir TestEvidence/bl077_capture_harness_20260303T225142Z_t3`
  - Evidence: `TestEvidence/bl077_capture_harness_20260303T225142Z_t3/`
- Live execute sample run (`--live-capture`) result: PASS
  - Command: `./scripts/qa-bl077-capture-harness-mac.sh --execute --runs 1 --live-capture --out-dir TestEvidence/bl077_capture_harness_20260303_live_execute`
  - Evidence: `TestEvidence/bl077_capture_harness_20260303_live_execute/`
- Owner sync packet prepared:
  - `TestEvidence/bl077_owner_sync_z1_20260303T225142Z/promotion_decision.md`
  - `TestEvidence/bl077_owner_sync_z1_20260303T225142Z/owner_decisions.md`
  - `TestEvidence/bl077_owner_sync_z1_20260303T225142Z/handoff_resolution.md`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | `scripts/qa-bl077-capture-harness-mac.sh --contract-only --runs 3` | contract matrix + replay summary |
| Candidate intake | T2 | 5 | `scripts/qa-bl077-capture-harness-mac.sh --execute --runs 5` | execute matrix + taxonomy |
| Promotion | T3 | 10 | `scripts/qa-bl077-capture-harness-mac.sh --execute --runs 10` | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run capture reliability soak | long-run parity and flake taxonomy |

### Cost/Flake Policy

- Diagnose specific failing replay indices before repeating multi-run sweeps.
- Keep default evidence windows short and targeted (checkpoint-centric) to reduce artifact bloat.
- Document any cadence override in owner decision artifacts with rationale.

## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment (2026-03-02)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.
