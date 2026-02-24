Title: LocusQ Backlog Execution Runbooks
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# LocusQ Backlog Execution Runbooks

> **DEPRECATED (2026-02-24):** This file is superseded by individual runbook docs in `Documentation/backlog/`. Each backlog item now has its own standardized runbook with agent mega-prompts, validation plans, and evidence contracts. This file is retained as Tier 2 reference only. Do not update this file.

## Purpose
Provide execution-level runbooks for all open BL/HX items tracked in the master backlog.

## Preservation Guarantee
1. No annex plan documents are replaced or discarded by this runbook.
2. Historical reviews remain in `Documentation/archive/` as reference sources.
3. Completed work evidence remains in `TestEvidence/` and is not collapsed into summaries only.
4. The master backlog remains authoritative; this runbook is procedural support.

## Usage Contract
1. Select a row from the master backlog queue in `Documentation/backlog-post-v1-agentic-sprints.md`.
2. Use the matching runbook entry below.
3. Run listed validations and capture listed evidence artifacts.
4. Synchronize status/evidence surfaces in the same change set.

## Global Closeout Checklist
1. `status.json` updated when acceptance or state claims change.
2. `TestEvidence/build-summary.md` snapshot updated.
3. `TestEvidence/validation-trend.md` trend row added.
4. `Documentation/backlog-post-v1-agentic-sprints.md` row state synchronized.
5. `./scripts/validate-docs-freshness.sh` passes.

## Runbook Index
| Runbook | Backlog ID | Owner Track | Annex Spec |
|---|---|---|---|
| RB-BL025 | BL-025 | Track C | `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md` |
| RB-BL014 | BL-014 | Track B | `Documentation/scene-state-contract.md` + BL-014 evidence bundle |
| RB-BL018 | BL-018 | Track A | `Documentation/spatial-audio-profiles-usage.md` |
| RB-BL022 | BL-022 | Track C | choreography sections in UI/runtime + backlog contracts |
| RB-BL012 | BL-012 | Track D | BL-012 tranche docs/evidence |
| RB-BL013 | BL-013 | Track D | `Documentation/plans/bl-013-hostrunner-feasibility-2026-02-23.md` |
| RB-BL026 | BL-026 | Track C | `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md` |
| RB-HX02 | HX-02 | Track F | BL-016 + synchronization contract surfaces |
| RB-HX06 | HX-06 | Track F | invariants + RT safety policy |
| RB-BL027 | BL-027 | Track C | `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md` |
| RB-BL028 | BL-028 | Track A/C | `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md` |
| RB-BL029 | BL-029 | Track B | `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md` |
| RB-HX05 | HX-05 | Track F | scene payload/transport contract docs |
| RB-BL030 | BL-030 | Track G | release + manual device acceptance surfaces |
| RB-BL017 | BL-017 | Track E | `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md` |
| RB-BL020 | BL-020 | Track E | backlog + perception overlay annex notes |
| RB-BL021 | BL-021 | Track E | backlog + room-story annex notes |
| RB-BL023 | BL-023 | Track C | resize hardening and BL-025 baseline surfaces |

## Procedural Runbooks

### RB-BL025
- Goal: close BL-025 with deterministic UX behavior and host-verified preset lifecycle confidence.
- Depends on: BL-019 and BL-022 staying green.
- Validate: production self-test lane, host preset spot-check lane, docs freshness gate.
- Exit artifact: updated BL-025 closeout evidence pointers in backlog + TestEvidence.

### RB-BL014
- Goal: finalize listener/speaker/aim/RMS overlay confidence lanes.
- Depends on: BL-015 and BL-016 baselines.
- Validate: smoke + acceptance suite + overlay-specific assertions.
- Exit artifact: BL-014 closeout evidence bundle with no unresolved high-severity regressions.

### RB-BL018
- Goal: promote spatial profile expansion from validation to done with strict baseline.
- Depends on: BL-014 stability.
- Validate: strict profile matrix lanes + docs freshness gate.
- Exit artifact: warning-free evidence row set and synchronized status claims.

### RB-BL022
- Goal: finalize choreography pack closure while BL-025 evolves.
- Depends on: BL-003 and BL-004 done baseline.
- Validate: choreography-specific lane + BL-025 regression guard rerun.
- Exit artifact: BL-022 done-ready evidence and drift notes (if any).

