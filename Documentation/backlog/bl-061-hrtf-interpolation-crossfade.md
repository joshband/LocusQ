Title: BL-061 HRTF Interpolation + Crossfade (Phase C, conditional)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-02

# BL-061 HRTF Interpolation + Crossfade (Phase C, conditional)

## Plain-Language Summary

BL-061 in plain terms: Replace nearest-neighbor HRIR selection with libmysofa continuous azimuth/elevation interpolation. Current state: Open (conditional on BL-060 gate pass). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-061 HRTF Interpolation + Crossfade (Phase C, conditional) |
| Why is this important? | Replace nearest-neighbor HRIR selection with libmysofa continuous azimuth/elevation interpolation. |
| How will we deliver it? | Use the validation plan and evidence bundle contract in this runbook to prove behavior and safety before promotion. |
| When is it done? | This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass. |
| Where is the source of truth? | Runbook: `Documentation/backlog/bl-061-hrtf-interpolation-crossfade.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-061 |
| Priority | P2 |
| Status | Open (conditional on BL-060 gate pass) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-060 gate pass |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Replace nearest-neighbor HRIR selection with `libmysofa` continuous azimuth/elevation interpolation. Add crossfaded filter updates (dual-convolver or equivalent) to eliminate zipper artifacts when source direction changes during head movement.

## Acceptance IDs

- interpolated HRTF changes produce no audible zipper
- crossfade duration ≤ 10ms
- no RT allocation during direction update
- libmysofa version pinned in CMakeLists.txt
- no RT locks/blocking I/O during interpolation or crossfade updates
- deterministic parity check against nearest-neighbor baseline is captured
- promotion is blocked unless BL-060 gate indicates measurable benefit

## Methodology Reference

- Canonical methodology: `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`.
- Reconciliation review: `Documentation/reviews/2026-03-01-headtracking-research-backlog-reconciliation.md`.


## Validation Plan

QA harness script: `scripts/qa-bl061-hrtf-interpolation-crossfade-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl061_*/status.tsv`.

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | runbook primary lane command at dev-loop depth | validation matrix + replay summary |
| Candidate intake | T2 | 5 (or heavy-wrapper 2-run cap) | runbook candidate replay command set | contract/execute artifacts + taxonomy |
| Promotion | T3 | 10 (or owner-approved heavy-wrapper 3-run equivalent) | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run sentinel drill when explicitly requested | parity/sentinel artifacts |

### Cost/Flake Policy

- Diagnose failing run index before repeating full multi-run sweeps.
- Heavy wrappers (`>=20` binary launches per wrapper run) use targeted reruns, candidate at 2 runs, and promotion at 3 runs unless owner requests broader coverage.
- Document cadence overrides with rationale in `lane_notes.md` or `owner_decisions.md`.


## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.

