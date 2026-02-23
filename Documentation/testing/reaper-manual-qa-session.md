Title: REAPER Manual QA Session Template
Document Type: Test Plan
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-23

# REAPER Manual QA Session Template

## Purpose

Provide a repeatable REAPER listening-session template for LocusQ manual validation with a synth source and headphone monitoring.

## Bootstrap

1. Open REAPER.
2. Run ReaScript: `qa/reaper/reascripts/LocusQ_Create_Manual_QA_Session.lua`.
3. Confirm tracks exist:
   - `LQ QA Synth Source`
   - `LQ QA Spatial Renderer`
4. Confirm FX inserts:
   - Track 1 has `ReaSynth`.
   - Track 2 has `LocusQ` (VST3/AU if available).
5. Confirm send from Track 1 -> Track 2.

## Monitoring Setup

1. Connect headphones (for example AirPods Pro v2).
2. In REAPER audio device settings, select intended output device.
3. Ensure monitor path is stereo output and not muted.

## Checklist

1. Press play and confirm synth audio reaches LocusQ track.
2. Set LocusQ mode to `Renderer`.
3. Switch `rend_headphone_mode`:
   - `stereo_downmix`
   - `steam_binaural`
4. Verify audible mode change and no dropouts.
5. Run transport checks (`rewind`, `play`, `stop`).
6. Run keyframe surface checks (add/move/delete/curve).
7. Save/load one preset and verify state restoration.

## Evidence Logging

- Add run result row to:
  - `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`
- Include:
  - timestamp,
  - host version,
  - output device,
  - PASS/FAIL per checklist block,
  - short issue notes (if any).

## HX-03 Automated Stability Lane

Run this automated host-stability lane when closing or revalidating BL-024:

```sh
./scripts/qa-hx03-reaper-multi-instance-mac.sh
```

Expected HX-03 pass contract:
- `clean_cache` and `warm_cache` phases both pass.
- All per-instance artifacts report `status=pass`, `locusqFxFound=true`, `renderOutputDetected=true`.
- No lingering `REAPER` process remains after each phase.
- No new `REAPER-*.ips` or `LocusQ-*.ips` crash reports are emitted during the run window.
