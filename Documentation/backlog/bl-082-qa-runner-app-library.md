Title: BL-082 QA Runner App Library — Upstream Extraction to audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19

# BL-082 QA Runner App Library — Upstream Extraction to audio-dsp-qa-harness

## Plain-Language Summary

BL-082 in plain terms: Extract the nearly-identical QA runner app logic shared across all four plugins into a shared `lib/qa_runner_app` library in `audio-dsp-qa-harness` so each plugin keeps only a thin adapter entrypoint. Current state: In Validation. LocusQ already has the extracted local shape: `qa/main.cpp` is a thin entrypoint, and `qa/LocusQQARunner.cpp` is `BaseQARunner`-backed. The remaining work is cross-repo adoption and owner closeout, not another large local refactor.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin developers maintaining QA executables across LocusQ, echoform, memory-echoes, and monument-reverb. |
| What is changing? | CLI parsing, suite routing, runtime-config application, and result export are extracted from each plugin's `qa/main.cpp` into a harness-owned `BaseQARunner` template; plugins provide only a DUT adapter factory and policy flags. |
| Why is this important? | ~1,985 LOC of near-identical runner code is maintained separately in four repos, causing routing bugs, profiling inconsistencies, and CLI drift. One extraction reduces total cross-repo custom runner code by ~1,500 LOC. |
| How will we deliver it? | Design `BaseQARunner` or CRTP template in harness; refactor LocusQ `qa/main.cpp` as reference; document adoption pattern; backport to echoform/memory-echoes/monument-reverb. |
| When is it done? | When harness publishes `lib/qa_runner_app`, LocusQ's runner is ≤50 LOC of adapter code, and LocusQ CI lanes pass with identical outputs to pre-extraction baseline. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-082-qa-runner-app-library.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-082 |
| Priority | P0 |
| Status | In Validation |
| Track | G - Tooling / Governance |
| Effort | Med / M |
| Depends On | — |
| Blocks | BL-083 (runtime-config contract depends on runner being stable), BL-084 (profiling contract) |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Extract the four-plugin runner app pattern into `audio-dsp-qa-harness/lib/qa_runner_app/` as a reusable C++ template library. The shared library owns:

- `RunOptions` struct (scenario path, suite path, mode flags, runtime-config overrides, profiling toggle)
- CLI argument parser (identical across all four plugins today)
- Suite routing logic: smoke → single scenario → full suite → discovery
- Runtime-config application (delegates to BL-083 contract after that lands)
- Result export and exit-code policy
- Profiling dispatch (delegates to BL-084 contract after that lands)

Each plugin's `qa/main.cpp` becomes:
```cpp
#include <qa_runner_app/BaseQARunner.h>
int main(int argc, char** argv) {
    return MyPluginQARunner{}.run(argc, argv);
}
```
where `MyPluginQARunner` registers DUT adapters and plugin-specific policy flags only.

## Acceptance IDs

- `audio-dsp-qa-harness` contains `lib/qa_runner_app/` with `BaseQARunner.h`, `RunOptions.h`, `CliParser.h`, `SuiteRouter.h`, `ResultExporter.h`
- LocusQ's `qa/main.cpp` is refactored to ≤60 LOC of adapter registration and policy flags
- All LocusQ QA lane commands (`--smoke`, `--scenario`, `--suite`, `--discover`) produce byte-for-byte identical outputs vs pre-extraction baseline
- LocusQ CI `qa_harness.yml` passes with zero regressions
- `audio-dsp-qa-harness` README documents the adoption pattern with a minimal `main.cpp` template

## Methodology Reference

- BL-082 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- Current LocusQ runner: `qa/main.cpp` (reference implementation for extraction)

## Implementation Slices

### S1 — Design and scaffold `BaseQARunner`
Define `RunOptions`, `CliParser`, `SuiteRouter` in `audio-dsp-qa-harness/lib/qa_runner_app/`. Implement CLI parsing extracted from LocusQ `qa/main.cpp`. No plugin-specific logic.

### S2 — Port LocusQ runner
Refactor LocusQ `qa/main.cpp` to use `BaseQARunner`. Run all existing QA lanes; confirm output parity. Capture baseline hash of key scenario outputs before extraction for regression gate.

### S3 — Result export and exit-code policy
Extract result export (TSV/JSON, exit code mapping) into `ResultExporter`. Confirm identical exit codes on PASS/FAIL scenarios across LocusQ and echoform baselines.

### S4 — Document adoption pattern + backport guide
Add `lib/qa_runner_app/README.md` documenting plugin adoption steps. Include diff-minimal `main.cpp` template. Note any known plugin-specific flags that must remain in-repo.

## Latest Validation Snapshot

- 2026-03-19 local parity lane: `qa/main.cpp` is 13 lines and remains a thin process entrypoint.
- `qa/LocusQQARunner.cpp` subclasses `qa_runner_app::BaseQARunner` and keeps repo-local policy isolated there.
- Local smoke coverage passed across single-scenario, suite, and curated-discover routes.
- Current evidence:
  - `TestEvidence/bl082_runner_app_library_20260319T192803Z/status.tsv`
  - `TestEvidence/bl082_runner_app_library_20260319T192803Z/summary.md`

## Validation Plan

QA harness script: `scripts/qa-bl082-runner-app-library-mac.sh` (to be authored in S2).
Evidence schema: `TestEvidence/bl082_*/status.tsv`.

Gate criterion: all LocusQ QA mode commands produce identical outputs to pre-extraction baseline (hash-pinned scenario results).

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

Additional field required at handoff: `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit introducing `lib/qa_runner_app/`.

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook lists only item-specific exceptions or additions.
