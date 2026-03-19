Title: BL-027 Renderer UX v2
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-18

# BL-027 Renderer UX v2

## Status
Done. Promoted via Slice F packet on 2026-02-25.

## Plain-Language Summary
BL-027 redesigned the RENDERER panel into a profile-authoritative, diagnostics-rich surface with cross-panel coherence to CALIBRATE v2. The important result is that profile state, output presentation, and renderer diagnostics became consistent and promotion-ready only after slice-by-slice validation and final owner packet review.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Renderer UX v2. |
| Why | Replaced static renderer presentation with live profile, layout, and diagnostics context. |
| Who | UI/runtime maintainers, operators, QA, and future CALIBRATE/RENDERER integration work. |
| When | Done on 2026-02-25 via Slice F promotion packet. |
| Where | [`Documentation/backlog/done/bl-027-renderer-uiux-v2.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-027-renderer-uiux-v2.md), annex spec, and `TestEvidence/...`. |
| How | Profile-authority UI shell, dynamic speaker/output presentation, diagnostics cards, coherence fixes, and final promotion evidence. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Slice history | Full A-F execution detail | Archived legacy copy |

## Core Outcome
- RENDERER became profile-authoritative instead of static.
- Output and speaker presentation adapted to live layout state.
- Steam and Ambisonic diagnostics became first-class cards.
- CALIBRATE and RENDERER profile coherence was made deterministic.

## Key Gates
- Slice A owner replay cleared the worker instability.
- Slices B-E passed their scoped validation bundles.
- Slice F promotion packet compiled the authoritative evidence matrix and recorded `GO`.

## Evidence Pointers
| Signal | Path |
|---|---|
| Slice A authoritative replay | `TestEvidence/owner_bl027_slice_a_replay_20260225T175000Z/` |
| Slice B | `TestEvidence/bl027_slice_b_impl_20260225T175738Z/` |
| Slice C | `TestEvidence/bl027_slice_c_20260225T180311Z/` |
| Slice D | `TestEvidence/bl027_slice_d_20260225T181011Z/` |
| Slice E | `TestEvidence/bl027_slice_e_20260225T182057Z/` |
| Slice F promotion packet | `TestEvidence/bl027_done_promotion_slice_f_20260225T205629Z/` |
| Annex spec | `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Slice A | Done | Renderer v2 shell with authoritative owner replay. |
| Slice B-D | Done | Dynamic outputs and diagnostics cards landed. |
| Slice E | Done | Cross-panel coherence fixed. |
| Slice F | Done | Promotion packet recorded final `GO`. |

## Archive Note
Full historical material is preserved at [`bl-027-renderer-uiux-v2-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-027-renderer-uiux-v2-legacy.md).
