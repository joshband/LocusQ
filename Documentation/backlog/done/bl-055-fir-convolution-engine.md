Title: BL-055 FIR Convolution Engine
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-19

# BL-055 FIR Convolution Engine

## Status
Done as a historical closeout. The original lane closed, and later truthfulness concerns were explicitly moved into follow-on work under BL-095.

## Plain-Language Summary
BL-055 integrated the FIR engine path into headphone monitoring and established the original latency, swap, and parity contract. The important current truth is narrower than the old packet: this runbook is the historical closeout, while BL-095 carries the unresolved truth-render and objective-validation follow-up.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | FIR engine integration, latency reporting, and swap/parity contract. |
| Why | Enabled direct vs partitioned convolution in the monitoring path with deterministic validation. |
| Who | DSP maintainers, QA owners, and release reviewers protecting realtime behavior. |
| When | Historical closeout retained; follow-on truthfulness concerns were redirected on 2026-03-17. |
| Where | [`Documentation/backlog/done/bl-055-fir-convolution-engine.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-055-fir-convolution-engine.md), `TestEvidence/...`, and follow-on runbook `Documentation/backlog/bl-095-partitioned-fir-truthfulness-recovery-and-objective-validation.md`. |
| How | FirEngineManager integration, latency/crossfade checks, and offline parity references. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Historical contract and follow-on boundary | This runbook |
| Detailed lane history | Full remediation and replay chronology | Archived legacy copy |

## Core Contract
- Processing is direct-form only; `getLatencySamples()` returns 0 for all tap counts (BL-095 Slice B, 2026-03-19).
- Engine swaps must stay click-safe; crossfade infrastructure remains in place.
- `FirEngineManager` engine-selection bookkeeping is retained for future use but does not affect latency reporting.
- BL-095 closed Slice B (runtime honesty) and carries any remaining objective-validation follow-up.

## Key Gates
- Historical contract and execute packets passed.
- Latency and swap-crossfade markers were present in the closeout lane.
- Follow-on truthfulness gap was explicitly documented instead of hidden.

## Evidence Pointers
| Signal | Path |
|---|---|
| Owner intake and follow-up packets | `TestEvidence/bl055_owner_*` |
| Validation trend and governance summary | `TestEvidence/validation-trend.md`, `TestEvidence/build-summary.md` |
| Active follow-on lane | `Documentation/backlog/bl-095-partitioned-fir-truthfulness-recovery-and-objective-validation.md` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| FIR engine integration | Done | Historical closeout retained. |
| Latency and swap validation | Done | Structural and execute evidence captured. |
| Truthfulness follow-on | Open elsewhere | BL-095 carries the remaining gap. |

## Archive Note
Full historical material is preserved at [`bl-055-fir-convolution-engine-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-055-fir-convolution-engine-legacy.md).
