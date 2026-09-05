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

## Batch C — Consolidate the CMake physics-probe duplication ✅ DONE (2026-09-04, pending CI confirmation)

**Risk: medium — touches the build file everyone depends on.**

| Action | Detail |
|---|---|
| **Correction on execution:** | Only **14** of the ~19-20 probe blocks (not all of them) share the full heavy pattern worth deduplicating — verified by programmatic pairwise diffing, not eyeballing. The other 6 ("Group B": `locusq_physics_probe`, `locusq_physics_tier_a_probe`, `locusq_physics_daw_automation_probe`, `locusq_fir_truthfulness_probe`, `locusq_timeline_track_type_probe`, and `locusq_qa` itself) have genuinely different, smaller source lists and no Steam Audio guard/`JucePlugin_*` macros — folding them into the same helper would have silently changed their behavior. Left untouched. |
| DONE | Replaced the 14 "Group A" blocks with one `locusq_add_qa_probe(target product_name main_cpp)` function + 14 one-line calls. |
| DONE | Removed the duplicate `Source/BeatSyncSystem.cpp` entry in the main `LocusQ` target's `target_sources` (was line 318, out of place before `FormationSystem.h`; the correctly `.h`-paired occurrence stays). |
| FLAGGED, not fixed | `locusq_qa`'s own `target_sources` has the *same* duplicate-file bug for `Source/BakeRecorder.h`/`.cpp` — out of the scope given (main target's `BeatSyncSystem.cpp` specifically). Separate follow-up decision. |
| NOT changed | `ctest` registration (still exactly the same 5 Group-B targets) — confirmed untouched by construction, the replaced spans don't overlap it. |

**Result:** 2684 → 1058 lines (−1626, ~61%). Verified before applying: current file matched the draft's assumed line numbers exactly (untouched since the audit). Verified after applying: exactly 1 `function()`/`endfunction()` pair, all 14 call sites present with unique target names, `ctest` block byte-identical, `locusq_physics_probe` (Group B) untouched, balanced parens/quotes across the whole file (structural check). No JUCE build environment available in this session to run a real `cmake` configure — **CI (`qa-critical` job on the open PR) is the real verification**, pending.

---

## Batch D — Prune TestEvidence ✅ DONE (2026-09-04)

**Risk: low.** Evidence is a historical record, not live state; nothing
reads these dirs at runtime.

