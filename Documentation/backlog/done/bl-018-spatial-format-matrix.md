---
Title: BL-018 Spatial Format Matrix Strict Closeout
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-28
---

# BL-018: Spatial Format Matrix Strict Closeout

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Done (2026-02-24 strict matrix pass) |
| Owner Track | Track A — Runtime Formats |
| Depends On | BL-014 |
| Blocks | BL-026, BL-017 |
| Annex Spec | `Documentation/spatial-audio-profiles-usage.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Strict profile matrix validation + evidence |

## Objective

Promote the spatial profile expansion (mono, stereo, quad, binaural, ambisonic) from validation to Done with a strict warning-free evidence baseline. All profile switching paths must be deterministic and diagnostics publication must match the scene-state contract.

## Scope & Non-Scope

**In scope:**
- Strict rerun of all spatial profile switching lanes
- Warning-free evidence baseline capture
- Diagnostics field verification against scene-state contract

**Out of scope:**
- New profile additions (that's BL-026/027 territory)
- Head tracking integration (that's BL-017)
- Output matrix enforcement (that's BL-028)

## Architecture Context

- Spatial profiles defined in `Source/SpatialRenderer.h` (enum + string mapping)
- Profile switching logic in `Source/PluginProcessor.cpp` (renderer parameter update path)
- Diagnostics publication: `Documentation/scene-state-contract.md` defines requested/active/stage fields
- Device profiles: ADR-0006 defines compatibility tiers
- Profiles usage guide: `Documentation/spatial-audio-profiles-usage.md`
- Invariants: Audio thread (RT safety), DSP Chain (processing order), Device Compatibility (canonical scene intent)

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Strict profile matrix validation | `Source/SpatialRenderer.h`, `Source/PluginProcessor.cpp` | BL-014 done | All profile lanes warning-free |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/test BL-018: Spatial format matrix strict closeout
Load: $steam-audio-capi, $spatial-audio-engineering, $skill_docs

Objective: Validate all spatial profile switching paths produce warning-free,
deterministic results with correct diagnostics publication.

Profiles to validate: mono, stereo, quadraphonic, 5.1, 7.1, binaural_generic,
binaural_steam, ambisonic_1st, ambisonic_3rd

Constraints:
- Each profile must switch cleanly without warnings or fallback triggers
- Diagnostics fields (requested, active, stage) must update correctly
- No RT safety violations during profile switching

Validation:
- Run production self-test with profile cycling
- Verify scene-state diagnostics match expected values per profile
- Capture per-profile evidence

Evidence:
- TestEvidence/bl018_profile_matrix_<timestamp>/per_profile_results.tsv
- TestEvidence/bl018_profile_matrix_<timestamp>/diagnostics_snapshot.json
- Update TestEvidence/validation-trend.md
```

### Slice A — Standalone Fallback Prompt

```
You are validating BL-018 for LocusQ, a JUCE-based spatial audio plugin.

PROJECT CONTEXT:
- Spatial profiles: enum in Source/SpatialRenderer.h (around line 60) with string mapping
- Profile switching: Source/PluginProcessor.cpp handles renderer parameter updates
- Scene-state contract: Documentation/scene-state-contract.md defines diagnostics fields
  (requested, active, stage) that must update when profiles change
- Profiles usage: Documentation/spatial-audio-profiles-usage.md describes all supported profiles
- ADR-0006: Device compatibility profiles — quad studio, laptop stereo, headphone stereo

TASK:
1. Build: cmake --build build --target all
2. For each spatial profile (mono, stereo, quad, 5.1, 7.1, binaural, ambisonic):
   a. Set profile via parameter
   b. Run production self-test lane
   c. Verify diagnostics fields update correctly
   d. Check for warnings in output
3. Compile per-profile results into TSV
4. Capture diagnostics snapshot as JSON
5. Verify all profiles switch without warnings

CONSTRAINTS:
- RT safety: No allocation/lock/blocking in processBlock during profile switch
- Profile switching must be deterministic (same input = same output)
- Diagnostics must publish within one snapshot cycle

EVIDENCE:
- TestEvidence/bl018_profile_matrix_<timestamp>/per_profile_results.tsv
- TestEvidence/bl018_profile_matrix_<timestamp>/diagnostics_snapshot.json
- TestEvidence/validation-trend.md (appended row)
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-018-matrix | Automated | Production self-test with profile cycling | All profiles pass, 0 warnings |
| BL-018-diag | Automated | Diagnostics field verification | requested/active/stage correct per profile |
| BL-018-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Latest Strict Validation (2026-02-24)

- Build lane (fallback prompt step 1): `cmake --build build --target all` → `PASS` (warnings only)
- Command: `./scripts/qa-bl018-profile-matrix-strict-mac.sh`
- Result: `PASS`
- Artifact directory: `TestEvidence/bl018_profile_matrix_20260224T183138Z`
- Required evidence:
  - `TestEvidence/bl018_profile_matrix_20260224T183138Z/per_profile_results.tsv`
  - `TestEvidence/bl018_profile_matrix_20260224T183138Z/diagnostics_snapshot.json`
- Profile outcomes (`mono`, `stereo`, `quadraphonic`, `5.1`, `7.1`, `binaural_generic`, `binaural_steam`, `ambisonic_1st`, `ambisonic_3rd`):
  - `warnings=0`
  - `fallback_triggered=false`
  - `rt_safe=true`
  - `diagnostics_match=true`

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Steam Audio unavailable on test machine | Med | Low | Fallback to generic HRTF, document in evidence |
| Profile switch causes transient audio glitch | Med | Med | Verify crossfade behavior, accept if < 10ms |
| Diagnostics fields lag behind actual state | High | Low | Check sequence numbers, verify within 2 snapshot cycles |

## Failure & Rollback Paths

- If a profile fails to switch: check SpatialRenderer.h enum mapping, verify APVTS parameter value range
- If diagnostics lag: check scene snapshot publication timing in PluginProcessor.cpp
- If warnings appear: classify as blocking vs cosmetic, document in evidence

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Profile results TSV | `TestEvidence/bl018_profile_matrix_<timestamp>/per_profile_results.tsv` | profile, result, warnings, diagnostics_match |
| Diagnostics JSON | `TestEvidence/bl018_profile_matrix_<timestamp>/diagnostics_snapshot.json` | per-profile requested/active/stage |
| Validation trend | `TestEvidence/validation-trend.md` | date, lane, result, notes |

## Closeout Checklist

- [x] All spatial profiles switch warning-free
- [x] Diagnostics fields correct for every profile
- [x] Evidence captured at designated paths
- [x] status.json updated
- [x] Documentation/backlog/index.md row updated
- [x] TestEvidence surfaces updated
- [x] ./scripts/validate-docs-freshness.sh passes


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/bl-018-spatial-format-matrix.md` |

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
