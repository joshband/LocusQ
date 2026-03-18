Title: BL-095 Partitioned FIR truthfulness recovery and objective validation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-095 Partitioned FIR truthfulness recovery and objective validation

## Plain-Language Summary

BL-095 in plain terms: restore truth between what LocusQ says its long-FIR calibration engine is doing and what the DSP actually does. Current state: Open. This item was created from the 2026-03-17 comprehensive code/DSP review after the repo’s BL-055 surfaces were found to over-claim partitioned-runtime readiness.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | DSP maintainers, QA/release owners, and host-facing validation owners who need trustworthy latency and engine-state reporting. |
| What is changing? | FIR engine naming, latency publication, promotion evidence, and runtime behavior are brought back into one truthful contract. |
| Why is this important? | The current code can report a partitioned engine and nonzero latency while still running a direct-form loop, and the archived BL-055 execute lane can pass on markers rather than behavior. |
| How will we deliver it? | First correct the governance/evidence contract, then either remove false partitioned claims or land a real partitioned path, and finally prove the result with objective latency/impulse/CPU evidence. |
| When is it done? | This item is done when requested/active engine truth, host latency reporting, and objective validation all agree for short and long FIR paths. |
| Where is the source of truth? | This runbook, the 2026-03-17 review report, BL-055 historical surfaces, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Acceptance + slice tables | Makes the correction path specific and implementation-ready. | `## Acceptance IDs`, `## Implementation Slices` |
| Objective evidence tables | Prevents a repeat of marker-only truth claims. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-095 |
| Priority | P0 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | High / L |
| Depends On | BL-050 (Done), BL-055 (Done), BL-073 (Done) |
| Blocks | — |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Restore a truthful FIR engine contract for calibration mode. BL-095 is complete only when the runtime either stays honestly direct-form with matching zero-latency reporting, or ships a real partitioned path whose measured impulse offset, host latency publication, engine state, and CPU behavior all agree.

## Source Inputs

- `Documentation/reviews/2026-03-17-comprehensive-code-dsp-review.md`
- `Documentation/backlog/done/bl-055-fir-convolution-engine.md`
- `scripts/qa-bl055-fir-convolution-engine-mac.sh`
- `Documentation/plans/bl-050-partitioned-fir-migration-contract-2026-03-01.md`
- `Source/headphone_dsp/HeadphoneFirHook.h`
- `Source/headphone_dsp/HeadphoneCalibrationChain.h`
- `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`
- `Source/PluginProcessor.cpp`

## Acceptance IDs

- `BL095-A1` Engine identifiers and diagnostics reflect the actual DSP path running for every tap-count range.
- `BL095-A2` Reported latency matches measured impulse offset and host-facing PDC behavior.
- `BL095-A3` BL-055-style execute validation uses runtime evidence, not marker-grep heuristics, for partitioned-latency and crossfade claims.
- `BL095-A4` If only direct FIR remains active, all backlog/index/runtime surfaces stop claiming partitioned behavior exists.
- `BL095-A5` If a real partitioned path lands, its swap/crossfade behavior is validated against previous-engine to next-engine output rather than wet-vs-dry placeholder logic.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Correct historical overclaim and replace marker-only validation with an objective contract. | `Documentation/backlog/done/bl-055-fir-convolution-engine.md`, `Documentation/backlog/index.md`, `scripts/qa-bl055-fir-convolution-engine-mac.sh`, new BL-095 evidence surfaces | BL-055 surfaces no longer overstate implemented behavior; objective lane entrypoint is defined |
| B | Correct runtime truth: direct-only honesty now, or real partitioned implementation. | `Source/headphone_dsp/HeadphoneFirHook.h`, `Source/headphone_dsp/HeadphoneCalibrationChain.h`, `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`, `Source/PluginProcessor.cpp` | engine reporting, latency publication, and runtime path are internally consistent |
| C | Capture latency/parity/CPU evidence and revalidate host-facing behavior. | `TestEvidence/bl095_*`, targeted FIR parity tooling and host-facing checks | short/long FIR evidence is green and promotion claims are behavior-backed |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL095-DOCS | Automated | `./scripts/validate-backlog-plain-language.sh` + `./scripts/validate-backlog-redundancy.py` + `./scripts/validate-docs-freshness.sh` | exit 0 |
| BL095-CONTRACT | Automated | `scripts/qa-bl095-fir-truthfulness-mac.sh --contract-only` | latency/engine/crossfade checks describe measured or directly provable runtime behavior |
| BL095-EXECUTE | Automated | `scripts/qa-bl095-fir-truthfulness-mac.sh --execute --runs 3` | impulse offset, reported latency, and CPU profile agree across tap thresholds |
| BL095-HOST | Focused validation | representative standalone/host PDC check after Slice B | no false nonzero PDC in direct-only mode, or correct PDC in partitioned mode |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | contract + execute FIR truth lane | objective latency/CPU artifacts + replay notes |
| Candidate intake | T2 | 5 | same lane at candidate depth | replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected truthfulness replay set | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-095 is a corrective follow-on to the 2026-03-17 review. BL-055 remains archived as a historical closeout surface, but it must no longer be treated as sufficient proof of a partitioned FIR runtime until BL-095 closes with objective evidence.
