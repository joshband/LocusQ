Title: BL-023 Resize/DPI Hardening
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-03-18

# BL-023 Resize/DPI Hardening

## Status
Done. Historical pre-closeout states are preserved in the archived runbook and evidence packets.

## Plain-Language Summary
BL-023 locked the deterministic resize and DPI behavior contract for the WebView UI across standalone and plugin-host windows. The main result is simple: breakpoints, DPI scaling, resize cadence, and host-matrix behavior now have explicit pass/fail rules and reproducible evidence.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Resize and DPI hardening contract plus host-matrix evidence. |
| Why | Prevents clipping, stale hit-target maps, and host-specific resize regressions from staying implicit. |
| Who | UI/runtime maintainers, QA, and release owners. |
| When | Done; historical closeout detail is preserved in archive. |
| Where | [`Documentation/backlog/done/bl-023-resize-dpi-hardening.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-023-resize-dpi-hardening.md), QA contract, and `TestEvidence/...`. |
| How | Explicit breakpoint/DPI contracts, deterministic taxonomy, matrix harness, and promotion evidence. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Final result and evidence map | This runbook |
| Host-matrix detail | Full contract and replay history | Archived legacy copy |

## Core Outcome
- Breakpoint, bounds, DPI, and resize-settle contracts became explicit.
- Host regression coverage was defined for standalone, REAPER, Logic, and Ableton.
- Deterministic taxonomy and replay semantics became part of the closeout surface.
- Release-governance visibility now depends on a documented resize/DPI contract instead of ad-hoc checks.

## Key Gates
- Contract and runtime matrix lanes were defined and exercised.
- Deterministic replay signatures and row counts stayed stable.
- Required machine-readable artifacts were produced.
- Docs freshness passed at closeout.

## Evidence Pointers
| Signal | Path |
|---|---|
| QA contract | `Documentation/testing/bl-023-resize-dpi-hardening-qa.md` |
| Evidence family | `TestEvidence/bl023_*` |
| Historical closeout detail | archived legacy copy |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| A1 | Done | Contract, taxonomy, and matrix definition established. |
| A2 | Done | Runtime/UI hardening landed. |
| C1/C2 | Done | Harness and determinism rules stabilized. |
| Promotion | Done | Evidence and closeout sync completed. |

## Archive Note
Full historical material is preserved at [`bl-023-resize-dpi-hardening-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-023-resize-dpi-hardening-legacy.md).
