Title: BL-025 EMITTER UI/UX V2 Deterministic Closeout
Document Type: Backlog Runbook (Closeout)
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02

# BL-025: EMITTER UI/UX V2 Deterministic Closeout

## Plain-Language Summary

This runbook tracks **BL-025** (BL-025: EMITTER UI/UX V2 Deterministic Closeout). Current status: **Done**. In plain terms: Completed full EMITTER panel redesign with 5 implementation slices (A-E): parameter rail restructure, emitter selector, directivity/velocity controls, preset lifecycle, resize behavior.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | BL-025: EMITTER UI/UX V2 Deterministic Closeout |
| Why is this important? | Completed full EMITTER panel redesign with 5 implementation slices (A-E): parameter rail restructure, emitter selector, directivity/velocity controls, preset lifecycle, resize behavior. |
| How will we deliver it? | Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/bl-025-emitter-uiux-v2.md` plus repo-local evidence under `TestEvidence/...`. |

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
| Priority | P1 |
| Status | Done |
| Completed | 2026-02-24 |
| Owner Track | Track C UX Authoring |

## Objective

Completed full EMITTER panel redesign with 5 implementation slices (A-E): parameter rail restructure, emitter selector, directivity/velocity controls, preset lifecycle, resize behavior. Annex: `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`.

## What Was Built

- Redesigned IA with 6 control sections
- Emitter instance selector
- Directivity azimuth/elevation controls
- Initial velocity vector controls
- Preset save/load with host path fix
- Resize behavior with overflow handling

## Key Files

- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `Source/PluginEditor.cpp`
- `Source/PluginProcessor.cpp`

## Evidence References

- `TestEvidence/locusq_production_p0_selftest_20260224T032239Z.json`
- `TestEvidence/reaper_headless_render_20260224T032300Z/status.json`
- Manual resize QA at `Documentation/testing/bl-025-emitter-resize-manual-qa-2026-02-23.md`

## Completion Date

2026-02-24


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-025-emitter-uiux-v2.md` |

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
