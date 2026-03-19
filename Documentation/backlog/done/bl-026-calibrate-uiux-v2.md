Title: BL-026 CALIBRATE UI/UX V2 Multi-Topology
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# BL-026 CALIBRATE UI/UX V2 Multi-Topology

## Status
Done. Owner promotion sync completed on 2026-02-25.

## Plain-Language Summary
BL-026 redesigned CALIBRATE into a multi-topology workflow with profile selection, dynamic topology rendering, profile library behavior, diagnostics, and host-resize stability. The key outcome is that CALIBRATE became the coherent foundation that BL-027, BL-028, and BL-029 built on.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | CALIBRATE UI/UX v2 multi-topology redesign. |
| Why | Replaced the fixed topology workflow with a broader, profile-driven calibration surface. |
| Who | Operators, UI/runtime maintainers, QA, and downstream renderer/calibration work. |
| When | Done on 2026-02-25 with owner promotion sync complete. |
| Where | [`Documentation/backlog/done/bl-026-calibrate-uiux-v2.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-026-calibrate-uiux-v2.md), annex spec, and `TestEvidence/...`. |
| How | Topology selector, dynamic speaker rows, profile library, diagnostics cards, and host integration validation. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Slice history | Full A-E execution detail | Archived legacy copy |

## Core Outcome
- CALIBRATE now supports multi-topology monitoring profiles.
- Dynamic speaker presentation and profile library behavior became first-class.
- Validation diagnostics became explicit instead of implied.
- The panel became the canonical UX base for later renderer and calibration improvements.

## Key Gates
- Slice A-E delivery completed.
- Host integration and resize behavior were validated.
- Promotion packet and owner sync recorded the final `Done` posture.
- Downstream blockers for BL-027, BL-028, and BL-029 were removed.

## Evidence Pointers
| Signal | Path |
|---|---|
| Annex spec | `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md` |
| Evidence family | `TestEvidence/bl026_*` |
| Historical closeout detail | archived legacy copy |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Slice A | Done | Topology selector and alias dictionary landed. |
| Slice B | Done | Dynamic speaker rows landed. |
| Slice C-D | Done | Profile library and diagnostics landed. |
| Slice E | Done | Host integration and resize regression closure. |

## Archive Note
Full historical material is preserved at [`bl-026-calibrate-uiux-v2-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-026-calibrate-uiux-v2-legacy.md).
