Title: BL-087 Recursive Scenario Discovery — audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-087 Recursive Scenario Discovery — audio-dsp-qa-harness

## Plain-Language Summary

BL-087 in plain terms: Add an optional `recursive=true` flag to `discoverSuite()` in `audio-dsp-qa-harness` so plugins with nested scenario directories (currently echoform, potentially others as suites grow) can use the harness-native discovery path instead of custom recursive wrappers. Current state: Open (deferred P2 — unblock when echoform or a second plugin requests it). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin QA maintainers who organize scenarios in subdirectories and need deterministic recursive discovery. |
| What is changing? | `discoverSuite(path, recursive=true)` performs depth-first, alphabetically-ordered traversal of scenario directories; plugins no longer need custom recursive wrappers. |
| Why is this important? | echoform currently carries a custom recursive wrapper that is not covered by harness tests and may produce non-deterministic ordering on some filesystems. A harness-native implementation is tested and deterministic. |
| How will we deliver it? | Add optional `recursive` parameter to `discoverSuite()` with deterministic traversal; add `_draft/` and `_disabled/` directory skip conventions; verify echoform baseline. |
| When is it done? | When harness `discoverSuite()` supports `recursive=true` with deterministic ordering and echoform's custom wrapper is removed. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-087-recursive-scenario-discovery.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-087 |
| Priority | P2 |
| Status | Open (deferred; start when echoform or second plugin confirms need) |
| Track | G - Tooling / Governance |
| Effort | Small / S |
| Depends On | — |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Extend `discoverSuite(const std::string& path)` to accept an optional `bool recursive = false` parameter. When `recursive=true`:

1. Traverse subdirectories depth-first in alphabetical order.
2. Collect all `.json` files matching the scenario schema.
3. Skip directories named `_draft/`, `_disabled/`, or prefixed with `.` (hidden dirs).
4. Return a `SuiteDefinition` with scenarios ordered deterministically (same order on any filesystem).

Determinism guarantee: discovery order must be reproducible across macOS, Linux, and Windows (NTFS/HFS+ sorting differences handled by explicit `std::sort` on collected paths).

## Acceptance IDs

- `discoverSuite(path, recursive=true)` is documented in the harness public API
- Discovery order is deterministic: running twice on the same directory produces identical `SuiteDefinition` scenario ordering
- `_draft/` and `_disabled/` directories are skipped
- echoform's custom recursive wrapper is removed; echoform scenario suite produces identical results using harness-native discovery
- A unit test covers the recursive traversal with at least 2 nesting levels

## Methodology Reference

- BL-087 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- echoform reference: custom recursive wrapper in echoform `qa/adapter.cpp`

## Implementation Slices

### S1 — Extend `discoverSuite()` API
Add `recursive` parameter; implement depth-first alphabetical traversal with skip conventions. Add unit test.

### S2 — Remove echoform custom wrapper
Update echoform `qa/adapter.cpp` to use harness-native `discoverSuite(path, /*recursive=*/true)`. Run echoform QA; confirm identical suite results.

## Validation Plan

QA harness script: `scripts/qa-bl087-recursive-discovery-mac.sh` (to be authored in S2).
Evidence schema: `TestEvidence/bl087_*/status.tsv`.

Gate criterion: echoform suite discovery produces identical scenario list and results via harness-native recursive path vs previous custom wrapper.

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

Additional field required at handoff: `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit extending `discoverSuite()`.

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook lists only item-specific exceptions or additions.
