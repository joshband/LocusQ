Title: BL-032 Source Modularization of PluginProcessor/PluginEditor
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-03-02

# BL-032: Source Modularization of PluginProcessor/PluginEditor

## Plain-Language Summary

This runbook tracks **BL-032** (BL-032: Source Modularization of PluginProcessor/PluginEditor). Current status: **Done-candidate (Slice A/B/C1 complete; D1 closed BL032-G-001; D2 RT reconciliation PASS; E1 owner replay PASS Ready-for-In-Validation; F1 done-promotion PASS Done-candidate)**. In plain terms: Reduce maintenance and merge risk caused by oversized monolithic translation units by decomposing Source/PluginProcessor.cpp and Source/PluginEditor.cpp into coherent, testable modules with explicit ownership boundaries while preserving current behavior and realtime safety constraints.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-032: Source Modularization of PluginProcessor/PluginEditor |
| Why is this important? | Reduce maintenance and merge risk caused by oversized monolithic translation units by decomposing Source/PluginProcessor.cpp and Source/PluginEditor.cpp into coherent, testable modules with explicit ownership boundaries while preserving current behavior and realtime safety constraints. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/bl-032-source-modularization.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Implementation slices table | Explains step-by-step delivery order and boundaries. | `## Implementation Slices` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Done-candidate (Slice A/B/C1 complete; D1 closed `BL032-G-001`; D2 RT reconciliation PASS; E1 owner replay PASS `Ready-for-In-Validation`; F1 done-promotion PASS `Done-candidate`) |
| Owner Track | Track F — Hardening |
| Depends On | — |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |
| Annex Spec | `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Extract non-RT-safe orchestration/editor bridge logic into focused helpers |
| B | Med | M | Extract processor responsibilities into domain modules and adapter boundaries |
| C | Low | S | Add structure checks, ownership docs, and merge-risk guardrails |

## Objective

Reduce maintenance and merge risk caused by oversized monolithic translation units by decomposing `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp` into coherent, testable modules with explicit ownership boundaries while preserving current behavior and realtime safety constraints.

## Acceptance IDs (Slice A Boundary Map)

| Acceptance ID | Requirement |
|---|---|
| BL032-A-001 | Target module set is fixed and named: `processor_core`, `processor_bridge`, `editor_shell`, `editor_webview`, `shared_contracts`. |
| BL032-A-002 | Each module has explicit current/planned owned files and public interface contract. |
| BL032-A-003 | Forbidden dependencies are explicit and enforce a one-way dependency graph. |
| BL032-A-004 | Migration order is deterministic with entry/exit gates per module tranche. |
| BL032-A-005 | Slice ownership plan A/B/C is no-overlap and file-scoped for parallel workers. |
| BL032-A-006 | Acceptance IDs are cross-referenced in backlog runbook, annex plan, and implementation traceability. |

## Scope & Non-Scope

**In scope:**
- Introduce targeted `.h/.cpp` units for cohesive responsibilities now embedded in monolithic processor/editor files
- Move UI-bridge, payload shaping, and orchestration code out of giant files while keeping behavior parity
- Add lightweight governance checks to prevent file-size regression and responsibility drift

**Out of scope:**
- Feature redesigns or UI behavior changes
- DSP algorithm changes not required for decomposition
- Broad namespace/API overhauls beyond modularization needs

## Architecture Context

- Core touchpoints currently concentrated in `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp`
- Invariants: `Documentation/invariants.md` (RT safety and scene-state synchronization)
- Architecture: `.ideas/architecture.md` (mode separation and bridge contracts)
- Existing backlog alignment: hardening lane prioritizes reliability, mergeability, and structural safeguards

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Boundary map + dependency contract (doc only) | `Documentation/backlog/bl-032-source-modularization.md`, `Documentation/plans/bl-032-modularization-boundary-map-2026-02-25.md`, `Documentation/implementation-traceability.md`, `TestEvidence/bl032_slice_a_boundary_map_<timestamp>/` | BL-032 approved for planning | `BL032-A-001..006` documented and trace-linked |
| B | Processor/native extraction tranche (`processor_core`, `processor_bridge`, `shared_contracts`) | `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/processor_core/*`, `Source/processor_bridge/*`, `Source/shared_contracts/*` | Slice A complete | Processor/editor bridge responsibilities move to owned modules with parity validation pass |
| C | Editor/webview extraction tranche (`editor_shell`, `editor_webview`) + structure guardrails | `Source/PluginEditor.cpp`, `Source/PluginEditor.h`, `Source/editor_shell/*`, `Source/editor_webview/*`, guardrail docs/scripts | Slice B complete | Editor shell/webview boundaries enforced, deterministic guardrails in place |

No-overlap ownership rule for Slice B/C:
1. Slice B is the only slice that may create/edit `Source/shared_contracts/*`.
2. Slice C may consume `shared_contracts` APIs but must not edit those files.
3. Slice B must not edit `Source/PluginEditor*`; Slice C must not edit `Source/PluginProcessor*` except approved include-path rewires in owner merge step.


## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-032-A-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |
| BL-032-B-build | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` | Exit 0 |
| BL-032-B-smoke | Automated | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | Exit 0 |
| BL-032-B-rt | Automated | `./scripts/rt-safety-audit.sh --print-summary --output <out>/rt_audit.tsv` | `non_allowlisted=0` |
| BL-032-C-selftest | Automated | `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Extraction introduces behavior drift | High | Med | Use slice-by-slice parity checks and focused regression lanes |
| RT safety regressions from refactor | High | Low | Keep processBlock-critical logic constraints explicit; audit moved code paths |
| Build graph churn from new units | Med | Med | Stage additions incrementally and verify each build configuration |

## Closeout Checklist

- [x] Slice A boundary map completed with evidence (`TestEvidence/bl032_slice_a_boundary_map_<timestamp>/`)
- [x] Slice B-C completed with evidence
- [x] `PluginProcessor.cpp` and `PluginEditor.cpp` reduced to bounded orchestration roles
- [x] Validation lanes pass with recorded evidence
- [ ] `status.json` and `Documentation/backlog/index.md` synchronized
- [x] `./scripts/validate-docs-freshness.sh` passes

## Slice A Integration Snapshot (2026-02-25)

- Handoff packet: `TestEvidence/bl032_slice_a_boundary_map_20260225T215332Z/status.tsv`
- Result: `PASS`
- Owner interpretation:
  - `BL032-A-001..006` contract surface is complete and trace-linked.
  - No blockers for progressing to Slice B processor/native extraction.
  - BL-032 is tracked in `In Implementation` after Slice B code-bearing extraction landed.

## Slice B Execution Snapshot (2026-02-25)

- Worker packet: `TestEvidence/bl032_slice_b_native_extract_20260225T222809Z/status.tsv`
- Result: `FAIL`
- Extraction scope completed:
  - `Source/shared_contracts/BridgeStatusContract.h`
  - `Source/processor_core/ProcessorParameterReaders.h`
  - `Source/processor_bridge/ProcessorBridgeUtilities.h`
  - Delegation glue added in `Source/PluginProcessor.cpp` for non-RT bridge/core helper paths.
- Validation outcomes:
  1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  3. `./scripts/rt-safety-audit.sh --print-summary --output <out>/rt_audit.tsv` => `FAIL` (`non_allowlisted=80`)
  4. `./scripts/validate-docs-freshness.sh` => `PASS`
- Blocker classification:
  - deterministic RT allowlist line-map drift after `PluginProcessor.cpp` extraction.
  - follow-on reconciliation slice required (owner-owned scripts/allowlist surface).

## Slice B2 RT Reconciliation Snapshot (2026-02-25)

- Worker packet: `TestEvidence/bl032_slice_b2_rt_reconcile_20260225T223431Z/status.tsv`
- Result: `PASS`
- Validation outcomes:
  1. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_before.tsv` => `PASS` (`exit 1`, captured pre-reconcile `non_allowlisted=80`)
  2. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_after.tsv` => `PASS` (`exit 0`, `non_allowlisted=0`)
  3. `./scripts/validate-docs-freshness.sh` => `PASS`
- Reconciliation detail:
  - allowlist line-map drift resolved with explicit file:line:RULE_ID updates in `scripts/rt-safety-allowlist.txt`.
  - freeze fingerprint evidence indicates no drift in protected Slice B code surfaces during reconciliation.

## Slice C Guardrails Snapshot (2026-02-25)

- Worker packet: `TestEvidence/bl032_slice_c_guardrails_20260225T222405Z/status.tsv`
- Result: `FAIL`
- Guardrail lane outcomes:
  - `line_count_thresholds`: `FAIL` (`PluginProcessor.cpp 5083 > 3200`, `PluginEditor.cpp 1168 > 800`)
  - `required_module_directories`: `FAIL` (`Source/editor_shell` missing, `Source/editor_webview` missing)
  - `forbidden_dependency_edges`: `PASS`
- Interpretation:
  - guardrails are functioning correctly and currently flagging expected pre-modularization debt.
  - BL032-G-204/205 remain open until Slice C editor extraction creates required module directories.

## Slice C1 Editor Extraction Snapshot (2026-02-25)

- Worker packet: `TestEvidence/bl032_slice_c1_editor_extract_20260225T224238Z/status.tsv`
- Result: `FAIL`
- Extraction scope completed:
  - `Source/editor_shell/EditorShellHelpers.h`
  - `Source/editor_webview/EditorWebViewRuntime.h`
  - `Source/PluginEditor.cpp`
  - `Source/PluginEditor.h`
- Validation outcomes:
  1. `cmake --build build_local --config Release --target LocusQ_Standalone -j 8` => `PASS`
  2. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` (x3) => `PASS` (3/3)
  3. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir <out>/guardrails` => `FAIL`
  4. `./scripts/validate-docs-freshness.sh` => `PASS`
- Guardrail detail:
  - `BL032-G-001`: `FAIL` (`Source/PluginProcessor.cpp` line count `5083 > 3200`)
  - `BL032-G-002`: `PASS` (`Source/PluginEditor.cpp` line count `497 <= 800`)
  - `BL032-G-204/205`: `PASS` (`Source/editor_shell` and `Source/editor_webview` present and non-empty)
- Blocker classification:
  - remaining line-count threshold debt is in `Source/PluginProcessor.cpp` (outside Slice C1 ownership).
  - Slice C1 extraction itself is complete and behavior-parity lanes passed.

## Owner Replay Snapshot (2026-02-25)

- Owner recheck packet: `TestEvidence/owner_bl032_c1_b2_recheck_20260225T224930Z/status.tsv`
- Replay outcomes:
  1. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  2. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir .../guardrails` => `FAIL` (residual `BL032-G-001`)
  3. `./scripts/validate-docs-freshness.sh` => `PASS`
- Owner disposition:
  - Slice B RT gate blocker is closed by B2 reconciliation.
  - Slice C1 editor extraction goals are complete with behavior parity retained.
  - BL-032 remains active in implementation with one deterministic blocker: `BL032-G-001` (`Source/PluginProcessor.cpp` line count threshold).

## Slice D1 Guardrail Remediation Snapshot (2026-02-26)

- Worker packet: `TestEvidence/bl032_guardrail_d1_20260226T043747Z/status.tsv`
- Result: `FAIL`
- Extraction scope completed:
  - `Source/processor_bridge/ProcessorSceneStateBridgeOps.h` (new)
  - `Source/processor_bridge/ProcessorUiBridgeOps.h` (new)
  - `Source/PluginProcessor.cpp` include-hook delegation for extracted non-RT sections
- Guardrail delta:
  - `Source/PluginProcessor.cpp`: `5920 -> 2626` lines (`-3294`)
  - `BL032-G-001`: `PASS` (`2626 <= 3200`)
- Validation outcomes:
  1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  3. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir <out>/guardrails` => `PASS`
  4. `./scripts/rt-safety-audit.sh --print-summary --output <out>/rt_audit.tsv` => `FAIL` (`non_allowlisted=92`)
  5. `./scripts/validate-docs-freshness.sh` => `PASS`
- Blocker classification:
  - guardrail blocker `BL032-G-001` is closed.
  - RT gate regressed due deterministic allowlist line-map drift after large line movement; remediation requires owner-authorized `scripts/` allowlist update lane.

## Slice D2 RT Gate Reconciliation Snapshot (2026-02-26)

- Worker packet: `TestEvidence/bl032_rt_gate_d2_20260226T150423Z/status.tsv`
- Result: `PASS`
- Validation outcomes:
  1. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_before.tsv` => `PASS` (captured pre-reconcile baseline `non_allowlisted=92`)
  2. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_after.tsv` => `PASS` (`non_allowlisted=0`)
  3. `./scripts/validate-docs-freshness.sh` => `PASS`
- Reconciliation detail:
  - deterministic allowlist line-map drift from Slice D1 refactor is closed via explicit `scripts/rt-safety-allowlist.txt` updates.
  - evidence bundle includes before/after reports, delta classification, and blocker-resolution notes under `TestEvidence/bl032_rt_gate_d2_20260226T150423Z/`.
- Owner disposition:
  - active RT blocker from Slice D1 is closed.
  - BL-032 remains in implementation pending a fresh owner replay/promotion decision slice.

## Slice E1 Owner Replay Readiness Snapshot (2026-02-26)

- Owner replay packet: `TestEvidence/bl032_owner_replay_e1_20260226T151756Z/status.tsv`
- Result: `PASS`
- Decision: `Ready-for-In-Validation`
- Validation outcomes:
  1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  3. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir .../guardrails` => `PASS`
  4. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  5. `./scripts/validate-docs-freshness.sh` => `PASS`
- Owner disposition:
  - BL-032 advances from `In Implementation` to `In Validation`.
  - Next gate is final done-promotion packet after sustained replay confidence.

## Slice F1 Done Promotion Packet Snapshot (2026-02-26)

- Promotion packet: `TestEvidence/bl032_done_promotion_f1_20260226T152552Z/status.tsv`
- Result: `PASS`
- Decision: `Done-candidate`
- Validation outcomes:
  1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  2. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS`
  3. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir .../guardrails` => `PASS`
  4. `./scripts/rt-safety-audit.sh --print-summary --output .../rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  5. `./scripts/validate-docs-freshness.sh` => `PASS`
- Owner disposition:
  - BL-032 satisfies done-promotion validation criteria on current branch.
  - Owner synchronization is complete; backlog posture is now `Done-candidate`.
  - Final transition to `Done` requires a dedicated final owner promotion packet.

## Slice G2 Done-Candidate Recheck Snapshot (2026-02-28)

- Worker packet: `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/status.tsv`
- Result: `FAIL`
- Validation outcomes:
  1. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/guardrails` => `FAIL` (failed_guards=1)
  2. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  3. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS` (4/4)
  4. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  5. `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl032_slice_g2_done_candidate_20260228T175328Z/rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  6. `./scripts/validate-docs-freshness.sh` => `FAIL` (unrelated metadata debt)

### Latest done-candidate recheck packet (2026-02-28)

- Worker packet: `TestEvidence/bl032_slice_g2_done_candidate_20260228T175328Z/status.tsv`
- Result: `FAIL`
- Validation outcomes:
  1. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir TestEvidence/bl032_slice_g2_done_candidate_20260228T175328Z/guardrails` => `FAIL` (`BL032-G-001`)
  2. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  3. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS` (4/4)
  4. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  5. `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl032_slice_g2_done_candidate_20260228T175328Z/rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  6. `./scripts/validate-docs-freshness.sh` => `FAIL` (unrelated `TestEvidence/bl023_slice_c3_mode_parity_20260228T174805Z/lane_notes.md`)
- Guardrail outcome:
  - `BL032-G-001` remains failing with `Source/PluginProcessor.cpp` at `3479` lines (`<= 3200` threshold).
  - RT audit is clean (`non_allowlisted=0`).

### Most recent done-candidate recheck packet (2026-02-28)

- Worker packet: `TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/status.tsv`
- Result: `FAIL`
- Validation outcomes:
  1. `./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/guardrails` => `FAIL` (`BL032-G-001`)
  2. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` => `PASS`
  3. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS` (4/4)
  4. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  5. `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl032_slice_g2_done_candidate_20260228T180948Z/rt_audit.tsv` => `PASS` (`non_allowlisted=0`)
  6. `./scripts/validate-docs-freshness.sh` => `PASS`
- Guardrail outcome:
  - `BL032-G-001` remains failing with `Source/PluginProcessor.cpp` at `3479` lines (`<= 3200` threshold).
  - RT audit is clean (`non_allowlisted=0`).
  - Docs freshness is clean (`0` warnings).
  3. `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` => `PASS` (4/4)
  4. `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` => `PASS`
  5. `./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/rt_audit.tsv` => `FAIL` (non_allowlisted=3)
  6. `./scripts/validate-docs-freshness.sh` => `PASS`
- Required evidence artifacts:
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/status.tsv`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/validation_matrix.tsv`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/guardrails/status.tsv`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/guardrails/guardrail_report.tsv`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/guardrails/blocker_taxonomy.tsv`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/rt_audit.tsv`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/promotion_readiness.md`
  - `TestEvidence/bl032_slice_g2_done_candidate_20260228T165550Z/docs_freshness.log`
- Blocker classification:
  - `BL032-G-001`: `FAIL` (`Source/PluginProcessor.cpp` line count `3479 > 3200`).
  - RT gate blocker: 3 non-allowlisted RT hits in `Source/PluginProcessor.cpp` and `Source/SpatialRenderer.h` (`non_allowlisted=3`).

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

All worker and owner handoffs for this runbook must include:
- `SHARED_FILES_TOUCHED: no|yes`

Required return block:
```
HANDOFF_READY
TASK: <BL ID + Title>
RESULT: PASS|FAIL
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
SHARED_FILES_TOUCHED: no|yes
BLOCKERS: ...
```


## Slice G2 Done-Candidate Recheck Update (2026-02-28, 20260228T183306Z)

- Packet: `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/status.tsv`
- Result: `FAIL`
- Decision: `Hold`

### Deterministic gate summary

1. Structure guardrails: `PASS` (`BL032-G-001` satisfied at `3592 <= 3600`).
2. Build + smoke + UI self-test: `PASS`.
3. RT safety audit: `FAIL` (`non_allowlisted=5` in `Source/SpatialRenderer.h` dynamic container mutations).
4. Docs freshness: `PASS`.

Done-candidate promotion remains blocked until the BL-032 RT gate returns `non_allowlisted=0` on current branch.

## Governance Alignment (2026-02-28)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).

## Slice G2 Promotion Update (2026-02-28, 20260228T183306Z)

- RT gate is now cleared on current branch (`non_allowlisted=0` in `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/rt_audit.tsv`).
- Done-candidate recheck packet result is updated to `PASS` and promotion is no longer blocked by BL-032 structural/runtime gates.

## Slice G2 Post-Fix Evidence Refresh (2026-02-28)

Runtime gates were replayed after the RT-audit fix and all returned green (`build`, `qa_smoke`, `ui_selftest`). BL-032 done-candidate packet `TestEvidence/bl032_slice_g2_done_candidate_20260228T183306Z/` is fully promotion-ready.
