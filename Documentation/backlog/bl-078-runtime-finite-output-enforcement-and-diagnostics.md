Title: BL-078 Runtime Finite-Output Enforcement and Diagnostics
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-05
Last Modified Date: 2026-03-17

# BL-078 Runtime Finite-Output Enforcement and Diagnostics

## Plain-Language Summary

BL-078 in plain terms: Carry BL-036's remaining runtime work into actual processor-side finite guardrails, additive diagnostics publication, and execute-mode fuzz/soak replay so finite-output protection is implemented, observable, and owner-verifiable instead of only specified on paper. Current state: Open (created on 2026-03-05 as the explicit follow-on after BL-036 was archived as a contract-and-evidence item). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, QA/release owners, and audio-engine maintainers who need real runtime finite-output containment, not just contract documentation. |
| What is changing? | Implement native finite guardrails in runtime DSP paths, publish additive finite-output diagnostics from processor surfaces, and prove the behavior with execute-mode fuzz/soak evidence. |
| Why is this important? | It keeps the remaining runtime safety work visible and owned after BL-036 closes, so the backlog does not hide processor-side finite-output debt behind a documentation-only done state. |
| How will we deliver it? | Reuse BL-036 as the contract authority, implement the runtime slices in scoped steps, and capture execute-mode evidence under `TestEvidence/...` before any promotion claims. |
| When is it done? | Current state: Open. This item is done when runtime guardrails, additive diagnostics, fuzz/soak replay, RT audit, and owner closeout evidence are all green. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-078-runtime-finite-output-enforcement-and-diagnostics.md`, BL-036 contract authority in `Documentation/backlog/done/bl-036-dsp-finite-output-guardrails.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Scope split table | Shows exactly what moved out of BL-036. | `## Scope Split Origin (2026-03-05)` |
| Implementation slices | Clarifies the runtime sequence and exit criteria. | `## Implementation Slices` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Scope Split Origin`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-078 |
| Priority | P0 |
| Status | In Implementation (A2 native guardrail loop + B2 diagnostics publication landed in Source/ 2026-03-17; finite-output lane 3/3 PASS `TestEvidence/bl078_a2_verify_20260317T182136Z/`; SIGSEGV root-caused and fixed 2026-03-17 — HeapBlock::malloc JUCE8 API break in SceneGraph.h + SpatialRenderer.h; smoke suite 8/8 PASS `TestEvidence/sigsegv_fix_20260317T192604Z/`; C1 fuzz/soak harness and owner replay pending) |
| Track | F - Hardening |
| Effort | Med / M |
| Depends On | BL-036 (Done) |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Implement the runtime finite-output guardrails, additive diagnostics publication, and execute-mode fuzz/soak replay required to satisfy BL-036's contract authority with real processor-side evidence.

## Scope Split Origin (2026-03-05)

BL-078 was created to carry the exact runtime TODOs that were still open inside BL-036 when its contract lane was ready to archive.

| Moved Slice | Work Now Owned By BL-078 | Origin |
|---|---|---|
| A2 | Implement native finite guardrails in runtime DSP paths. | BL-036 `## TODOs` |
| B2 | Publish additive finite-output diagnostics from processor surfaces. | BL-036 `## TODOs` |
| C1 | Execute runtime finite-output fuzz/soak lane and owner replay. | BL-036 `## TODOs` |

BL-036 remains the contract/taxonomy authority. BL-078 is the runtime enforcement follow-on.

## Acceptance IDs

- `BL078-A-001`: Runtime DSP paths implement BL-036 finite guardrails at the documented protected boundaries without introducing non-RT-safe behavior.
- `BL078-A-002`: Processor surfaces publish additive finite-output diagnostics with a stable schema aligned to BL-036's diagnostics contract.
- `BL078-A-003`: Execute-mode finite-output fuzz/soak evidence shows zero non-contained NaN/Inf leaks and captures deterministic machine-readable results.
- `BL078-A-004`: Smoke, RT audit, and docs/status governance surfaces remain green after runtime enforcement lands.
- `BL078-A-005`: Owner closeout packet records the runtime follow-on disposition with repo-local evidence.

## Scope

In scope:
- Native finite guardrail integration in runtime DSP paths.
- Additive finite-output diagnostics publication from processor surfaces.
- Execute-mode finite-output fuzz/soak lane evidence and owner replay.
- RT-audit confirmation that the runtime work does not introduce new non-allowlisted issues.

Out of scope:
- Rewriting BL-036 contract authority or acceptance taxonomy.
- New spatial rendering features unrelated to finite-output protection.
- UI redesign or visualization-only work unrelated to diagnostics publication.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A2 | Native runtime finite-guardrail integration | Protected runtime boundaries satisfy BL-036 contract without non-finite host-output leakage |
| B2 | Additive diagnostics publication + fallback telemetry | Processor surfaces publish stable finite-output diagnostics aligned to BL-036 |
| C1 | Runtime fuzz/soak execute replay | Execute-mode replay shows zero non-contained leaks and records machine-readable evidence |
| D1 | Owner readiness replay | Build/smoke/RT/docs/status gates are green with runtime finite-output evidence attached |

## TODOs

- [x] Implement native finite guardrails in runtime DSP paths. (`Source/PluginProcessor.cpp` — post-renderer guardrail loop; `Source/PluginProcessor.h` — `PublishedFiniteGuardrailDiagnostics` struct)
- [x] Publish additive finite-output diagnostics from processor surfaces. (6 atomic fields published each Renderer block with seqlock-style release fence)
- [x] Fix pre-existing SIGSEGV in locusq_qa (HeapBlock::malloc JUCE8 API break — SceneGraph.h:86 + SpatialRenderer.h:551; smoke suite 8/8 PASS `TestEvidence/sigsegv_fix_20260317T192604Z/`).
- [ ] Execute runtime finite-output fuzz/soak lane and owner replay.
- [ ] Synchronize backlog/index/status/evidence surfaces when BL-078 advances.

## A2/B2 Implementation Snapshot (2026-03-17)

- Build: `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` → PASS
- Finite-output contract lane replay (3/3): `TestEvidence/bl078_a2_verify_20260317T182136Z/status.tsv` → PASS
- Docs freshness: PASS
- SIGSEGV root-caused 2026-03-17: HeapBlock::malloc JUCE8 API break in SceneGraph.h:86 + SpatialRenderer.h:551 — fixed; smoke suite 8/8 PASS
- Remaining: C1 fuzz/soak harness (`scripts/qa-bl078-runtime-finite-output-mac.sh`) and D1 owner closeout

## Validation Plan

Primary runtime lane:
- `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
- `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json`
- `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh`
- `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl078_<slice>_<timestamp>/rt_audit.tsv`

Required execute-mode additions:
- runtime finite-output fuzz/soak command(s) and scenario contract
- additive diagnostics schema capture
- owner replay / promotion-decision packet

## Evidence Contract

Minimum required outputs:
- `status.tsv`
- `validation_matrix.tsv`
- `finite_fuzz.tsv`
- `diagnostics_schema.tsv`
- `rt_audit.tsv`
- `lane_notes.md`
- `docs_freshness.log`

Owner closeout additions:
- `promotion_decision.md`
- `handoff_resolution.md`
