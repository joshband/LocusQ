Title: LocusQ Backlog Completion Plan — Mainline Execution Snapshot
Document Type: Planning Report
Author: APC Codex
Created Date: 2026-03-05
Last Modified Date: 2026-03-05

# LocusQ Backlog Completion Plan — Mainline Execution Snapshot

> Authority: `Documentation/backlog/index.md` for backlog state and `status.json` for repo execution posture.
> Operating model: the repository has been consolidated onto `main`; prior side-branch session instructions are retired.
> Snapshot date: 2026-03-05.

## Purpose

Translate the current backlog snapshot into an execution plan that matches the post-consolidation `main` workflow.
This report is an active Tier 1 planning surface under `Documentation/reports/`; it does not override Tier 0 status authority.

## Current Incomplete Inventory

17 backlog items remain incomplete as of 2026-03-05 after the BL-035, BL-036, BL-037, BL-038, BL-041, and BL-051 done-archive sync.

Machine summary export note: `Documentation/reports/data/backlog-summary.json` currently reports `open=18` / `done=68`, but that still overcounts the incomplete queue because legacy blank or markdown-wrapped status strings are misclassified by the exporter. The human-audited incomplete count for current execution planning is 17.

### Done-Candidate (promotion queue)

| ID | Title | Priority | Track | Dependency Gate | Next Action |
|---|---|---|---|---|---|
| BL-032 | Source modularization of PluginProcessor/PluginEditor | P2 | F | current `BL032-G-001` guardrail hold | clear hold, then promote on `main` |
| BL-039 | Parameter relay spec generation | P1 | B | BL-032 promoted | Promote after BL-032 |
| BL-040 | UI modularization and authority status UX | P1 | B | BL-039 promoted | Promote after BL-039 |

### In Validation

| ID | Title | Priority | Track | Current State | Next Action |
|---|---|---|---|---|---|
| BL-020 | Confidence/masking overlay mapping | P2 | E | validation packet is green; owner review pending | Prepare owner promotion packet |
| BL-053 | Head tracking orientation injection | P1 | E | structural replay and manual sync evidence are green | Prepare owner promotion packet |
| BL-055 | FIR convolution engine | P1 | E | C4/C6 remediation landed; follow-up PASS | Prepare owner promotion packet |

### In Implementation

| ID | Title | Priority | Track | Current State | Unblocked? |
|---|---|---|---|---|---|
| BL-021 | Room-story overlays | P2 | E | execute evidence and owner recheck are green; still listed In Implementation | YES |
| BL-058 | Companion profile acquisition UI + HRTF matching | P0 | E | active implementation lane; QA harness authored | YES |
| BL-059 | CalibrationProfile integration handoff | P0 | E | core integration lane; waiting on upstream items | NO |
| BL-076 | SpatialRenderer decomposition and boundary guardrails | P1 | F | Wave 6 helper extraction landed and replayed green | YES |

### Open

| ID | Title | Priority | Track | Depends On | Ready? |
|---|---|---|---|---|---|
| BL-054 | PEQ cascade RT integration | P1 | E | BL-052 done | YES |
| BL-056 | Calibration state migration + latency contract | P1 | E | BL-054, BL-055 | NO |
| BL-060 | Phase B listening test harness + evaluation | P1 | E | BL-059 | NO |
| BL-061 | HRTF interpolation + crossfade (conditional) | P2 | E | BL-060 gate pass | NO |
| BL-067 | AUv3 app-extension lifecycle and host validation | P1 | A | BL-048 done, BL-073 done | YES |
| BL-068 | Temporal effects core (delay/echo/looper/frippertronics) | P1 | E | BL-050 done, BL-055 | PARTIAL |
| BL-078 | Runtime finite-output enforcement and diagnostics | P0 | F | BL-036 done; follow-on created from moved runtime slices | YES |

## Mainline Priority Buckets

### 1. Immediate Promotion Pass On `main`

Use a governance-first pass to reduce backlog count without reopening branch sprawl.

- BL-035, BL-036, BL-037, BL-038, BL-041, and BL-051 completed this pass and are now archived under `Documentation/backlog/done/`.
- BL-032 remains in the promotion queue, but it is held by the current `BL032-G-001` structural guardrail failure (`Source/PluginProcessor.cpp` `3653 > 3600`).
- BL-036 is no longer a promotion blocker because its remaining runtime implementation work was split into BL-078.
- BL-078 is the new explicit P0 hardening lane for processor-side finite-output enforcement, diagnostics publication, and runtime fuzz/soak replay.
- BL-039 still follows BL-032, and BL-040 still follows BL-039.

### 2. Finish Current Validation Lanes

