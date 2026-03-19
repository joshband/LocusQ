Title: BL-098 Local validation lane restoration and clean-build completeness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19 (Done — all acceptance criteria met; Reaper DAW auto-gate PASS satisfies BL098-A2)

# BL-098 Local validation lane restoration and clean-build completeness

## Plain-Language Summary

BL-098 in plain terms: restore repo-local validation so developers and reviewers can trust a clean build and at least one plugin-owned automated lane without depending on accidental build order. Current state: In Validation. A fresh build tree now configures without the old manual policy flag, `locusq_webui_typecheck` is dependency-complete, and `ctest` now discovers a repo-local Web UI validation lane; the remaining work is to keep the documented local baseline tight and promotion-ready.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Local developers, QA maintainers, release owners, and reviewers who need a fast trustworthy validation baseline inside this repo. |
| What is changing? | Clean-build dependency wiring, local automated test registration, and minimal repeatable validation commands are being tightened. |
| Why is this important? | Right now a clean repo can fail typecheck for dependency-order reasons, and local `ctest` does not provide meaningful plugin-side confidence. |
| How will we deliver it? | Fix build-graph dependencies first, restore at least one plugin-owned automated lane or equivalent local test target, and document the minimal validation contract. |
| When is it done? | This item is done when a clean-tree local validation pass succeeds deterministically and `ctest` or an equivalent repo-owned automated lane provides real plugin-side signal. |
| Where is the source of truth? | This runbook, the 2026-03-17 review report, BL-042/BL-085 related validation surfaces, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Validation table | Makes the local baseline contract explicit. | `## Validation Plan` |
| Acceptance + slices | Clarify what “local validation restored” actually means. | `## Acceptance IDs`, `## Implementation Slices` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-098 |
| Priority | P1 |
| Status | Done (2026-03-19: all five acceptance criteria met — clean-tree configure/UI typecheck PASS; Reaper DAW auto-gate PASS satisfies plugin-owned automated behavior lane; `ctest` discovers `locusq_webui_typecheck` plus physics probes; local baseline documented and replayable) |
| Track | G - Release/Governance |
| Effort | Medium / M |
| Depends On | BL-042 (Done) |
| Blocks | — |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Re-establish a trustworthy local validation floor for LocusQ. BL-098 is complete only when clean-tree UI validation no longer depends on unrelated prior targets and the repo exposes at least one meaningful local automated plugin-side validation lane that reviewers can run without reconstructing history or external context.

## Source Inputs

- `Documentation/reviews/2026-03-17-comprehensive-code-dsp-review.md`
- `CMakeLists.txt`
- `Documentation/backlog/done/bl-042-qa-ci-regression-gates.md`
- `Documentation/backlog/bl-085-cmake-integration-module.md`
- `TestEvidence/validation-trend.md`

## Acceptance IDs

- `BL098-A1` `locusq_webui_typecheck` is dependency-complete in a clean tree and no longer relies on other targets having already installed UI dependencies.
- `BL098-A2` A documented local validation entrypoint exists that proves at least one plugin-owned automated behavior lane, not just successful compilation.
- `BL098-A3` `ctest` either discovers meaningful repo-local tests or is explicitly replaced by a repo-owned alternative with equivalent local ergonomics and documentation.
- `BL098-A4` Minimal local validation commands are documented and replayable from a fresh checkout.
- `BL098-A5` Validation evidence and backlog/governance surfaces describe the restored local floor honestly.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Fix clean-build dependency completeness for UI/typecheck lanes. | `CMakeLists.txt`, related UI targets/scripts | clean-tree typecheck/build no longer depend on accidental prior target execution |
| B | Restore meaningful repo-local automated signal. | local test target registration, `ctest` integration or documented alternative, related scripts/CMake | at least one plugin-owned automated lane is discoverable and runnable locally |
| C | Close the governance/documentation loop for local validation. | backlog/evidence/docs surfaces, minimal validation guidance | reviewers can follow one documented local baseline without guessing |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL098-CLEAN-CONFIGURE | Automated | fresh configure/build setup in a new build dir | configure succeeds and required targets are present |
| BL098-CLEAN-UI | Automated | clean-tree `locusq_webui_typecheck` and/or UI build lane | exit 0 without relying on unrelated prior target runs |
| BL098-LOCAL-AUTO | Automated | `ctest --test-dir <build-dir> --output-on-failure` or documented repo-owned replacement | at least one meaningful plugin-side automated lane executes |
| BL098-DOCS | Automated | `./scripts/validate-backlog-plain-language.sh` + `./scripts/validate-backlog-redundancy.py` + `./scripts/export-backlog-summaries.py --check` + `./scripts/validate-docs-freshness.sh` | exit 0 |

## Validation Snapshot (2026-03-19)

| Check | Result | Notes |
|---|---|---|
| Fresh configure | PASS | `cmake -S . -B build_bl098_check2 -DBUILD_LOCUSQ_QA=ON -DCMAKE_BUILD_TYPE=Release` now succeeds without `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`. |
| Clean-tree Web UI typecheck | PASS | `cmake --build build_bl098_check2 --target locusq_webui_typecheck -j4` installs deps and typechecks from the fresh tree. |
| `ctest` discovery | PASS | `ctest --test-dir build_bl098_check2 -N` now lists `locusq_webui_typecheck` plus the existing probe tests. |
| `ctest` execution | PASS | `ctest --test-dir build_bl098_check2 -R '^locusq_webui_typecheck$' --output-on-failure` passed. |

**Closeout evidence (2026-03-19):** Reaper DAW auto-gate (`scripts/reaper-phys-daw-auto-gate-mac.sh`) passed all 4 gates (A=1, B=1, C=1, D=1) with BlackHole audio active, transport running, and 114 physics DAW params registered. This is the plugin-owned automated behavior lane required by BL098-A2. All five acceptance criteria are satisfied. Item is Done.

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | fresh configure + clean UI lane + local automated lane | validation matrix + logs |
| Candidate intake | T2 | 5 | repeated clean-tree local validation baseline | replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected local-validation baseline | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-098 is the local-validation follow-on from the 2026-03-17 review. It complements CI/governance items such as BL-042 and BL-085, but it specifically owns the gap between “CI eventually proves this somewhere” and “a reviewer can trust a fresh local checkout today.”
