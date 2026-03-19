Title: BL-030 Release Governance and Device Rerun
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-03-18

# BL-030 Release Governance and Device Rerun

## Status
Done. Historical pre-closeout gate states remain in execution snapshots. The active result is a repeatable release/device-rerun governance contract with recorded evidence.

## Plain-Language Summary
BL-030 operationalized a recurring release checklist and device-rerun matrix for shipping decisions. The core outcome is simple: every release candidate now has an explicit gate order, device-profile coverage, and evidence expectation before ship.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Release checklist and device-rerun governance contract. |
| Why | Prevents ad-hoc ship decisions and implicit N/A handling. |
| Who | QA owners, release owners, and operators preparing ship decisions. |
| When | Done; closeout evidence retained in historical packets. |
| Where | [`Documentation/backlog/done/bl-030-release-governance.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-030-release-governance.md) and `TestEvidence/...`. |
| How | Ordered release gates, device-profile reruns, CI support, and explicit evidence paths. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Release governance summary and evidence map | This runbook |
| Slice detail | Full slice-by-slice execution history | Archived legacy copy |

## Core Contract
- Release gates must run in a fixed order.
- Device reruns cover the defined reference profiles.
- N/A handling must be explicit and justified.
- CI can automate the repeatable gates, but manual device validation remains visible.
- Evidence must be repo-local and tied to ship-readiness decisions.

## Key Gates
- Release checklist template exists and stays canonical.
- Device rerun matrix covers the scoped device profiles.
- Automated release-gate checks are wired into the validation path.
- Baseline execution evidence exists for the first governed run.

## Evidence Pointers
| Signal | Path |
|---|---|
| BL-030 QA contract | `Documentation/testing/bl-030-release-governance-qa.md` |
| Release-governance evidence family | `TestEvidence/bl030_release_governance_*/` |
| Canonical release/device docs | `Documentation/runbooks/release-checklist-template.md`, `Documentation/runbooks/device-rerun-matrix.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A | Done | Release checklist template defined. |
| B | Done | Device rerun matrix defined. |
| C | Done | CI release-gate integration documented. |
| D | Done | Baseline execution evidence captured. |

## Archive Note
Full historical material is preserved at [`bl-030-release-governance-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-030-release-governance-legacy.md).
