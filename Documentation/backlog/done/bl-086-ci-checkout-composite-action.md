Title: BL-086 CI Checkout Composite Action — audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19

# BL-086 CI Checkout Composite Action — audio-dsp-qa-harness

## Plain-Language Summary

BL-086 in plain terms: publish a GitHub Actions composite action in `audio-dsp-qa-harness` that standardizes private-repo checkout and auth token validation so plugin repos stop maintaining subtly different versions of the same YAML block. Current state: Done. LocusQ CI now uses the shared action, the token-missing path is explicitly validated, and the workflow now consumes the action’s `harness-path` output instead of hardcoding the checkout location.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin CI maintainers managing GitHub Actions workflows for LocusQ, echoform, memory-echoes, and monument-reverb. |
| What is changing? | A multi-step checkout + token validation block is replaced with a single `uses: joshband/audio-dsp-qa-harness/.github/actions/checkout-qa-harness@master` call plus the action output is used downstream for `QA_HARNESS_DIR`. |
| Why is this important? | monument-reverb currently falls back to `github.token` (insufficient for private harness); echoform/memory-echoes have inconsistent error messages; any harness URL or ref change requires four-repo updates. A composite action is a single-repo fix. |
| How will we deliver it? | Author `.github/actions/checkout-qa-harness/action.yml` in harness; update LocusQ `qa_harness.yml` to use it; verify CI passes; document migration for other plugins. |
| When is it done? | When the composite action exists in harness, LocusQ CI uses it, and CI passes with identical behavior to the current workflow. |
| Where is the source of truth? | This done runbook, `Documentation/backlog/index.md`, `.github/workflows/qa_harness.yml`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-086 |
| Priority | P1 |
| Status | Done |
| Track | G - Tooling / Governance |
| Effort | Small / S |
| Depends On | — |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Author a composite GitHub Actions action that encapsulates:

1. **Token validation**: verify `SUBMODULE_TOKEN` is non-empty; emit actionable error message if missing (not a cryptic checkout failure).
2. **Harness checkout**: `actions/checkout@v4` with configurable `ref` (default: `master`), `path`, and `token`.
3. **Checkout confirmation**: verify harness directory exists and contains expected marker file (`CMakeLists.txt` or `lib/` directory) before proceeding.

Action inputs:
- `token` (required): the PAT for private repo access
- `ref` (optional, default: `master`): harness branch or tag
- `path` (optional, default: `audio-dsp-qa-harness`): checkout destination

Action outputs:
- `harness-path`: resolved absolute path to checked-out harness (for downstream `cmake -DQA_HARNESS_DIR=` step)

## Acceptance IDs

- `audio-dsp-qa-harness` contains `.github/actions/checkout-qa-harness/action.yml`
- LocusQ `qa_harness.yml` multi-step checkout block is replaced with a single `uses:` call
- LocusQ CI passes with zero regressions after change
- Missing `SUBMODULE_TOKEN` produces an actionable error message (not generic checkout 401)
- Action `README` documents required secrets and migration steps for the other three plugins

## Methodology Reference

- BL-086 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- LocusQ reference block: `.github/workflows/qa_harness.yml` lines 227–243

## Implementation Slices

### S1 — Author `action.yml`
Implement composite action with token validation, checkout, and path output.

### S2 — Update LocusQ `qa_harness.yml`
Replace checkout block with `uses:` call. Run CI; confirm identical behavior.

### S3 — Document migration
Add migration notes in action README and use the LocusQ lane as the reference proof for other plugins.

## Latest Validation Snapshot

- 2026-03-19: upstream composite action exists at `.github/actions/checkout-qa-harness/action.yml`.
- LocusQ `qa_harness.yml` uses the composite action in both checkout sites.
- LocusQ configure steps now consume `steps.checkout-qa-harness.outputs.harness-path` instead of hardcoding the checkout directory.
- Local execute lane confirms the token-missing path exits non-zero with the expected actionable message.
- Current evidence:
  - `TestEvidence/bl086_ci_composite_action_20260319T193051Z/status.tsv`
  - `TestEvidence/bl086_ci_composite_action_20260319T193051Z/summary.md`

## Validation Plan

QA harness script: `scripts/qa-bl086-ci-composite-action-mac.sh` (validates action.yml schema and simulates token-missing error path locally).
Evidence schema: `TestEvidence/bl086_*/status.tsv`.

Gate criterion: LocusQ CI workflow passes; local action schema validation passes; token-missing error path produces expected message.

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

Additional field captured at closeout:
- `UPSTREAM_HARNESS_COMMIT: 9bb2ec8`

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

This runbook is the historical closeout record for the BL-086 CI action migration.
