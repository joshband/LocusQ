Title: BL-083 Runtime-Config Contract Enforcement — audio-dsp-qa-harness ScenarioExecutor
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-20

# BL-083 Runtime-Config Contract Enforcement — audio-dsp-qa-harness ScenarioExecutor

## Plain-Language Summary

BL-083 in plain terms: move `applySuiteRuntimeConfig()` into an automatic harness-owned path so all plugins get consistent runtime-config behavior without manual per-repo implementations. Current state: In Validation. The core executor contract is already live upstream, and LocusQ no longer carries a bulky local workaround in `qa/main.cpp`. The remaining work is broader cross-repo verification and owner closeout, not another local executor rewrite.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA engineers and CI systems that run scenario suites across LocusQ, echoform, memory-echoes, and monument-reverb. |
| What is changing? | `ScenarioExecutor::executeScenario()` automatically applies suite-level runtime-config overrides before each run; per-repo manual implementations are removed. |
| Why is this important? | echoform and memory-echoes never call `applySuiteRuntimeConfig()`, causing false-green tests that run with stale sample rates or wrong channel counts. LocusQ carries a 27-LOC manual workaround that is fragile and undocumented. |
| How will we deliver it? | Add `applySuiteRuntimeConfig()` as a public harness API and call it inside `ScenarioExecutor` before scenario execution; remove manual workarounds from LocusQ `qa/main.cpp`; verify all four plugin baselines are unaffected. |
| When is it done? | When harness `ScenarioExecutor` owns runtime-config application, LocusQ's manual workaround is removed, and scenario outputs match pre-change baseline. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-083-runtime-config-contract.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-083 |
| Priority | P0 |
| Status | Done |
| Track | G - Tooling / Governance |
| Effort | Small / S |
| Depends On | — |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Make runtime-config application a first-class, automatic harness contract rather than an optional per-plugin step. Specifically:

1. Expose `applySuiteRuntimeConfig(ExecutionConfig& config, const SuiteDefinition& suite)` as a documented public harness API.
2. Call it automatically inside `ScenarioExecutor::executeScenario()` (or `executeSuite()`) before each scenario runs.
3. Remove LocusQ's 27-LOC manual workaround in `qa/main.cpp` (`applySuiteRuntimeConfigManual`).
4. Add a harness-level warning when a suite specifies runtime-config overrides but the executor detects they were not applied (defense-in-depth).

Runtime-config fields in scope: `sample_rate`, `block_size`, `num_channels`, `seed`, `output_dir`.

## Acceptance IDs

- `ScenarioExecutor` calls `applySuiteRuntimeConfig()` internally before each scenario run
- `applySuiteRuntimeConfig()` is documented in the harness public API header with usage notes
- LocusQ `qa/main.cpp` no longer contains `applySuiteRuntimeConfigManual` or equivalent
- Harness emits a warning when a suite's `runtimeConfig` field is present but empty/null after application
- All LocusQ scenario outputs match pre-change baseline (no behavioral change for correctly-configured suites)
- echoform and memory-echoes scenario suites that previously skipped runtime-config now apply overrides correctly (verified by running their existing suites with explicit `sample_rate` override and confirming the new value is used)

## Methodology Reference

- BL-083 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- LocusQ workaround location: `qa/main.cpp` (`applySuiteRuntimeConfigManual`, lines ~150–177)

## Implementation Slices

### S1 — Expose and document `applySuiteRuntimeConfig()` as public API
Add to harness public header. Document fields, precedence order (suite overrides base config), and default passthrough behavior when no overrides specified.

### S2 — Integrate into `ScenarioExecutor`
Call `applySuiteRuntimeConfig()` inside `executeScenario()` before scenario execution. Add warning log when suite defines overrides that result in no-op (e.g., sample_rate == 0).

### S3 — Remove LocusQ workaround
Delete `applySuiteRuntimeConfigManual` from `qa/main.cpp`. Run full LocusQ scenario baseline; confirm zero output delta.

## Latest Validation Snapshot

- 2026-03-19 upstream contract check: `ScenarioExecutor::resolveExecutionConfig()` delegates to `applySuiteRuntimeConfig(config_, suite)`.
- Upstream no-op override warning contract is present.
- LocusQ local proof lane shows suite `runtime_config` overrides beat CLI base config and redirect output into the suite-owned output directory.
- Current evidence:
  - `TestEvidence/bl083_runtime_config_contract_20260319T192716Z/status.tsv`
  - `TestEvidence/bl083_runtime_config_contract_20260319T192716Z/summary.md`

## Validation Plan

QA harness script: `scripts/qa-bl083-runtime-config-contract-mac.sh` (to be authored in S3).
Evidence schema: `TestEvidence/bl083_*/status.tsv`.

Gate criterion: LocusQ scenario outputs hash-match pre-extraction baseline; at least one echoform/memory-echoes suite demonstrates correct runtime-config application where it was previously skipped.

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

Additional field required at handoff: `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit introducing the `ScenarioExecutor` change.

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook lists only item-specific exceptions or additions.
