Title: BL-XXX [TITLE]
Document Type: Backlog Runbook
Author: APC Codex
Created Date: [YYYY-MM-DD]
Last Modified Date: [YYYY-MM-DD]

# BL-XXX: [TITLE]

## Status Ledger

| Field | Value |
|---|---|
| Priority | [P1/P2] |
| Status | [In Planning / In Progress / In Validation / Done] |
| Owner Track | [Track X — Name] |
| Depends On | [BL-YYY, BL-ZZZ] |
| Blocks | [BL-AAA] |
| Annex Spec | `[Documentation/plans/bl-XXX-....md]` |
| Default Replay Tier | [T0/T1/T2/T3/T4 per `Documentation/backlog/index.md`] |
| Heavy Lane Budget | [Standard / High-cost wrapper] |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | [Low/Med/High] | [S/M/L/XL] | [FILL] |
| B | [Low/Med/High] | [S/M/L/XL] | [FILL] |

## Objective

[One paragraph: what this item achieves, why it matters, what success looks like.]

## Scope & Non-Scope

**In scope:**
- [FILL]

**Out of scope:**
- [FILL]

## Architecture Context

[Brief summary of relevant architecture decisions, invariants, and ADR links.]

- Invariants: `Documentation/invariants.md` — [relevant categories]
- ADRs: [ADR-XXXX links]
- Architecture: `.ideas/architecture.md` — [relevant subsystems]

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | [FILL] | `Source/...` | [dependency done] | [self-test lane passes] |
| B | [FILL] | `Source/...` | Slice A done | [FILL] |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl BL-XXX Slice A: [INSTRUCTION]
Load: $[skill1], $[skill2]

Objective: [FILL]

Constraints:
- No heap allocation, locks, or blocking I/O in processBlock()
- Parameter reads must be RT-safe
- [additional constraints]

Validation:
- [exact commands to run]
- [expected output patterns]

Evidence:
- Write results to TestEvidence/bl_XXX_<timestamp>/
- Update TestEvidence/validation-trend.md
```

### Slice A — Standalone Fallback Prompt

```
You are implementing BL-XXX Slice A for LocusQ, a JUCE-based spatial audio plugin
(VST3/AU/CLAP) with a WebView UI using Three.js.

PROJECT CONTEXT:
- Repository: LocusQ
- Architecture: Three-mode plugin (EMITTER/RENDERER/CALIBRATE) with shared lock-free
  SceneGraph singleton, WebView UI via juce::WebBrowserComponent
- Key files: Source/PluginProcessor.cpp (APVTS, DSP dispatch, scene snapshot pub/sub),
  Source/PluginEditor.cpp (WebView shell, native bridge), Source/ui/public/js/index.js
  (UI runtime), Source/SpatialRenderer.h (spatial DSP chain)
- RT safety invariant: No heap allocation, locks, or blocking I/O in processBlock()
- Scene graph invariant: Lock-free inter-instance state exchange, sequence-safe snapshots

TASK:
[step-by-step implementation instructions]

FILES TO MODIFY:
[exact file paths with current-state summary]

CONSTRAINTS:
[RT safety, threading, framework rules]

VALIDATION:
[exact commands with expected output]

EVIDENCE:
[artifact paths and required fields]

REFERENCE DOCS:
- Architecture: .ideas/architecture.md
- Parameters: .ideas/parameter-spec.md
- Invariants: Documentation/invariants.md
- Annex spec: [path]
- Scene contract: Documentation/scene-state-contract.md
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| [UI-PX-XXXA] | Automated | `[command]` | Exit 0, no FAIL lines |
| [Manual-XX] | Manual | [Steps 1-N] | Checklist all checked |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | [T0/T1] | [1/3] | `[qa lane command]` | `validation_matrix.tsv`, run logs |
| Candidate intake | [T2] | [5 or justified alternative] | `[qa lane command]` | replay summary + taxonomy |
| Promotion | [T3] | [10 or owner-approved alternative] | `[qa lane command]` | owner packet evidence |

### Cost/Flake Policy

- Heavy wrappers (>=20 binary launches per wrapper run) must avoid repeated full-sweep reruns.
- On failure, diagnose failing run(s) first; do not blindly repeat full multi-run sweeps.
- Any cadence override must be documented in `lane_notes.md` or owner decision artifacts.

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| [FILL] | [High/Med/Low] | [High/Med/Low] | [FILL] |

## Failure & Rollback Paths

- If validation lane [X] fails: [diagnostic steps], [recovery action]
- If dependency assumption breaks: escalate to [BL-YYY] owner, document blocker in status notes
- If RT safety violation detected: revert change, audit with `$skill_troubleshooting`

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Self-test JSON | `TestEvidence/blXXX_<slug>_<timestamp>.json` | timestamp, pass_count, fail_count, lane_id |
| Validation trend entry | `TestEvidence/validation-trend.md` | date, lane, result, notes |
| Build summary update | `TestEvidence/build-summary.md` | date, build_type, result |

## Closeout Checklist

- [ ] All implementation slices complete
- [ ] All validation lanes pass
- [ ] Evidence bundle captured at designated paths
- [ ] `status.json` updated with current state and evidence notes
- [ ] `Documentation/backlog/index.md` dashboard row updated
- [ ] `TestEvidence/build-summary.md` snapshot updated
- [ ] `TestEvidence/validation-trend.md` trend entry added
- [ ] `README.md` and `CHANGELOG.md` updated (for Done transitions)
- [ ] `./scripts/validate-docs-freshness.sh` passes
- [ ] Replay cadence policy and any overrides are documented with rationale

## Owner Promotion Packet (Orchestrator Reuse)

When moving from `In Validation` toward `Done-candidate`, create an owner sync evidence bundle:
- `TestEvidence/<bl_or_hx>_owner_sync_<slice>_<timestamp>/`

Required owner packet files:
- `status.tsv`
- `validation_matrix.tsv`
- `owner_decisions.md`
- `handoff_resolution.md`
- `promotion_decision.md` (use `Documentation/backlog/_template-promotion-decision.md`)

Return contract for owner sync handoff:
```
HANDOFF_READY
TASK: <BL/HX Owner Sync Slice>
RESULT: PASS|FAIL
DECISION: <In Validation|Done-candidate|Blocked>
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
BLOCKERS: ...
```
