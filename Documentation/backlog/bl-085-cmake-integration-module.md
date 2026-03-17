Title: BL-085 CMake Integration Module — audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-085 CMake Integration Module — audio-dsp-qa-harness

## Plain-Language Summary

BL-085 in plain terms: Add a harness-provided `qa_harness_integration.cmake` module that exposes an `enable_qa_harness()` CMake function so plugins have a single, tested integration path instead of each maintaining subtly different submodule/find_package/target-fallback glue. Current state: Open. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin maintainers configuring CMake builds for LocusQ, echoform, memory-echoes, and monument-reverb. |
| What is changing? | Each plugin's CMakeLists.txt QA harness integration block is replaced with a single `include(qa_harness_integration)` + `enable_qa_harness(TARGET my_qa_exe ADAPTER my_adapter.cpp)` call. |
| Why is this important? | The four plugins carry different integration strategies (LocusQ: submodule + find_package + namespaced fallback; echoform/monument-reverb: submodule-only; memory-echoes: find_package-only). This causes recurring CMake misconfiguration failures during harness upgrades. |
| How will we deliver it? | Author `cmake/qa_harness_integration.cmake` in harness; update LocusQ CMakeLists.txt as reference; document migration guide for other plugins. |
| When is it done? | When harness ships `cmake/qa_harness_integration.cmake`, LocusQ uses it, and LocusQ build + QA CI passes with no regressions. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-085-cmake-integration-module.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-085 |
| Priority | P1 |
| Status | Open |
| Track | G - Tooling / Governance |
| Effort | Med / M |
| Depends On | BL-082 (runner app library; best adopted together) |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Publish a harness CMake module that normalizes plugin integration across three lookup strategies:

1. **Submodule** (`external/audio-dsp-qa-harness` or configurable path via `QA_HARNESS_DIR`)
2. **`find_package`** (`find_package(audio_dsp_qa_harness CONFIG)`)
3. **Namespaced target fallback** (`audio_dsp_qa_harness::qa_core`, etc.)

The `enable_qa_harness()` function:
- Detects which strategy applies (submodule present → use it; else find_package; else error with actionable message)
- Sets up include paths, link targets, and compile definitions
- Accepts `TARGET`, `ADAPTER_SOURCES`, `EXTRA_INCLUDES`, and optional `QA_FLAG_NAME` arguments
- Emits `STATUS` messages identifying which detection path was used (aids CI debugging)

## Acceptance IDs

- `audio-dsp-qa-harness` contains `cmake/qa_harness_integration.cmake` with `enable_qa_harness()` function
- LocusQ `CMakeLists.txt` QA harness block is replaced with `include(qa_harness_integration)` + `enable_qa_harness(...)` and produces identical build artifacts
- LocusQ CI `qa_harness.yml` passes with zero regressions after CMake change
- Module handles missing harness gracefully: emits actionable error message rather than CMake cryptic target-not-found failure
- `cmake/qa_harness_integration.cmake` includes inline documentation of all arguments and detection strategy

## Methodology Reference

- BL-085 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- LocusQ reference CMake block: `CMakeLists.txt` lines 475–509

## Implementation Slices

### S1 — Author `qa_harness_integration.cmake`
Implement three-strategy detection and `enable_qa_harness()` function. Write inline documentation.

### S2 — Update LocusQ CMakeLists.txt
Replace existing harness integration block. Run `cmake --build` + full QA suite; confirm zero regressions.

### S3 — Migration guide
Add `cmake/MIGRATION.md` in harness documenting the three-step migration for echoform/memory-echoes/monument-reverb.

## Validation Plan

QA harness script: `scripts/qa-bl085-cmake-integration-mac.sh` (to be authored in S2).
Evidence schema: `TestEvidence/bl085_*/status.tsv`.

Gate criterion: LocusQ CMake configure + build + QA smoke lane produce identical results to pre-change baseline.

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

Additional field required at handoff: `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit introducing `cmake/qa_harness_integration.cmake`.

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook lists only item-specific exceptions or additions.
