Title: BL-084 Profiling Contract Hardening — audio-dsp-qa-harness ScenarioExecutor
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-20

# BL-084 Profiling Contract Hardening — audio-dsp-qa-harness ScenarioExecutor

## Plain-Language Summary

BL-084 in plain terms: Make `ScenarioExecutor` enforce that performance invariants are only evaluated when profiling is enabled; emit an explicit error or warning when a scenario contains `perf_*` invariants but `ExecutionConfig.enableProfiling` is false so that false-green performance tests are eliminated. Current state: Done. The shared harness now owns profiling attachment in `qa_runner_app::BaseQARunner`, and LocusQ no longer carries a local profiling-dispatch workaround in its QA runner. Broader cross-repo audit can continue as follow-on adoption work, not as an open LocusQ blocker.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA engineers running CPU performance and throughput regression tests across LocusQ, echoform, memory-echoes, and monument-reverb. |
| What is changing? | `ScenarioExecutor` checks for `perf_*` invariants and raises an error or configurable warning when `enableProfiling=false`; LocusQ's workaround code in `qa/main.cpp` is removed. |
| Why is this important? | monument-reverb and echoform silently skip profiling, allowing perf invariant checks to pass on unmeasured data. LocusQ carries a custom `profileDspPerformance()` dispatch in `main.cpp` to paper over the gap. Without this fix, performance regressions can pass undetected. |
| How will we deliver it? | Harden `ScenarioExecutor` to detect `perf_*` invariants and enforce profiling precondition; provide a policy enum (`WARN`, `ERROR`, `SKIP`) so plugins can adopt incrementally; remove LocusQ workaround. |
| When is it done? | Done. `ScenarioExecutor` enforces the profiling precondition, LocusQ's workaround code is removed, and the local profiling lane is green. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-084-profiling-contract-hardening.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-084 |
| Priority | P0 |
| Status | Done |
| Track | G - Tooling / Governance |
| Effort | Med / M |
| Depends On | — |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Eliminate silent false-green performance tests by enforcing a profiling precondition contract inside `ScenarioExecutor`. The contract:

1. Before evaluating any invariant whose key begins with `perf_`, check `ExecutionConfig.enableProfiling`.
2. If `enableProfiling=false` and `perf_*` invariants are present, apply the plugin-configured `ProfilingPolicy`:
   - `WARN` (default): log a warning and skip perf invariant evaluation (counts as SKIP, not PASS).
   - `ERROR`: fail the scenario immediately with a descriptive message.
   - `IGNORE`: legacy passthrough (existing behavior — explicitly opt-in to suppress warnings).
3. Remove LocusQ's custom `profileDspPerformance()` dispatch from `qa/main.cpp`; replace with harness-native `enableProfiling=true` in suite config.
4. Default `ProfilingPolicy` is `WARN` to allow incremental adoption without breaking existing suites.

## Acceptance IDs

- `ScenarioExecutor` checks `enableProfiling` before evaluating any `perf_*` invariant
- A scenario with `perf_cpu_mean_ms` invariant and `enableProfiling=false` produces a SKIP result (WARN policy) or FAIL (ERROR policy), not a PASS
- `ProfilingPolicy` is documented in the harness public API header with valid values and migration guidance
- LocusQ `qa/main.cpp` no longer contains custom `profileDspPerformance()` dispatch
- All LocusQ perf scenarios produce identical PASS results using `enableProfiling=true` in suite config
- echoform/monument-reverb suites with `perf_*` invariants emit WARN (not silent PASS) when profiling is disabled

## Methodology Reference

- BL-084 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- LocusQ workaround location: `qa/main.cpp` (profiling dispatch, lines ~84–90 `RunOptions.enableProfiling`)

## Implementation Slices

### S1 — Add `ProfilingPolicy` enum and wire into `ScenarioExecutor`
Define `ProfilingPolicy { WARN, ERROR, IGNORE }` in harness. Check `perf_*` invariant keys before evaluation; apply policy. Default: `WARN`.

### S2 — Expose policy in `ExecutionConfig`
Add `ProfilingPolicy profilingPolicy = ProfilingPolicy::WARN` to `ExecutionConfig`. Document in public API header.

### S3 — Remove LocusQ workaround; update suite configs
Delete `profileDspPerformance()` dispatch from `qa/main.cpp`. Update affected LocusQ suite JSON files to set `enableProfiling: true`. Run full LocusQ perf scenario baseline; confirm PASS.

### S4 — Audit echoform/monument-reverb suites
Identify suites with `perf_*` invariants and `enableProfiling` unset. Confirm they now emit WARN rather than silent PASS. Document findings in evidence.

## Latest Validation Snapshot

- 2026-03-19 shared-runner slice: `qa_runner_app::BaseQARunner` now auto-attaches profiling metrics when profiling is enabled.
- LocusQ local workaround removed from `qa/LocusQQARunner.cpp`.
- Harness precondition policy still enforces WARN/ERROR behavior for perf invariants without profiling.
- Current evidence:
  - `TestEvidence/bl084_profiling_contract_20260319T192550Z/status.tsv`
  - `TestEvidence/bl084_profiling_contract_20260319T192550Z/summary.md`

## Validation Plan

QA harness script: `scripts/qa-bl084-profiling-contract-mac.sh` (to be authored in S3).
Evidence schema: `TestEvidence/bl084_*/status.tsv`.

Gate criteria:
- LocusQ perf scenarios PASS with `enableProfiling=true`
- A synthetic scenario with `perf_cpu_mean_ms` + `enableProfiling=false` produces SKIP (WARN) not PASS
- echoform/monument-reverb audit result documented

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

Additional field required at handoff: `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit introducing `ProfilingPolicy`.

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook lists only item-specific exceptions or additions.
