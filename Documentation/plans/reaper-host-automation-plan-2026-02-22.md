Title: REAPER Host Automation and Manual QA Plan
Document Type: Planning
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-23

# REAPER Host Automation and Manual QA Plan

## Decision Summary

Yes, adding a REAPER headless lane is worth it for LocusQ. It improves automated host-level regression coverage for transport/layout/runtime integration, but it does not replace manual headphone listening checks for spatial perception.

## Why Headless REAPER Helps

1. Validates host-level plugin loading and render lifecycle (beyond standalone tests).
2. Catches transport/layout regressions in a real DAW execution path.
3. Supports deterministic CI-style smoke checks using scripted project renders.
4. Reduces manual setup cost for repeated verification.

## What Headless REAPER Does Not Replace

1. Perceptual validation of binaural quality (must still be listened to manually).
2. Interactive UI/gesture checks that require operator intent.
3. Device-specific confidence checks (for example AirPods routing in live monitoring).

## Proposed Hybrid Test Model

### Automated Lane (Headless)

- Run REAPER with a fixed project and offline render command.
- Assert:
  - process exits cleanly,
  - render artifact is produced,
  - LocusQ mode/routing state in project remains valid.
- Target checks:
  - transport/layout patch regressions,
  - Steam request/fallback stability,
  - plugin load stability after install/update.

### Manual Lane (Listening QA)

- Use a scripted project bootstrap:
  - Track A: synth source (ReaSynth + deterministic MIDI clip),
  - Track B: LocusQ renderer insert,
  - send Track A -> Track B.
- Run checklist with headphones:
  - `stereo_downmix` vs `steam_binaural` behavior,
  - mode/output switching coherence,
  - transport/keyframe interactions,
  - no crackle/dropouts when switching render modes.

## New Artifacts

- Headless smoke runner: `scripts/reaper-headless-render-smoke-mac.sh`
- BL-024 automation wrapper (multi-run + report): `scripts/qa-bl024-reaper-automation-lane-mac.sh`
- HX-03 multi-instance regression lane (clean + warm cache): `scripts/qa-hx03-reaper-multi-instance-mac.sh`
- Manual session bootstrap script (ReaScript): `qa/reaper/reascripts/LocusQ_Create_Manual_QA_Session.lua`
- Manual QA runbook: `Documentation/testing/reaper-manual-qa-session.md`

## Backlog Mapping

- `BL-024`: REAPER host automation lane + manual QA session bootstrap.
- `HX-03`: deterministic clean/warm cache multi-instance crash-regression lane for REAPER host stability.
- `BL-009` is prerequisite context for headphone-path assertions.

## Validation Gate Proposal for BL-024

1. `scripts/reaper-headless-render-smoke-mac.sh <project.rpp>` returns PASS.
2. `scripts/qa-bl024-reaper-automation-lane-mac.sh` publishes `status.tsv` + `report.md` with strict `locusqFxFound=true`.
3. ReaScript bootstrap creates two-track QA session with synth->LocusQ routing.
4. Manual runbook checklist completed with operator evidence row update.
5. `scripts/qa-hx03-reaper-multi-instance-mac.sh` passes both `clean_cache` and `warm_cache` phases with zero new crash reports.

## Closeout Update (2026-02-23)

- BL-024 closeout gate was completed with:
  - BL-024 wrapper acceptance pass (`3/3`): `TestEvidence/bl024_reaper_automation_20260223T030210Z/status.tsv`
  - Manual runbook evidence row: `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`
  - HX-03 deterministic multi-instance pass: `TestEvidence/hx03_reaper_multi_instance_20260223T031450Z/status.tsv`

## References

- REAPER User Guide: `http://reaper.fm/userguide.php`
- REAPER SDK (ReaScript): `http://reaper.fm/sdk/reascript/reascript.php`
- REAPER SDK (Extensions): `http://reaper.fm/sdk/plugin/plugin.php`
- SWS Extension: `https://sws-extension.org/`
- ReaPack: `https://reapack.com/`
- Official REAPER GitHub: `https://github.com/Cockos-Reaper-DAW/Reaper-Audio-Software`
- SWS GitHub: `https://github.com/reaper-oss/sws`
- X-Raym ReaScripts: `https://github.com/X-Raym/REAPER-ReaScripts`
- mpl ReaScripts: `https://github.com/MichaelPilyavskiy/ReaScripts`
- Przemoc ReaScripts: `https://github.com/przemoc/REAPER-ReaScripts`
- flavianohonorato scriptsForReaper: `https://github.com/flavianohonorato/scriptsForReaper`
- reaper-reableton scripts: `https://github.com/edkashinsky/reaper-reableton-scripts`
- Bitfocus Companion module: `https://github.com/bitfocus/companion-module-cockos-reaper`
- awesome-reaper: `https://github.com/indiscipline/awesome-reaper`
- REAPER topic index: `https://github.com/topics/reaper`
- JSFX plugin collection example: `https://github.com/ashaduri/reaper_plugins`
- REAPER OSC API integration example: `https://lykaiosnz.github.io/reaper-osc.js/classes/reaper.html`
