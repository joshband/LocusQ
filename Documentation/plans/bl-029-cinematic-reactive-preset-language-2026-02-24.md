Title: BL-029 Cinematic Reactive Preset Language
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-03-18

# BL-029 Cinematic Reactive Preset Language

## Status

Approved. This is the active pre-code contract for BL-029 Slice G6.
Legacy detail copy:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/plans/bl-029-cinematic-reactive-preset-language-2026-02-24-legacy.md`

## Goal

Define a deterministic cinematic preset language before any more BL-029 expansion lands.
The active contract must keep preset naming, reactive behavior, and QA acceptance stable across WebView runtimes.

## Scope

- Freeze preset-family vocabulary and mode mapping.
- Define additive reactive-envelope fields and fallback behavior.
- Define cinematic authoring tokens for family, mood, kinetic style, and fade style.
- Keep rain and snow semantically distinct.
- Keep runtime IDs stable. Do not rename existing signal or motion IDs.

## Core Contracts

| Area | Contract |
|---|---|
| Authority | No new cinematic preset code expansion before G6 approval. |
| Determinism | Same family, seed, and mode must resolve to the same profile tokens and replay hash. |
| Fallback | Missing reactive blocks must fall back to `rendererAuditionReactivity` and an explicit status string. |
| Safety | All reactive values must stay finite and clamped. No audio-thread-unsafe expansion is allowed. |
| Parity | Backend notes must cover `WKWebView` and `WebView2` before v3 sign-off. |

## Delivery Order

### v1

- Freeze lexicon and preset-to-mode mapping.
- Preserve rain vs snow semantics.
- Require deterministic replay for stable modes.

### v2

- Add the reactive envelope contract.
- Define clamp, smoothing, and hysteresis rules.
- Keep fallback deterministic when the reactive block is absent.

### v3

- Add cinematic tokens for family, mood, kinetic, and fade.
- Require QA IDs and parity notes before authoring expansion.
- Lock the final mapping table as the source of truth for preset semantics.

## Validation Plan

### Required IDs

- `G6-V1-01`
- `G6-V1-02`
- `G6-V1-03`
- `G6-V2-01`
- `G6-V2-02`
- `G6-V2-03`
- `G6-V2-04`
- `G6-V3-01`
- `G6-V3-02`
- `G6-V3-03`
- `G6-V3-04`

### Evidence

- `TestEvidence/bl029_audition_reactive_qa_slice_g3_<timestamp>/`
- `TestEvidence/bl029_cinematic_reactive_preset_language_slice_g6_<timestamp>/`

### Gate Checks

- preset-family mapping completeness
- finite reactive scalars and bounded ranges
- explicit fallback when reactive block is missing
- deterministic replay hash stability
- per-check QA diagnostics with seed evidence

## Risks

- Preset semantics can drift between renderer and UI.
- Reactive fields can become too coupled to frame timing.
- Rain and snow can converge visually if opacity and fade rules are not enforced.
- Backend parity can diverge if WebView assumptions are not documented.

## Visual Aid Index

| Artifact | Use |
|---|---|
| Legacy archive copy | Full tables, semantic notes, and threshold detail. |
| Acceptance IDs | Machine-checkable G6 gate contract. |
| QA evidence packets | Deterministic replay and fallback proof. |

## Archive Note

The original long-form plan is preserved in the archive copy above.
Use this active file for pre-code decisions and the archive file for deep reference.