These items are already in the owner-review zone and should move before starting more downstream work.

- BL-055 is the highest-leverage validation lane because it unlocks BL-056 and BL-068.
- BL-053 should move next because it is part of the BL-059 dependency bundle.
- BL-020 is lower urgency but still a clean promotion candidate once owner packet work is refreshed.
- BL-021 should be reviewed for promotion or status normalization because its evidence posture is stronger than its current `In Implementation` label suggests.

### 3. Keep Startable Implementation Moving

These are the best active coding lanes while promotion work is progressing.

- BL-058 remains the main P0 feature lane.
- BL-078 is now the main P0 hardening follow-on created by the BL-036 scope split.
- BL-076 is an independent hardening lane with green replay evidence and minimal dependency drag.
- BL-054 is ready to start immediately and is needed before BL-056 and BL-059 can settle.
- BL-067 is startable now that BL-073 is done, but it still must satisfy the zero-`TODO` execute-evidence rule before promotion.

### 4. Downstream Unlock Sequence

- BL-054 plus BL-055 unlock BL-056.
- BL-053, BL-054, BL-055, BL-056, and BL-058 collectively unlock BL-059.
- BL-059 unlocks BL-060.
- BL-060 gate pass is required before BL-061.
- BL-055 promotion also clears the remaining dependency gate for BL-068.

## Dependency Spine

The shortest route to reducing backlog risk and opening the remaining Phase B/C calibration work is:

```text
BL-032 -> BL-039 -> BL-040
BL-036 -> BL-078
BL-054 + BL-055 -> BL-056
BL-053 + BL-054 + BL-055 + BL-056 + BL-058 -> BL-059 -> BL-060 -> BL-061
BL-055 -> BL-068
```

Independent progress lanes:

- BL-021 can move toward promotion without waiting on the calibration chain
- BL-067 and BL-076 can advance in parallel with the calibration chain if file-touch sets stay disjoint

## Operating Rules On `main`

- Use `main` as the integration branch. Do not revive the retired `claude/backlog-completion-plan-2h1PJ` workflow.
- Keep one active writer per BL/HX lane at a time.
- Treat `Documentation/backlog/index.md`, `status.json`, `TestEvidence/build-summary.md`, and `TestEvidence/validation-trend.md` as governance-sync surfaces.
- Keep canonical evidence repo-local under `TestEvidence/`; `/tmp` outputs are not authoritative.
- Run `./scripts/validate-docs-freshness.sh` before closing out planning/governance edits.
- Use optional parallel-agent tooling only when explicitly requested; the default operating mode is direct coordination on `main`.

## Recommended Order Of Operations

1. Run a promotion sync on `main` for BL-032, BL-035, BL-038, and BL-051.
   Status: BL-035, BL-038, and BL-051 completed; BL-032 held by the current guardrail recheck.
2. Run the next promotion sync on `main` for BL-036, BL-037, and BL-041.
   Status: completed; BL-036 archived after scope split, BL-037 archived, BL-041 archived, and BL-078 created as the runtime finite-output follow-on.
3. Continue the remaining promotion queue: BL-032, BL-039, and BL-040.
4. Clear the current validation queue: BL-055, BL-053, BL-020, and a BL-021 status/promotion review.
5. Continue the startable implementation lanes: BL-058, BL-078, BL-076, BL-054, and BL-067.
6. Move the dependency-locked follow-on work in order: BL-056, BL-068, BL-059, BL-060, and BL-061 if its gate passes.

## Risks And Follow-Ups

| Risk | Why It Matters | Mitigation |
|---|---|---|
| BL-078 is now the honest runtime follow-on for finite-output enforcement | BL-036 done no longer covers processor-side guardrail implementation, diagnostics publication, or execute replay | keep BL-078 visible as a P0 hardening lane until runtime evidence exists |
| BL-059 remains dependency-heavy | it is the main integration choke point for calibration/listening follow-on work | finish BL-053, BL-054, BL-055, BL-056, and BL-058 before committing to BL-059 scope |
| BL-067 and BL-068 still carry execute-evidence gate language | these lanes are not promotion-safe until `TODO` execute rows are cleared | keep contract-only work separate from promotion claims |
| BL-021 status may be stale relative to evidence | the queue can look more blocked than it really is | review runbook and owner packet posture before scheduling more implementation there |
| BL-061 is conditional | Phase C should not consume time before Phase B proves value | treat BL-060 as the explicit decision gate |

## Validation Status

`not tested` — this is a planning-document rewrite. Execution and promotion validation occur in the runbook and evidence lanes.

## Files Changed

- `Documentation/reports/2026-03-05-backlog-completion-plan.md`
