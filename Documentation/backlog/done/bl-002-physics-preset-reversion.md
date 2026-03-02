Title: BL-002 Physics Preset Host Reversion Fix
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02

# BL-002: Physics Preset Host Reversion Fix

## Plain-Language Summary

This runbook tracks **BL-002** (BL-002: Physics Preset Host Reversion Fix). Current status: **Done**. In plain terms: Fixed physics engine state reverting to defaults when host reloaded presets, ensuring parameter persistence across save/load cycles.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-002: Physics Preset Host Reversion Fix |
| Why is this important? | Fixed physics engine state reverting to defaults when host reloaded presets, ensuring parameter persistence across save/load cycles. |
| How will we deliver it? | Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-002-physics-preset-reversion.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Promotion gate table | Shows what passed/failed for closeout decisions. | `## Promotion Gate Summary` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| Priority | P0 |
| Status | Done |
| Completed | 2026-02-21 |
| Owner Track | B Scene/UI Runtime |

## Objective

Fixed physics engine state reverting to defaults when host reloaded presets, ensuring parameter persistence across save/load cycles.

## What Was Built

- Corrected preset serialization for physics parameters
- Validated host state persistence

## Key Files

- `Source/PluginProcessor.cpp`
- `Source/PhysicsEngine.h`

## Evidence References

- Production self-test baseline (part of initial P0 closeout cycle)
- `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md` entries

## Completion Date

2026-02-21


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-002-physics-preset-reversion.md` |

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
