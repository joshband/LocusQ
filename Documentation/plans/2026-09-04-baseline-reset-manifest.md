Title: Baseline Reset Manifest
Document Type: Implementation Plan
Author: Claude
Created Date: 2026-09-04
Last Modified Date: 2026-09-04

# Baseline Reset Manifest

## Purpose

Working list of every archive/delete/consolidate/fix action identified by the
2026-09-04 engineering audit (DSP, architecture/build, WebView UI, CI/CD, and
agentic-process passes, plus a follow-up gap-filling pass covering licensing,
sanitizers, Windows CI, companion tests, and `Design/`). Nothing in this
manifest has been executed yet — it is the review artifact for sequencing that
work in small, independently-verifiable batches, per the project's own
small-batch preference.

Each batch is scoped to be a single reviewable commit/PR. Batches are ordered
low-risk-and-reversible first. None of them touch the Boids/Attractor/
Collision/Choreography-Lab scope question — that is a product decision
(see the audit's "Decision" section) and is deliberately excluded from this
manifest until resolved.

Status legend: `PENDING` (not started) / `IN PROGRESS` / `DONE`.

---

## Batch A — Delete unreachable UI prototype scaffolding

**Risk: low.** Nothing in this batch is reachable from the shipping build.
Git history preserves all of it.

| Action | Path | Why |
|---|---|---|
| DELETE | `Source/ui/public/incremental/index_stage{2..11}.html` | Superseded, monotonically-additive prototypes; DOM contract already ported into `Source/ui/src/index.ts`. |
| DELETE | `Source/ui/public/incremental/js/stage{2..11}_ui.js` | Same. |
| DELETE | `Source/ui/public/incremental/index_stage13.html`, `js/stage13_ui.js` | Orphaned: no `BinaryData` embed, no route in `EditorWebViewRuntime.h`. Unreachable by any build. |
| DECIDE THEN ACT | `Source/ui/public/incremental/index_stage12.html`, `js/stage12_ui.js` | Only stage still wired to `LOCUSQ_UI_POC`. Keep only if the debug/incremental route is still wanted operationally; otherwise delete with the rest. |
| DELETE | `Source/ui/public/poc/**` (`index_poc.html`, `js/poc_ui.js`) | Same gated-but-unreferenced pattern as `incremental/`. |
| EDIT | `CMakeLists.txt:242-267` (`LOCUSQ_UI_POC` `BinaryData` embed list) | Remove entries for deleted files; remove the option entirely if stage12 also goes. |
| EDIT | `Source/editor_webview/EditorWebViewRuntime.h:88-96,628-766` | Remove the WebView resource routes for deleted stages/poc. |
| DELETE | `scripts/standalone-ui-selftest-stage{4..12}-mac.sh` | Historical build-evidence scripts for prototypes that no longer exist. Keep stage12's script only if stage12 is kept. |
| FIX | `status.json` (~lines 1405-1413) | Remove/correct the `incremental_stage13_ui_selftest_passed: true` and pluginval-pass claims — this is currently a false attestation for code the build cannot reach. |

**Verification:** full `cmake configure` + `cmake --build` for the WebView-enabled target after deletion; confirm `Source/ui/public/index.html` still loads with no missing-asset errors.

---

## Batch B — Archive superseded design packages ✅ DONE (2026-09-04)

**Risk: low.** `Design/HANDOFF.md` already names v3 as the sole approved
package; v1/v2 were dead weight, not history anyone builds from.

| Action | Path | Why |
|---|---|---|
| ARCHIVED → `Documentation/archive/2026-09-04-baseline-reset/design-v1-v2/` | `Design/v1-style-guide.md`, `Design/v1-ui-spec.md`, `Design/v1-test.html` | Superseded by v3 per `Design/HANDOFF.md`. |
| ARCHIVED → same | `Design/v2-style-guide.md`, `Design/v2-ui-spec.md`, `Design/v2-test.html` | Same. |
| KEPT | `Design/v3-*`, `Design/HANDOFF.md`, `Design/index.html` | Canonical, current. |
| ~~FIX reference~~ | `.codex/skills/design/SKILL.md`, `.codex/workflows/design.md` | **Correction on execution:** these only describe the design skill's own "always do v1→v2→v3" iteration process for *future* tasks — not path references to these specific files. No fix needed. |
| ~~FIX reference~~ | `.ideas/physics-simulation-impl-plan.md` | **Correction on execution:** the actual match was `Design/physics-v1-ui-spec.md` / `Design/physics-v1-style-guide.md` — different, never-created files, unrelated to this archival. Left alone. |

**Verification:** confirmed no build/script/source reference to `Design/v1`/`Design/v2` before moving; `git mv` used to preserve history; archive documented in `Documentation/archive/2026-09-04-baseline-reset/README.md`.

---

## Batch C — Consolidate the CMake physics-probe duplication

**Risk: medium — touches the build file everyone depends on.** Do this as
its own PR, verified by a full local build, not folded into a docs/archive
batch.

| Action | Detail |
|---|---|
| REFACTOR | Replace the 19 near-identical `juce_add_console_app` probe blocks (`CMakeLists.txt:815-2699`) with one parametrized CMake function taking probe name + source list. |
| FIX | Remove the duplicate `Source/BeatSyncSystem.cpp` entry in the main target's `target_sources` (`CMakeLists.txt:352,356`). |
| DECIDE | Register more of the ~20 QA probe targets with `ctest` (currently only 5 of ~20 are, `CMakeLists.txt:2681-2719`) — or confirm the "may depend on runtime libraries not present everywhere" reasoning still holds per-probe. |

**Verification:** clean configure + build on both the CI-supported OSes; confirm all 20 probe binaries still produce identical output/behavior to pre-refactor (diff a captured run before/after).

---

## Batch D — Prune TestEvidence

**Risk: low.** Evidence is a historical record, not live state; nothing
reads these dirs at runtime.

| Action | Detail |
|---|---|
| KEEP | `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md` (curated summaries — but see Batch F, these have re-bloated once already and need a budget, not just a one-time trim). |
| ARCHIVE → `Documentation/archive/2026-09-04-baseline-reset/test-evidence/` | All timestamped run dirs under `TestEvidence/` **except** the single most recent dir per BL-item prefix. (72 dirs today; expect this to cut it to roughly the number of distinct BL items with evidence, likely ~40-50.) |
| DECIDE | Whether "most recent per BL item" or "most recent per BL item that is still an open/active item" is the right retention rule — closed items arguably don't need even one dir kept live. |

**Verification:** none needed beyond confirming no script or workflow references a specific archived path by name (`grep -r "TestEvidence/<dirname>"` before archiving each).

---

## Batch E — Rewrite status.json to current-state-only

**Risk: medium — this file is read by humans and by agent workflow logic.**
Sequence after A-D so the "notes" field isn't still describing files that no
longer exist.

| Action | Detail |
|---|---|
| SNAPSHOT | Copy current `status.json` verbatim to `Documentation/archive/2026-09-04-baseline-reset/status.json.pre-reset-snapshot` before editing anything. |
| REWRITE | `status.json` keeps: `plugid_name`, `version`, `current_phase`, `ui_framework`, `complexity_score`, timestamps, and a short `notes` describing only the currently-active blockers — not a chronological log of every closed BL item. |
| MOVE history | Per-item `blXXX_..._status/_script/_evidence/_result` keys move to the existing `Documentation/backlog/done/BL-XXX.md` files (many already exist there) rather than staying in status.json. status.json should *point at* the backlog, not duplicate it. |
| REFRESH | `TODO.md` — either regenerate from actual current state (cross-check against `Documentation/backlog/index.md` and `CHANGELOG.md`), or retire it in favor of the backlog index if it's redundant once status.json is trustworthy. |

**Verification:** confirm every `_evidence` pointer that moves still resolves to a real path after Batch D's pruning; confirm nothing outside status.json parses the removed keys (`grep -r "bl[0-9]\+_.*_status" scripts .github` for programmatic readers before removing).

---

## Batch F — Skill catalog consolidation

**Risk: low — additive-safe, but changes agent-facing routing, so verify
triggering still works.**

| Action | Detail |
|---|---|
| FOLD | The 10 alias-router skills in `.claude/skills/` (`skill_debug`, `skill_design`, `skill_docs`, `skill_dream`, `skill_impl`, `skill_plan`, `skill_ship`, `skill_test`, plus the `juce-webview`/`juce-webview-windows`/`skill_design_webview` 3-way split) into their real targets — either delete the alias and rely on the target skill's own description matching, or fold the routing into `SKILLS.md`'s existing matrix instead of one file per alias. |
| MERGE | Single-ticket skills (`auv3-plugin-lifecycle`, `clap-plugin-lifecycle`, `headtracking-companion-runtime`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`, `apple-spatial-companion-platform`) into broader domain skills (e.g. one `plugin-format-lifecycle` skill covering AUv3+CLAP, one `spatial-audio-validation` skill covering HRTF/perceptual/companion validation) — keep the specific BL-### knowledge as content *inside* the merged skill, not as the skill's identity. |
| BUDGET | Cap `.codex/skills/*/SKILL.md` at ~150-250 always-loaded lines; move anything longer to a referenced doc loaded on demand. |

**Verification:** exercise a handful of real trigger phrases per merged skill against the description text to confirm routing still works before deleting the originals.

---

## Batch G — Governance-doc de-duplication

**Risk: low.**

| Action | Detail |
|---|---|
| KEEP canonical | `AGENT_RULE.md` (already the declared source of truth, and its sync to `.codex/rules/agent.md` / `.claude/rules/agent.md` is confirmed byte-identical — no drift to fix). |
| THIN | `CLAUDE.md`, `CODEX.md`, `AGENTS.md` — reduce each to routing/pointer content specific to that agent surface, removing restated priority-order/phase-discipline/validation-vocabulary text that's already in `AGENT_RULE.md`. |
| LICENSE | Add a root `LICENSE` file and confirm JUCE licensing mode (GPLv3 vs. commercial) in a visible place — currently absent entirely, which is a real gap for a project fetching the Steam Audio SDK and depending on JUCE. |

**Verification:** none beyond a read-through confirming no rule is lost, only de-duplicated.

---

## Explicitly out of scope for this manifest

- **BoidsSystem / AttractorSystem / CollisionSystem / Choreography Lab** — contradicts the shipped V1 creative brief per the DSP audit, but keep-vs-cut is a product decision, not a cleanup. Tracked separately.
- **Sanitizer (ASan/TSan/UBSan) integration, Windows CI runner addition, companion `swift test` wiring** — these are new CI capability, not archival; tracked as their own follow-up work once Batch A-G land, so the baseline is stable before adding new gates on top of it.
- **PhysicsEngine/PhysicsWorker consolidation, processor_core/processor_bridge cleanup** — real-time code changes requiring the physics acceptance suite; separate PRs per the original audit's Phase 3.
