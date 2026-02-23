Title: Spatial Audio Validation Lanes
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# Validation Lanes

## Objective Automation Lanes

### BL-009 Headphone Contract
Command:
```bash
bash scripts/qa-bl009-headphone-contract-mac.sh
```
Expected:
- Distinct evidence bundle under `TestEvidence/bl009_headphone_contract_<timestamp>/`.
- Explicit contract result for fallback (`stereo_downmix`) versus active binaural path (`steam_binaural` when available).

### BL-018 Ambisonic/Layout Contract
Command:
```bash
bash scripts/qa-bl018-ambisonic-contract-mac.sh
```
Expected:
- Distinct evidence bundle under `TestEvidence/bl018_ambisonic_contract_<timestamp>/`.
- FOA/layout manifests and deterministic checks (finite outputs, directional edge cases, lane status TSV).

### Combined Runner
Command:
```bash
./.codex/skills/spatial-audio-engineering/scripts/run_spatial_lanes.sh
```
Expected:
- Sequential execution summary for BL-009 and BL-018 lanes.

## Manual Listening Lanes
- Validate stereo fallback quality and image stability.
- Validate binaural path audibility and movement consistency.
- Confirm no regression in transport/layout UI behavior when spatial modes are enabled.

## Headphone-Specific Manual Checklist
- AirPods Pro 2:
  - Confirm host output reaches endpoint correctly.
  - Confirm no false claim of plugin-controlled personalized/head-tracked mode.
- Sony WH-1000XM5:
  - Confirm endpoint playback and level consistency.
  - Confirm tests are framed as DAW stereo/binaural monitoring unless 360 Reality Audio app/host integration is explicitly in scope.

## Evidence Closeout
Update when acceptance claims change:
- `Documentation/backlog-post-v1-agentic-sprints.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`
- `status.json` (if phase claims or shipped-capability fields changed)
