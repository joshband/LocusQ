Title: BL-052 Steam Audio Virtual Surround + Quad Layout
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28
Validated By: APC Codex
Validation Date: 2026-02-28

# BL-052 Steam Audio Virtual Surround + Quad Layout

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-052 |
| Priority | P1 |
| Status | Done (A1 harness PASS, test-phase gates PASS, done-archive sync PASS) |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-038 |
| Blocks | BL-053, BL-054 |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |
| SHARED_FILES_TOUCHED | no |
| Promotion Decision Packet | `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/promotion_decision.md` |
| Final Evidence Root | `TestEvidence/bl052_20260228_170811/` |
| Archived Runbook Path | `Documentation/backlog/done/bl-052-steam-audio-virtual-surround-quad-layout.md` |

## Objective

Implement `QuadSpeakerLayout` enum and `SteamAudioVirtualSurround` class to render a quad speaker bed to stereo binaural via Steam Audio, and wire a deterministic monitoring mode switch (`speakers` / `steam_binaural` / `virtual_binaural`) in `PluginProcessor`.

## Acceptance IDs

- quad->binaural renders without RT allocation in `processBlock`
- monitoring mode switch is deterministic
- speakers path is unchanged

## Validation Plan

- QA harness script: `scripts/qa-bl052-steam-audio-virtual-surround-mac.sh`
- Build: `cmake --build build_local --target LocusQ_Standalone -- -j4`
- Selftest: `./scripts/standalone-ui-selftest-production-p0-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app`
- Governance gates: `jq empty status.json`, `./scripts/validate-docs-freshness.sh`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | `./scripts/qa-bl052-steam-audio-virtual-surround-mac.sh` | `TestEvidence/bl052_20260228_165555/status.tsv`, `TestEvidence/bl052_20260228_170009/status.tsv`, `TestEvidence/bl052_20260228_170149/status.tsv` |
| Candidate intake | T2 | 1 (owner-approved reduced cadence) | `./scripts/qa-bl052-steam-audio-virtual-surround-mac.sh` | `TestEvidence/bl052_20260228_170811/status.tsv` |
| Promotion | T3 | 1 (owner-approved reduced cadence) | build + qa lane + selftest + docs/schema gates | `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/` |

### Cost/Flake Policy

- BL-052 lane is lightweight and deterministic; targeted reruns were used to resolve one transient C13 contract check mismatch (`170009` -> `170149` PASS).
- Candidate/promotion cadence was reduced after deterministic replay parity stabilized and all governance gates were green.

## Ownership and Promotion Governance

- Owner handoff marker: `SHARED_FILES_TOUCHED: no`.
- Canonical promotion evidence is repo-local under `TestEvidence/`.
- Owner closeout packet:
  - `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/status.tsv`
  - `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/validation_matrix.tsv`
  - `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/owner_decisions.md`
  - `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/handoff_resolution.md`
  - `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/promotion_decision.md`

## Evidence

| Check | Result |
|---|---|
| C1 QuadSpeakerLayout.h exists | PASS |
| C2 Quadraphonic enum = 0 | PASS |
| C3 SteamAudioVirtualSurround.h exists | PASS |
| C4 prepare() declared | PASS |
| C5 applyBlock() declared | PASS |
| C6 renderVirtualSurroundForMonitoring in SpatialRenderer.h | PASS |
| C7 monitoringInputPtrs_ / monitoringOutputPtrs_ present | PASS |
| C8 PluginProcessor.h includes SteamAudioVirtualSurround.h | PASS |
| C9 calMonitorVirtualSurround member declared | PASS |
| C10 applyCalibrationMonitoringPath declared | PASS |
| C11 applyCalibrationMonitoringPath wired in PluginProcessor.cpp | PASS |
| C12 kSpeakers path returns early (no-op) | PASS |
| C13 No RT heap allocation in monitoring path | PASS |
| C14 calMonitorVirtualSurround.prepare() called in prepareToPlay | PASS |

## Done Transition Checklist

- [x] Runbook moved from `Documentation/backlog/` to `Documentation/backlog/done/`
- [x] `Documentation/backlog/index.md` updated with Done linkage
- [x] `status.json` synchronized
- [x] `TestEvidence/build-summary.md` updated
- [x] `TestEvidence/validation-trend.md` updated
- [x] Owner decision + handoff resolution linked
- [x] `jq empty status.json` passes
- [x] `./scripts/validate-docs-freshness.sh` passes

## Closeout Snapshot

- A1 lane evidence: `TestEvidence/bl052_20260228_170149/status.tsv`
- Final QA lane evidence: `TestEvidence/bl052_20260228_170811/status.tsv`
- P0 selftest evidence: `TestEvidence/locusq_production_p0_selftest_20260228T170816Z.json`
- Owner done closeout packet: `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/`


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-052-steam-audio-virtual-surround-quad-layout.md` |

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
