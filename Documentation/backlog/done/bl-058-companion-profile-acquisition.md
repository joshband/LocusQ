Title: BL-058 Companion Profile Acquisition UI + HRTF Matching
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-18

# BL-058 Companion Profile Acquisition UI + HRTF Matching

## Status
Done. Z1 owner sync passed on 2026-03-17, execute and self-test evidence passed, privacy gates stayed clean, and the send gate closed correctly until readiness conditions were met.

## Plain-Language Summary
BL-058 delivered the companion-side acquisition flow for personalized headphone profiles. It added guided ear-photo capture, deterministic nearest-neighbor baseline matching, local-only privacy behavior, and readiness/sync gating so the companion publishes profile data only from a known-good state.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Guided companion profile capture and baseline HRTF subject matching. |
| Why | Established a truthful, privacy-bounded path for personalized profile acquisition before later calibration handoff work. |
| Who | Companion operators, headphone users, QA owners, and personalization/runtime maintainers. |
| When | Done on 2026-03-17 after Z1 owner sync, execute lane `16/16`, and self-test `7/7` all passed. |
| Where | [`Documentation/backlog/done/bl-058-companion-profile-acquisition.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-058-companion-profile-acquisition.md) and `TestEvidence/...`. |
| How | Guided capture UI, deterministic SADIE II nearest-neighbor baseline, explicit readiness states, and promotion evidence. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final contract, gates, and evidence map | This runbook |
| UI/runtime detail | Full capture, matching, and validation history | Archived legacy copy |

## Core Contract
- Capture flow requires left ear, right ear, and frontal views.
- Matching stays deterministic and local-only.
- Images are not retained after embedding/match processing.
- Readiness and send-gate behavior stay explicit and testable.
- Axis-sweep and stale-pose checks remain part of the truthfulness contract.

## Key Gates
- Execute lane passes end-to-end.
- Self-test passes.
- Privacy contract shows no network or retained-image leakage.
- Matching latency stays within accepted bounds.
- Owner sync closes the promotion packet cleanly.

## Evidence Pointers
| Signal | Path |
|---|---|
| Z1 owner sync packet | `TestEvidence/bl058_owner_sync_z1_20260317T042803Z_5881/` |
| Manual runtime packet family | `TestEvidence/bl058_manual_runtime_<timestamp>/` |
| Validation trend and governance summary | `TestEvidence/validation-trend.md`, `TestEvidence/build-summary.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Capture UI | Done | Guided left/right/frontal flow shipped. |
| HRTF baseline match | Done | Deterministic nearest-neighbor baseline added. |
| Privacy and readiness gates | Done | Local-only and send-gate rules held. |
| Owner promotion | Done | Z1 owner sync passed. |

## Archive Note
Full historical material is preserved at [`bl-058-companion-profile-acquisition-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-058-companion-profile-acquisition-legacy.md).