| Action | Detail |
|---|---|
| KEPT | `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, `README.md`, and 44 singleton/most-recent-per-lane run dirs. |
| ARCHIVED → `Documentation/archive/2026-09-04-baseline-reset/test-evidence/` | 22 of 27 older-duplicate timestamped run dirs. |
| **Correction on execution:** | 5 of the original 27 candidates were NOT archived — each is individually cited by exact path as the sole evidence for a distinct test mode/phase (contract vs. execute, T1 vs. T2) in a live doc or `status.json` field, not a stale duplicate. Re-classified as "keep" instead of updating their citations. Full list in `Documentation/archive/2026-09-04-baseline-reset/README.md`. |

**Verification:** every one of the 22 moved paths independently grep-checked (`status.json`, `Documentation/`, `scripts/`, `.github/`) with zero live references found; one soft reference from inside an already-archived legacy doc was accepted as low-priority.

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

## Batch F — Skill catalog consolidation — ⏸ SKIPPED (2026-09-04, by decision)

**Correction on execution:** the "Risk: low" label above was wrong. All 37
`.claude/skills/*` files are legitimate thin routers (not bloat — real
content lives once in `.codex/skills/`), and the 10 "alias" names
(`skill_debug`, `skill_impl`, etc.) are used as **canonical identifiers**
in 20+ live files — `SKILLS.md`, `AGENTS.md`, `CLAUDE.md`, `CODEX.md`,
backlog docs, and an ADR — not just convenience redirects. Deleting them
requires a synchronized multi-file rename, not a simple cleanup.

**Decision: skip.** The alias files are cheap (15 lines each) and not
actually harmful; the original framing that this was easy low-risk
cleanup was wrong. Revisit only if the skill catalog grows further. The
single-ticket-skill merge proposals (`auv3-plugin-lifecycle` +
`clap-plugin-lifecycle` -> `plugin-format-lifecycle`,
`hrtf-rendering-validation-lab` + `perceptual-listening-harness` ->
`spatial-audio-validation`) and a found stray dead file
(`.codex/skills/skill_design_webview/SKILL_OLD.md`) remain in the draft
at `/tmp/.../scratchpad/batch-f-skills-draft.md` if revisited later, but
were not applied.

---

## Batch G — Governance-doc de-duplication ✅ DONE (2026-09-04)

**Risk: low.**

| Action | Detail |
|---|---|
| DONE | Thinned `CLAUDE.md` (117→93 lines), `CODEX.md` (77→69), `AGENTS.md` (131→112) — restated sections (Priority Order, phase discipline, backlog automation, framework gate, spec/ADR handling, validation vocabulary, troubleshooting) replaced with pointers to `AGENT_RULE.md`. Agent-surface-specific content (Codex reasoning tiers, Claude's Output/Quality Contracts, path listings, slash-command routing table) kept in full per a conservative section-by-section comparison — nothing cut without a specific matching `AGENT_RULE.md` section. |
| **Gap found and fixed:** | `AGENT_RULE.md`'s own Priority Order never ranked `CLAUDE.md`/`CODEX.md`/`AGENTS.md` against each other, despite declaring itself canonical over them — the two files being thinned had *independently drifted* restatements (both omitted "Safety and correctness" as a tier). Fixed in `AGENT_RULE.md` itself: added an explicit rank for "the active per-agent-surface contract (CLAUDE.md/CODEX.md)" between AGENT_RULE.md and workflow/skill instructions, plus a line stating that pointers in the other three files are authoritative back to this file. Synced to `.codex/rules/agent.md` and `.claude/rules/agent.md` per the file's own sync command — diff-confirmed identical. |
| DEFERRED, by decision | Root `LICENSE` file / JUCE licensing mode — explicitly skipped. This is a business decision (commercial vs. open-source distribution, which determines whether a paid JUCE commercial license is required instead of GPLv3) that doesn't belong inside a documentation cleanup pass. Still an open item — see "Explicitly out of scope" below. |

**Verification:** section-by-section comparison of every `##` heading in all three files against `AGENT_RULE.md`, applied only where a specific matching section existed; line counts confirmed exactly matching the draft's predictions post-edit; sync diff-confirmed identical.

---

## Batch H — CI cost/speed: gate macOS jobs on changed paths ✅ DONE (2026-09-05)

**Risk: low — additive gating only, no job's actual steps changed.** Not
part of the original 7-batch plan; added after direct evidence from this
session's own commits.

**Trigger:** nearly every commit pushed while executing Batches A-G
(a one-line allowlist fix, TestEvidence moves, markdown-only governance
edits) ran the **full macOS gauntlet** unconditionally: `qa-format-clap`
(30min), `qa-format-auv3` (35min), the macOS leg of `qa-critical`, and the
**90-minute** `qa-bl029-reliability-gate` — roughly 3+ hours of macOS
runner time per push, none of which could possibly be affected by those
changes.

| Action | Detail |
|---|---|
| ADDED | A `changes` job (`.github/workflows/qa_harness.yml`) that diffs the PR/push against its base SHA and outputs `code_changed`, matching on `Source/`, `CMakeLists.txt`, `qa/`, `third_party/`, the RT-safety script/allowlist, or the workflow file itself. No usable base SHA (workflow_dispatch, a brand-new branch) defaults to `true` rather than under-running. |
| GATED | `qa-format-clap`, `qa-format-auv3`, and both legs of `qa-critical` now require `code_changed == 'true'` OR the event being `workflow_dispatch` OR the ref being `main` — i.e. every PR still gets full validation the moment it touches code, but a docs/process-only commit no longer spends macOS minutes at all. |
| MOVED (per decision) | `qa-bl029-reliability-gate` (90 min) is no longer on every PR at all — now `main` push / `workflow_dispatch` / a `needs-reliability-gate` label applied to the PR. Uses `always()` in its `if:` to still run correctly when `qa-critical` itself was skipped (e.g. a labelled docs-only PR), while still refusing to run if `qa-critical` actively failed. |
| FIXED (needed for the label path) | Added `labeled`/`unlabeled` to the `pull_request` trigger's `types:` — otherwise applying the label to an already-open PR wouldn't re-trigger the workflow at all. |

**Verification:** YAML parses (`python3 -c "import yaml; yaml.safe_load(...)"`), all 8 job names present. Real behavioral verification is CI itself, on both a docs-only push (should skip all 4 heavy jobs) and a `Source/`-touching push (should run them) — pending, since the fix landed at the end of the session.

## Explicitly out of scope for this manifest

- **BoidsSystem / AttractorSystem / CollisionSystem / Choreography Lab** — contradicts the shipped V1 creative brief per the DSP audit, but keep-vs-cut is a product decision, not a cleanup. Tracked separately.
- **Sanitizer (ASan/TSan/UBSan) integration, Windows CI runner addition, companion `swift test` wiring** — these are new CI capability, not archival; tracked as their own follow-up work once Batch A-G land, so the baseline is stable before adding new gates on top of it.
- **PhysicsEngine/PhysicsWorker consolidation, processor_core/processor_bridge cleanup** — real-time code changes requiring the physics acceptance suite; separate PRs per the original audit's Phase 3.
- **Root `LICENSE` file / JUCE licensing mode** — deferred by explicit decision on 2026-09-04 (see Batch G). Business decision (commercial vs. open-source, GPLv3 vs. paid JUCE commercial license), not something a cleanup pass should decide. Still open.
- **Skill catalog alias-name rename** (Batch F) — skipped by explicit decision on 2026-09-04. Reclassified from "low risk" to "load-bearing in 20+ files" during drafting; not worth the risk for a purely cosmetic gain. Draft with the reclassification and the (still-available) single-ticket-skill merge proposals lives at `/tmp/claude-0/-home-user-LocusQ/57687359-7ef9-5fff-89f2-9e143addc7a6/scratchpad/batch-f-skills-draft.md` (session-local scratch path, not committed to the repo).
