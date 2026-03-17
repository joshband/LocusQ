Title: BL-081 Perceptual Listening Harness — Upstream Extraction to audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-081 Perceptual Listening Harness — Upstream Extraction to audio-dsp-qa-harness

## Plain-Language Summary

BL-081 in plain terms: Extract the BL-060 perceptual listening harness (`bl060-analyze-results.py` + trial schema) into `audio-dsp-qa-harness` as a shared `tools/perceptual/` Python package, so that echoform, memory-echoes, monument-reverb, and any future plugins can run Phase B–style perceptual listening studies with a common evidence schema and analysis toolchain. Current state: Open (depends on BL-060 In Validation). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin developers and QA owners across LocusQ, echoform, memory-echoes, and monument-reverb who need perceptual validation evidence. |
| What is changing? | `bl060-analyze-results.py` and the BL-060 trial schema are promoted to a shared upstream package at `tools/perceptual/` in `audio-dsp-qa-harness`; each consuming repo gets a thin harness script that invokes it. |
| Why is this important? | Without extraction, each plugin must copy-paste the analysis script, causing schema drift and duplicated maintenance. The shared package ensures consistent gate metrics, artifact contracts, and backport parity. |
| How will we deliver it? | Author `tools/perceptual/` in `audio-dsp-qa-harness`; update LocusQ to consume it (replacing local copy); verify BL-060 evidence is reproducible through the upstream path; document backport instructions for echoform/memory-echoes/monument-reverb. |
| When is it done? | When `audio-dsp-qa-harness` publishes `tools/perceptual/`, LocusQ's harness script delegates to it, and the BL-060 gate evidence is reproduced end-to-end via the upstream path. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-081-perceptual-listening-harness-upstream-extraction.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

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
| ID | BL-081 |
| Priority | P2 |
| Status | Open |
| Track | G - Tooling / Governance |
| Effort | Small / S |
| Depends On | BL-060 (In Validation; origin implementation) |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Extract the BL-060 perceptual listening harness into a shared upstream package in `audio-dsp-qa-harness` so all four plugins can invoke it without copy-paste. The shared package owns:

- `tools/perceptual/analyze_results.py` — the canonical analysis entrypoint (renamed from `bl060-analyze-results.py`)
- `tools/perceptual/schema.py` — required CSV column definitions and gate constants
- `tools/perceptual/stats.py` — Welch t-test, MAE, FB confusion, externalization metrics
- `tools/perceptual/report.py` — artifact writers (`metrics_summary.tsv`, `stats_report.md`, `gate_decision.md`, `reproducibility_check.tsv`)

Each plugin's harness script becomes a thin wrapper that:
1. Locates the upstream `tools/perceptual/analyze_results.py`
2. Passes its own `trial_log.csv` and `--out-dir`
3. Captures evidence under its own `TestEvidence/` layout

LocusQ's `scripts/bl060-analyze-results.py` is retained as a shim (delegates to upstream) for backward compatibility with existing CI lanes.

## Acceptance IDs

- `audio-dsp-qa-harness` contains `tools/perceptual/` with at minimum `analyze_results.py`, `schema.py`, `stats.py`, `report.py`
- `tools/perceptual/analyze_results.py` accepts `--trial-log` and `--out-dir` with identical CLI surface to the current BL-060 script
- LocusQ's BL-060 gate evidence (`gate_decision.md`, `metrics_summary.tsv`, `stats_report.md`, `reproducibility_check.tsv`) is reproduced byte-for-byte using the upstream script against the existing BL-060 fixture `trial_log.csv`
- LocusQ's `scripts/bl060-analyze-results.py` is updated to a thin shim (or removed with a deprecation note pointing to upstream)
- `audio-dsp-qa-harness` README documents the `tools/perceptual/` usage pattern with a minimal backport example for echoform/memory-echoes/monument-reverb
- No new mandatory external dependencies added (pure Python stdlib remains sufficient)
- All four plugins' CI lanes continue to pass after extraction

## Methodology Reference

- BL-060 origin implementation: `scripts/bl060-analyze-results.py`, `scripts/qa-bl060-phase-b-listening-test-mac.sh`
- BL-060 reference evidence: `TestEvidence/bl060_phase_b_listening_20260317T174025Z_90778/`
- Cross-plugin QA audit: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`

## Implementation Slices

### S1 — Upstream package scaffold
Author `tools/perceptual/` in `audio-dsp-qa-harness` with the four modules listed in Objective. Preserve exact CLI surface and gate constants from `bl060-analyze-results.py`. Add `tools/perceptual/__init__.py` and a minimal `README.md` section.

### S2 — LocusQ shim + parity test
Update `scripts/bl060-analyze-results.py` to delegate to the upstream package path (read from `QA_HARNESS_DIR` env var or `CMakeLists.txt` submodule path). Run `scripts/qa-bl060-phase-b-listening-test-mac.sh --execute` and confirm gate hash matches the T1 reference (`1849befd4fda3f44`).

### S3 — Backport documentation
Add a `tools/perceptual/backport.md` in `audio-dsp-qa-harness` documenting the three-step pattern for adopting repos: (1) locate upstream script, (2) provide `trial_log.csv` in schema, (3) run analysis and capture evidence. Include a concrete example using echoform's repo layout.

## Validation Plan

QA harness script: `scripts/qa-bl081-perceptual-harness-upstream-mac.sh` (to be authored in S2).
Evidence schema: `TestEvidence/bl081_*/status.tsv`.

Required analysis artifacts (reproduced via upstream path):
- `trial_log.csv` (BL-060 reference fixture, seed=42)
- `metrics_summary.tsv`
- `stats_report.md`
- `gate_decision.md` (gate_hash_prefix must match `1849befd4fda3f44`)
- `reproducibility_check.tsv`

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

Additional field required at handoff: `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit that introduced `tools/perceptual/`.

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.
