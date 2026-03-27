Title: BL-121 Companion Room Calibration and Device-Assisted Room Estimation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-26
Last Modified Date: 2026-03-26

# BL-121: Companion Room Calibration and Device-Assisted Room Estimation

## Plain-Language Summary

BL-121 adds a new `Room Calibration` lane to the LocusQ companion.
It should estimate a compact room profile using Mac-only quick scans first, then grow into an iPhone Pro full-scan path with stronger geometry and acoustic truth.

Current state: Open.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | LocusQ operators, companion maintainers, calibration owners, and QA reviewers deciding whether room-aware claims are trustworthy. |
| What is changing? | The headtracking companion grows a new room-calibration mode with Mac quick scan, guided multi-point capture, and a planned iPhone Pro full-scan path. |
| Why is this important? | LocusQ already talks about room context and calibration, but there is no truthful, companion-owned lane yet for estimating room conditions and feeding them back into CALIBRATE surfaces. |
| How will we deliver it? | Freeze the boundary in one room-profile contract, build Mac quick scan first, add guided waypoint capture, then add iPhone Pro geometry-assisted capture and proof lanes. |
| When is it done? | Done means the companion can produce a truthful room profile with explicit route provenance, confidence, repo-local evidence, and plugin-facing fallback language. |
| Where is the source of truth? | This runbook, `Documentation/plans/2026-03-26-companion-room-calibration-execution-packet.md`, and `TestEvidence/bl121_<slice>_<timestamp>/`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast scan of scope and dependencies | `## Status Ledger` |
| Progress snapshot | Shows current, next, and deferred work | `## Progress Snapshot` |
| Slice table | Keeps device-path sequencing explicit | `## Implementation Slices` |
| Validation plan | Makes proof ownership reviewable | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Open |
| Owner Track | E - Spatial / Calibration |
| Depends On | BL-058, BL-096, BL-101 |
| Blocks | future room-aware calibration and rendering follow-ons |
| Annex Spec | `Documentation/plans/2026-03-26-companion-room-calibration-execution-packet.md` |
| Default Replay Tier | T1 |
| Heavy Lane Budget | High-cost wrapper |

## Automation Contract

Draft-only by default.

| Field | Value |
|---|---|
| Automation Mode | `draft_only` unless owner-approved otherwise |
| Stage Cap | `T1` / `T2` / `T3` |
| Owner Approval Required For | `Done`, archive move, status/index transition |
| Runner Output | `DRAFT_READY`, `BLOCKED`, `MANUAL_ONLY` |

## Progress Snapshot

| Item | Status | Updated | Where | Remaining |
|---|---|---|---|---|
| Feature boundary and API posture | `[ACTIVE]` | 2026-03-26 | this runbook + execution packet | implementation slices and proof wrappers still need to be written |
| Existing motion companion baseline | `[DONE]` | 2026-03-26 | `companion/Sources/LocusQHeadTrackingCompanion/main.swift`, `companion/Sources/LocusQHeadTrackerCore/MotionService.swift` | none |
| Mac quick-scan implementation | `[NEXT]` | 2026-03-26 | companion runtime/audio capture path | no probe engine or room-profile estimator exists yet |
| iPhone Pro geometry-assisted scan | `[QUEUED]` | 2026-03-26 | new iPhone capture path + local sync boundary | RoomPlan capture and fusion architecture still need implementation |
| AirPods experimental capture lane | `[DEFERRED]` | 2026-03-26 | optional route-selection path | keep advisory-only until routing truth and variance are proven |

## Objective

Make room estimation a truthful companion feature.
That includes one companion-owned room-profile contract, one Mac-only coarse path, one stronger iPhone Pro path, and explicit confidence/fallback language in the plugin.

## Scope

### In scope

- companion-owned room calibration UI and runtime state
- Mac quick scan with laptop playback and recording
- guided multi-point capture with explicit waypoint confirmation
- iPhone Pro geometry-assisted scan planning and eventual integration
- compact room-profile persistence and plugin-facing truth surfaces
- route provenance, confidence scoring, and privacy defaults

### Out of scope

- plugin-side room probing in `processBlock()`
- exact room-dimension claims from Mac-only capture
- undocumented per-AirPods-microphone control
- cloud processing or silent upload of room recordings or scans

## Architecture Context

