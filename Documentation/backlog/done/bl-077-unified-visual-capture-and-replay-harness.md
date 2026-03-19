Title: BL-077 Unified Visual Capture and Replay Harness
Document Type: Backlog Done Runbook
Author: APC Codex
Created Date: 2026-03-02
Last Modified Date: 2026-03-18

# BL-077 Unified Visual Capture and Replay Harness

## Status
Done. Contract, execute, live, T2, and T3 evidence all passed, and the harness closeout/archive sync was recorded.

## Plain-Language Summary
BL-077 created the reusable visual capture lane for plugin and companion workflows. It standardized guided capture, checkpoint cues, dense frames, contact sheets, clips, and machine-readable summaries so later trust-wave lanes could reuse one evidence shape instead of inventing their own.

## 6W Snapshot (Who/What/Why/How/When/Where)
| Question | Answer |
|---|---|
| What | Unified capture and replay harness for plugin plus companion evidence. |
| Why | Increased QA evidence speed, consistency, and reviewability for later visual lanes. |
| Who | QA owners, companion operators, release reviewers, and automation consumers. |
| When | Done; closeout and archive sync recorded on 2026-03-03. |
| Where | [`Documentation/backlog/done/bl-077-unified-visual-capture-and-replay-harness.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-077-unified-visual-capture-and-replay-harness.md) and `TestEvidence/...`. |
| How | Profile-driven capture runs, deterministic post-processing, and owner-ready evidence packets. |

## Visual Aid Index
| Type | Purpose | Source |
|---|---|---|
| Table | Outcome, artifact contract, and evidence map | This runbook |
| Capture flow diagram | Detailed guided-capture pipeline | Archived legacy copy |

## Core Contract
- One-command capture runs must produce a deterministic artifact tree.
- Cue/checkpoint behavior must stay profile-driven and reproducible.
- Output packets must stay reviewable by both humans and scripts.
- Later lanes may reuse the harness contract without redefining the schema.

## Key Gates
- Guided capture runs complete reliably.
- Post-processing emits the expected artifact set.
- T2 and T3 owner packets pass with stable evidence paths.
- Closeout sync preserves the harness as a reusable platform lane.

## Evidence Pointers
| Signal | Path |
|---|---|
| Final promotion packet | `TestEvidence/bl077_capture_harness_<timestamp>/` |
| Capture contract matrix | `TestEvidence/bl077_capture_harness_<timestamp>/capture_contract_matrix.tsv` |
| Artifact schema inventory | `TestEvidence/bl077_capture_harness_<timestamp>/artifact_schema_inventory.tsv` |

## Milestone Snapshot
| Milestone | Result | Note |
|---|---|---|
| Core capture CLI | Done | Deterministic guided run established. |
| Post-processing packager | Done | Frames, sheets, clips, and indexes standardized. |
| Lane integration | Done | Active QA lanes can consume the harness contract. |
| Owner promotion | Done | T2/T3 evidence completed and synced. |

## Archive Note
Full historical material is preserved at [`bl-077-unified-visual-capture-and-replay-harness-legacy.md`](/Users/artbox/Documents/Repos/LocusQ/Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-077-unified-visual-capture-and-replay-harness-legacy.md).
