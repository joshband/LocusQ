Title: REAPER Host Automation Recommendation
Document Type: Exported Response
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

## Direct Answer

Yes, adding a REAPER headless lane is beneficial for LocusQ. It will improve automated host-level regression testing for transport/layout/runtime integration.

## What It Helps With

- Fast, repeatable DAW-level smoke checks after each patch.
- Detecting host lifecycle regressions (load/render/exit behavior).
- Validating transport/layout stability in a real host path.
- Reducing manual setup overhead for repeated QA cycles.

## What It Does Not Replace

- Human listening validation for binaural/spatial quality.
- Device-specific perceptual checks (for example AirPods behavior).
- UX-level interaction quality judgments.

## Recommended Test Model

Use a hybrid model:

1. Automated lane (headless REAPER)
- Render a fixed `.rpp` test project with LocusQ inserted.
- Assert clean process exit and artifact generation.
- Use for transport/layout and plugin load regressions.

2. Manual listening lane (REAPER session template)
- Track 1: synth source.
- Track 2: LocusQ insert.
- Route Track 1 -> Track 2.
- Validate `stereo_downmix` vs `steam_binaural` in headphones.

## Added Repo Artifacts

- `scripts/reaper-headless-render-smoke-mac.sh`
- `qa/reaper/reascripts/LocusQ_Create_Manual_QA_Session.lua`
- `Documentation/testing/reaper-manual-qa-session.md`
- `Documentation/plans/reaper-host-automation-plan-2026-02-22.md`
- Backlog item `BL-024` in `Documentation/backlog-post-v1-agentic-sprints.md`

## Steam Diagnostics Status (BL-009)

Latest opt-in self-test evidence confirms active Steam path on current host:

- `steamCompiled=true`
- `steamAvailable=true`
- `stage=ready`
- `err=0`
- Evidence: `TestEvidence/locusq_production_p0_selftest_20260221T104708Z.json`

## Key REAPER References Tracked in Backlog

- http://reaper.fm/userguide.php
- http://reaper.fm/sdk/reascript/reascript.php
- http://reaper.fm/sdk/plugin/plugin.php
- https://sws-extension.org/
- https://reapack.com/
- https://github.com/Cockos-Reaper-DAW/Reaper-Audio-Software
- https://github.com/reaper-oss/sws
- https://github.com/X-Raym/REAPER-ReaScripts
- https://github.com/MichaelPilyavskiy/ReaScripts
- https://github.com/przemoc/REAPER-ReaScripts
- https://github.com/flavianohonorato/scriptsForReaper
- https://github.com/edkashinsky/reaper-reableton-scripts
- https://github.com/bitfocus/companion-module-cockos-reaper
- https://github.com/indiscipline/awesome-reaper
- https://github.com/topics/reaper
- https://github.com/ashaduri/reaper_plugins
- https://lykaiosnz.github.io/reaper-osc.js/classes/reaper.html