- Invariants: `Documentation/invariants.md` - truthful status, bounded payloads, finite-safe runtime, privacy-conscious persistence
- ADRs: `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`
- Architecture: `.ideas/architecture.md`, `.ideas/plan.md`, `Documentation/plans/2026-03-26-companion-room-calibration-execution-packet.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Freeze room-profile contract, route provenance, and truth language | companion docs, plugin calibration contract docs, bridge schema | runbook exists | one truthful compact room-profile contract exists |
| B | Mac `Quick Scan` playback/capture path | `companion/Sources/...` | Slice A complete | one-button coarse room scan runs end to end |
| C | Guided multi-point flow with camera guidance and waypoint labels | companion UI/runtime, Mac camera guidance path | Slice B complete | multi-point operator flow works with explicit waypoint confirmation |
| D | iPhone Pro full scan with RoomPlan and acoustic fusion | new iPhone-side capture module/app, local sync boundary | Slice A complete | geometry-assisted scan can produce a compact room profile |
| E | Plugin ingest and CALIBRATE truth surfaces | `Source/processor_core/...`, UI/bridge surfaces | Slices B or D produce profile data | plugin renders room-profile confidence and fallback reason truthfully |
| F | Validation and evidence wrappers | scripts, docs, `TestEvidence/bl121_*` | at least one runtime slice exists | deterministic proof and truthful manual evidence exist |

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL121-CONTRACT | Automated | `./scripts/qa-bl121-room-calibration-mac.sh --contract-only` | wrapper emits `status.tsv`, `validation_matrix.tsv`, and `summary.md` with no missing contract surfaces |
| BL121-COMPANION | Automated | `cd companion && swift test` | existing companion tests stay green and new room-profile tests pass |
| BL121-MAC-QUICKSCAN | Manual/Automated hybrid | `./scripts/qa-bl121-room-calibration-mac.sh --quick-scan` | Mac quick scan completes, records route provenance, and produces bounded-variance room metrics |
| BL121-MULTIPOINT | Manual/Automated hybrid | `./scripts/qa-bl121-room-calibration-mac.sh --multi-point` | named waypoint flow completes with truthful capture labels and confidence output |
| BL121-IPHONE | Manual/Automated hybrid | future iPhone capture wrapper or Xcode test target | RoomPlan-assisted capture exports geometry-assisted room profile without overclaiming precision |
| BL121-PLUGIN | Automated | targeted plugin round-trip / CALIBRATE proof wrapper | plugin-facing room profile, confidence, and fallback reason render truthfully |
| BL121-DOCS | Automated | `./scripts/export-backlog-summaries.py --check`, `./scripts/validate-backlog-plain-language.sh`, `./scripts/validate-backlog-redundancy.py`, `./scripts/validate-docs-freshness.sh`, `jq empty status.json` | exit 0 |

## Replay Cadence

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Evidence |
|---|---|---|---|
| Dev loop | T1 | 1-3 depending on route cost | contract logs, route provenance, room-profile snapshots |
| Candidate | T2 | 2 for heavy multi-device wrappers unless owner raises it | replay summary + blocker taxonomy |
| Promotion | T3 | 3 for heavy multi-device wrappers unless owner raises it | owner packet + deterministic/manual evidence |

## Risks

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| AirPods route changes break measurement trust | High | High | keep AirPods recording optional and advisory-only |
| Mac camera is mistaken for geometry truth | Med | High | use Mac vision only for guidance and waypoint confirmation |
| iPhone pairing flow becomes brittle | Med | Med | keep Mac quick scan independent and useful by itself |
| Raw room captures persist unexpectedly | High | Med | store compact room profile only by default |
| Walking flow overclaims spatial mapping | High | Med | require manual waypoints or iPhone AR geometry for spatial claims |

## Evidence Bundle

| Artifact | Path | Notes |
|---|---|---|
| `status.tsv` | `TestEvidence/bl121_<slice>_<timestamp>/status.tsv` | machine-readable packet status |
| `validation_matrix.tsv` | `TestEvidence/bl121_<slice>_<timestamp>/validation_matrix.tsv` | per-command results |
| `summary.md` | `TestEvidence/bl121_<slice>_<timestamp>/summary.md` | human-readable lane summary |
| `room_profile.json` | `TestEvidence/bl121_<slice>_<timestamp>/room_profile.json` | compact derived room profile, not raw audio |
| `route_provenance.json` | `TestEvidence/bl121_<slice>_<timestamp>/route_provenance.json` | input/output device, route mode, and confidence metadata |
| `capture_notes.md` | `TestEvidence/bl121_<slice>_<timestamp>/capture_notes.md` | truthful notes for manual and multi-device runs |

## Closeout Checklist

- [ ] Slices complete
- [ ] Validation lanes pass
- [ ] Evidence captured under `TestEvidence/...`
- [ ] Runbook summary and 6W stay current
- [ ] `Documentation/backlog/index.md` updated when state changes
- [ ] `status.json` updated when state changes
- [ ] `TestEvidence/build-summary.md` updated when required
- [ ] `TestEvidence/validation-trend.md` updated when required
- [ ] `./scripts/validate-docs-freshness.sh` passes

## Owner Sync Handoff

Use the canonical owner packet under:
- `TestEvidence/bl121_owner_sync_<slice>_<timestamp>/`

Required files:
- `status.tsv`
- `validation_matrix.tsv`
- `promotion_decision.md`
- `owner_decisions.md` when coordination risk is not low
- `handoff_resolution.md` when coordination risk is not low