### RB-BL012
- Goal: complete tranche closeout while preserving HX-04 embedded parity guard.
- Depends on: none.
- Validate: BL-012 lane + HX-04 parity audit + docs freshness gate.
- Exit artifact: tranche status row and parity evidence pointers synchronized.

### RB-BL013
- Goal: decide BL-013 promotion from feasibility to sustained validation lane.
- Depends on: BL-012.
- Validate: feasibility script rerun (VST3 backend + skeleton fallback) and host test pass.
- Exit artifact: promotion recommendation and fresh status/report artifacts.

### RB-BL026
- Goal: implement CALIBRATE v2 slices A-E and move to validation.
- Depends on: BL-025 baseline and stable BL-009/BL-018 diagnostics.
- Validate: `UI-P1-026A..E` lane set + host/manual checks + docs freshness gate.
- Exit artifact: first BL-026 validation bundle and backlog row transition.

### RB-HX02
- Goal: audit registration lock and memory-order contract expectations.
- Depends on: BL-016 baseline.
- Validate: targeted concurrency regression checks + smoke/acceptance sanity reruns.
- Exit artifact: documented audit outcomes and any required code/doc fixes.

### RB-HX06
- Goal: enforce recurring RT-safety static audits in `processBlock()` call paths.
- Depends on: BL-016 baseline.
- Validate: static audit script, focused QA rerun, docs freshness gate.
- Exit artifact: structured status/report artifacts for RT-safety audit lane.

### RB-BL027
- Goal: implement first renderer-v2 slice aligned with BL-026 profile contracts.
- Depends on: BL-026 shared alias/diagnostics contracts.
- Validate: renderer v2 lanes + requested/active/stage parity checks.
- Exit artifact: BL-027 promotion evidence and synchronized backlog row.

### RB-BL028
- Goal: enforce spatial output matrix legality and domain exclusivity behavior.
- Depends on: BL-017, BL-026, BL-027 contract baselines.
- Validate: matrix legality checks + domain/status parity lanes + docs freshness.
- Exit artifact: matrix rule evidence and contract-surface synchronization.

### RB-BL029
- Goal: implement deterministic DSP visualization/tooling tranche (trace + spectral + reflection + calibration assistant).
- Depends on: BL-025/026/027/028 contract stability.
- Validate: `UI-P2-029A..E` lanes + schema/document synchronization checks.
- Exit artifact: BL-029 validation bundle and cross-surface schema updates.

### RB-HX05
- Goal: enforce scene payload budget and throttling contract for UI responsiveness.
- Depends on: BL-016 and BL-025 baseline.
- Validate: high-emitter stress lane + cadence/payload assertions + docs freshness.
- Exit artifact: payload budget evidence and contract threshold documentation.

### RB-BL030
- Goal: operationalize recurring release/device-rerun governance.
- Depends on: BL-024 baseline, BL-025 baseline, HX-06 active.
- Validate: manual device rerun checklist (`DEV-01..DEV-06`) + closeout sync checklist + docs freshness.
- Exit artifact: repeatable release-closeout record with evidence links and explicit `N/A` handling policy.

### RB-BL017
- Goal: deliver companion bridge slice-A without violating RT constraints.
- Depends on: BL-009 and BL-018 stability.
- Validate: synthetic bridge contract lane + non-regression checks on adjacent spatial lanes.
- Exit artifact: BL-017 in-validation evidence and bridge diagnostics publication proof.

### RB-BL020
- Goal: implement confidence/masking overlay with deterministic data mapping.
- Depends on: BL-014 and BL-019.
- Validate: overlay mapping assertions + UI responsiveness checks + docs freshness.
- Exit artifact: BL-020 planning-to-validation promotion evidence.

### RB-BL021
- Goal: implement adaptive room-story overlays on stable telemetry surfaces.
- Depends on: BL-014 and BL-015.
- Validate: overlay correctness checks + payload budget guard checks.
- Exit artifact: BL-021 validation evidence and payload contract compliance proof.

### RB-BL023
- Goal: harden resize/DPI behavior after BL-025 baseline finalization.
- Depends on: BL-025 baseline.
- Validate: host resize matrix checks + control interaction regression checks.
- Exit artifact: BL-023 validation evidence matrix and regression summary.
