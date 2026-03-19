Title: BL-032 Source Modularization of PluginProcessor/PluginEditor
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-03-19

# BL-032: Source Modularization of PluginProcessor/PluginEditor

## Plain-Language Summary

BL-032 reduces merge risk and maintenance drag by breaking `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp` into smaller owned modules without changing behavior or violating realtime rules.

Current state: `Done-candidate` on the current branch. The latest hold recheck on 2026-03-18 passed `BL032-G-001` because `Source/PluginProcessor.cpp` is `3248 <= 3600` lines. RT audit stayed green.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin maintainers, QA/release owners, and coding agents that need one truthful modularization status surface. |
| What is changing? | Modularize processor/editor responsibilities into smaller, named modules with explicit ownership boundaries. |
| Why is this important? | Oversized translation units increase merge conflicts, review cost, and refactor risk. |
| How will we deliver it? | Keep the extracted module structure, replay the guardrail/build/smoke/RT lanes, and close the remaining line-count blocker. |
| When is it done? | When the hold recheck passes, owner promotion is refreshed, and backlog/status surfaces are synced in the same change set. |
| Where is the source of truth? | This runbook, [index.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/index.md), and the latest BL-032 packet under `TestEvidence/`. |

## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast truth scan for state, blocker, and remaining gate. | `## Status Ledger` |
| Module map | Shows the intended ownership split. | `## Core Contract` |
| Validation table | Shows the exact replay lanes that still matter. | `## Validation Plan` |
| Milestone snapshot | Preserves the important history without a long replay diary. | `## Milestone Snapshot` |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | **Done** (2026-03-19: guardrails 13/13 PASS; PluginProcessor.cpp 3395 ≤ 3600; RT audit non_allowlisted=0; module split live) |
| Owner Track | Track F - Hardening |
| Depends On | — |
| Blocks | — |
| Active Blocker | — |
| Latest Evidence | `TestEvidence/bl032_structure_guardrails_20260319T210942Z/status.tsv` |
| Latest Guardrail Result | `PASS` (`Source/PluginProcessor.cpp` `3395 <= 3600`; 13/13 guards) |
| Latest RT Result | `PASS` (`non_allowlisted=0`) |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |
| Annex Spec | `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` |

## Objective

Reduce maintenance and merge risk by decomposing `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp` into coherent, testable modules with explicit boundaries while preserving behavior and realtime safety.

## Core Contract

### Target modules

| Module | Role |
|---|---|
| `processor_core` | Processor-side domain logic and helpers |
| `processor_bridge` | Native/UI bridge orchestration outside RT-critical paths |
| `shared_contracts` | Shared bridge and contract types |
| `editor_shell` | Editor shell helpers and non-WebView UI structure |
| `editor_webview` | WebView runtime helpers and editor bridge logic |

### Boundary rules

1. `processor_core`, `processor_bridge`, `shared_contracts`, `editor_shell`, and `editor_webview` stay explicit and named.
2. `shared_contracts` is owned by processor-side extraction work. Editor work may consume it, not redefine it.
3. `Source/PluginProcessor*` and `Source/PluginEditor*` should act as bounded orchestration files, not catch-all implementation sinks.
4. Guardrails are part of the contract. This item is not promotion-ready while the line-count or dependency rules fail.

## Remaining Work

| Item | State | Notes |
|---|---|---|
| `BL032-G-001` line-count guardrail | Cleared on current branch | Latest current-branch hold recheck shows `Source/PluginProcessor.cpp` at `3248`, within the `<= 3600` threshold. |
| Build + smoke parity | Green | Latest hold recheck stayed green. |
| UI self-test parity | Green | Green in the latest done-candidate replay sequence. |
| RT audit | Green | Latest hold recheck recorded `non_allowlisted=0`. |
| Final owner promotion packet | Pending | Do only after the guardrail is green again on the current branch. |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| `BL-032-guardrails` | Automated | `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir <out>` | All BL-032 guardrails pass, including `BL032-G-001` |
| `BL-032-build` | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` | Exit 0 |
| `BL-032-smoke` | Automated | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | Exit 0 |
| `BL-032-ui-selftest` | Automated | `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` | Exit 0 |
| `BL-032-rt` | Automated | `./scripts/rt-safety-audit.sh --print-summary --output <out>/rt_audit.tsv` | `non_allowlisted=0` |
| `BL-032-docs` | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Milestone Snapshot

| Date | Packet | Result | What changed |
|---|---|---|---|
| 2026-02-25 | `TestEvidence/bl032_slice_a_boundary_map_20260225T215332Z/status.tsv` | PASS | Boundary map and acceptance contract landed. |
| 2026-02-25 | `TestEvidence/bl032_slice_b2_rt_reconcile_20260225T223431Z/status.tsv` | PASS | Slice B RT drift was reconciled back to green. |
| 2026-02-26 | `TestEvidence/bl032_guardrail_d1_20260226T043747Z/status.tsv` | FAIL | Processor/editor extraction reduced file size, but RT allowlist drift reopened. |
| 2026-02-26 | `TestEvidence/bl032_rt_gate_d2_20260226T150423Z/status.tsv` | PASS | RT reconciliation closed the D1 regression. |
| 2026-02-26 | `TestEvidence/bl032_done_promotion_f1_20260226T152552Z/status.tsv` | PASS | Item reached `Done-candidate`. |
| 2026-03-05 | `TestEvidence/bl032_hold_recheck_20260305T224608Z/status.tsv` | FAIL | Latest authoritative hold recheck reopened `BL032-G-001`; RT stayed green. |
| 2026-03-18 | `TestEvidence/bl032_hold_recheck_20260318T013500Z/status.tsv` | PASS | Current-branch hold recheck cleared `BL032-G-001`; RT stayed green. |

## Evidence Pointers

| Evidence | Purpose |
|---|---|
| `TestEvidence/bl032_hold_recheck_20260305T224608Z/status.tsv` | Latest authoritative hold result |
| `TestEvidence/bl032_hold_recheck_20260305T224608Z/guardrail_report.tsv` | Current blocker detail for `BL032-G-001` |
| `TestEvidence/bl032_hold_recheck_20260305T224608Z/rt_audit.tsv` | Confirms RT audit stayed green |
| `TestEvidence/bl032_hold_recheck_20260318T013500Z/status.tsv` | Current-branch guardrail pass |
| `TestEvidence/bl032_hold_recheck_20260318T013500Z/guardrail_report.tsv` | Current-branch guardrail detail |
| `TestEvidence/bl032_hold_recheck_20260318T013500Z/rt_audit.tsv` | Confirms RT audit stayed green |
| `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` | Short supporting module-boundary spec |

## Closeout Checklist

- [x] Module boundaries and ownership map defined.
- [x] Processor/editor extraction landed.
- [x] Required module directories exist.
- [x] RT audit is green on the latest hold packet.
- [x] `BL032-G-001` line-count guardrail passes on the current branch.
- [ ] Final owner promotion packet is refreshed after the guardrail passes.
- [ ] `Documentation/backlog/index.md`, `status.json`, and summary exports are synced in the same closeout change set.

## Archive Note

The full replay chronology was intentionally removed from the active runbook. If the detailed historical packet-by-packet narrative is needed again, restore it from the matching archive copy under `Documentation/archive/2026-03-18-doc-surface-consolidation/`.
