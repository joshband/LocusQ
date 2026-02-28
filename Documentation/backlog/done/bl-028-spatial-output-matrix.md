---
Title: BL-028 Spatial Output Matrix
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-28
---

# BL-028 — Spatial Output Matrix Enforcement

## 1. Status Ledger

| Field | Value |
|---|---|
| ID | BL-028 |
| Status | Done (owner sync packet finalized from Slice D promotion evidence) |
| Priority | P2 |
| Track | A (DSP/Architecture) + C (UX Authoring) |
| Effort | High / L total; Med / M per slice |
| Depends | BL-017, BL-026, BL-027 |
| Blocks | BL-029 |
| Annex Spec | `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md` |
| QA Contract | `Documentation/testing/bl-028-spatial-output-matrix-qa.md` |

## 2. Objective

Define one authoritative and deterministic matrix contract that governs legal spatial output behavior across:
- `binaural` (internal binaural stereo),
- `stereo`,
- `quad`,
- `5.1`,
- `7.1`,
- `7.4.2`,
- and `ExternalSpatial` multichannel bed operation.

The contract must specify mismatch handling, fallback behavior, diagnostics schema, fail-safe routing, and user-visible status text for blocked transitions.

## 3. Acceptance IDs (A1 Planning)

| Acceptance ID | Requirement |
|---|---|
| BL028-A1-001 | Authoritative matrix behavior is defined for stereo/quad/5.1/7.1/7.4.2/binaural. |
| BL028-A1-002 | Mismatch handling contract defines fallback mode precedence and fail-safe routing. |
| BL028-A1-003 | Diagnostics fields/enums are explicitly specified for requested/active/rule/fallback states. |
| BL028-A1-004 | User-visible status text is deterministic and reason-code based. |
| BL028-A1-005 | Deterministic QA lane spec includes scenarios, artifact schema, and thresholds. |
| BL028-A1-006 | Acceptance IDs are cross-referenced consistently across runbook/spec/qa docs. |

## 4. Deterministic Matrix Contract Summary

Normative matrix rows and rule IDs are defined in:
- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md`

High-level behavior:
1. `InternalBinaural` is legal only on stereo output.
2. `Multichannel` is legal only on `quad/5.1/7.1/7.4.2` layouts.
3. `ExternalSpatial` is legal only on multichannel bed outputs (>=4 channels).
4. Head tracking is legal only for `InternalBinaural`.
5. Illegal requested states are blocked and routed through deterministic fallback policy.

## 5. Mismatch Handling Contract Summary

The mismatch/fallback contract is normative in the 2026-02-25 annex spec and includes:
1. Fallback precedence:
   - `retain_last_legal`
   - `derive_from_host_layout`
   - `safe_stereo_passthrough`
2. Fail-safe behavior for no-legal-state conditions.
3. Explicit reason-code and status text mapping.

No silent auto-correction is permitted.

## 6. Diagnostics Contract Summary

Scene-state must publish the full matrix diagnostics set (requested/active domain/layout, rule ID/state, fallback mode, fail-safe route, reason code, status text, event sequence) exactly as defined in the annex spec.

## 7. QA Lane Contract Summary

Deterministic lane requirements are defined in:
- `Documentation/testing/bl-028-spatial-output-matrix-qa.md`
- `scripts/qa-bl028-output-matrix-lane-mac.sh`
- `qa/scenarios/locusq_bl028_output_matrix_suite.json`

This includes:
1. required scenario set,
2. required artifact schema,
3. deterministic thresholds,
4. pass/fail rules,
5. acceptance ID parity check.

## 8. Slice A1 Deliverables

- [x] Runbook acceptance language updated with deterministic criteria.
- [x] Annex spec updated (`2026-02-25`) with authoritative matrix + mismatch + diagnostics contracts.
- [x] QA contract doc created with deterministic scenario/artifact/threshold definitions.
- [x] Cross-reference acceptance IDs added to runbook/spec/qa docs.

## 9. Slice B1 Deliverables (QA Lane Scaffold)

- [x] Deterministic BL-028 scenario suite scaffold added (`qa/scenarios/locusq_bl028_output_matrix_suite.json`).
- [x] Executable lane runner added (`scripts/qa-bl028-output-matrix-lane-mac.sh`).
- [x] Lane emits deterministic artifacts: `status.tsv`, `qa_lane.log`, `scenario_result.log`, `matrix_report.tsv`, `acceptance_parity.tsv`.
- [x] Lane checks mapped directly to `BL028-A1-001..006`.

## 10. Entry Criteria for Implementation Slices

Before implementation work begins:
1. BL-017, BL-026, BL-027 dependency contracts must remain green.
2. BL-028 A1 acceptance IDs (`BL028-A1-001..006`) must remain stable.
3. Docs freshness gate must pass.

## 11. Closeout Criteria for Slice A1/B1

- [x] `BL028-A1-001` satisfied.
- [x] `BL028-A1-002` satisfied.
- [x] `BL028-A1-003` satisfied.
- [x] `BL028-A1-004` satisfied.
- [x] `BL028-A1-005` satisfied.
- [x] `BL028-A1-006` satisfied.
- [x] Slice B1 lane scaffold executable with deterministic artifact schema.
- [x] Slice B2 reliability replay hardening complete (`--runs 5` deterministic parity).
- [x] Slice C1 native additive matrix diagnostics publication complete.
- [x] Slice C2 RT gate reconciled (`non_allowlisted=0`).
- [x] Slice D done-promotion packet assembled with fresh replay.
- [x] Owner sync to backlog index/status/evidence surfaces complete.

## 12. Slice D Promotion Packet (Owner Finalized)

Promotion packet bundle:
- `TestEvidence/bl028_done_promotion_slice_d_20260225T211241Z/`

Required prior evidence cited:
1. `TestEvidence/bl028_slice_b1_20260225T181438Z/`
2. `TestEvidence/bl028_slice_b2_20260225T183554Z/`
3. `TestEvidence/bl028_slice_c1_20260225T210253Z/`
4. `TestEvidence/bl028_rt_gate_c2_20260225T210743Z/`

Fresh Slice D replay summary:
1. Build (`locusq_qa` + `LocusQ_Standalone`): PASS.
2. BL-028 lane (`--runs 5`): PASS (`divergence_count=0`, `transient_failures=0`).
3. RT static audit: PASS (`non_allowlisted=0`).
4. Docs freshness: PASS.

Owner disposition:
- `DONE` (promoted by owner sync packet using Slice D fresh replay + prior B1/B2/C2 evidence).


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-028-spatial-output-matrix.md` |

### Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | Legacy closeout documented | `Evidence References` |
| Lane replay/parity | Legacy closeout documented | `Evidence References` |
| RT safety | Legacy closeout documented | `Evidence References` |
| Docs freshness | Legacy closeout documented | `Evidence References` |
| Status schema | Legacy closeout documented | `Evidence References` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | Required for modern promotions; legacy packets may predate marker | `Evidence References` |

### Backlog/Status Sync Checklist

- [x] Runbook archived under `Documentation/backlog/done/`
- [x] Backlog index links the done runbook
- [x] Historical evidence references retained
- [ ] Legacy packet retrofitted to modern owner packet template (`_template-promotion-decision.md`) where needed
- [ ] Legacy closeout fully normalized to modern checklist fields where needed
