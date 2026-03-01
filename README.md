Title: LocusQ Root README
Document Type: Project README
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-03-01

# LocusQ

LocusQ is a JUCE-based spatial audio plugin and standalone app for building and monitoring 3D sound scenes.

It provides three runtime modes:
- `CALIBRATE`: routing, monitoring path, and calibration workflow
- `EMITTER`: source placement and movement in 3D space
- `RENDERER`: output/render controls and headphone render diagnostics

## Backlog Closeout Snapshot (2026-02-28)

- BL-023 is now `Done`; runbook archived at `Documentation/backlog/done/bl-023-resize-dpi-hardening.md`.
- Canonical promotion/closeout packet: `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/`.
- Owner closeout governance gates are green:
  - `jq empty status.json` => PASS
  - `./scripts/validate-docs-freshness.sh` => PASS

## Skill Routing Snapshot (2026-03-01)

New specialist skills are available for current head-tracking and calibration lanes:
- `headtracking-companion-runtime`
- `apple-spatial-companion-platform` (Swift/CoreMotion/Vision companion API ownership for BL-057/BL-058)
- `hrtf-rendering-validation-lab`
- `perceptual-listening-harness`
- `auv3-plugin-lifecycle` (AUv3 app-extension lifecycle and host-validation lanes)
- `temporal-effects-engineering` (delay/echo/looper/frippertronics-style temporal DSP contracts)
- `simulation-behavior-audio-visual` (fluid/crowd/flocking/herd simulation-driven audio+visual behavior)
- `realtime-dimensional-visualization` (beautiful, operator-focused 2D/3D/4D information-visualization systems)
- `documentation-hygiene-expert` (repo-scale docs cleanup/de-bloat/freshness remediation with ADR alignment, plus git artifact hygiene automation)
- `skill_docs` (documentation governance metadata/ADR traceability/root routing-contract sync)

Documentation ownership split:
- `documentation-hygiene-expert`: cleanup/de-bloat, backlog and architecture hygiene, root/API-doc freshness, stale code-comment remediation.
- `skill_docs`: governance metadata, ADR/invariant traceability, standards/tier enforcement, and routing-contract parity.

Git artifact hygiene automation commands:
- `./scripts/git-artifact-hygiene-audit.sh --ref HEAD`
- `./scripts/git-artifact-hygiene-guard.sh`
- `./scripts/git-artifact-cleanup-index.sh --manifest TestEvidence/git_artifact_cleanup_candidates.tsv`

Canonical routing references:
- `SKILLS.md`
- `Documentation/skill-selection-matrix.md`

## At a Glance

- Built with JUCE 8 and C++20
- WebView UI runtime (`WKWebView` on macOS, `WebView2` on Windows, `WebKitGTK` on Linux)
- Cross-platform plugin targets via CMake
- Scripted QA lanes and deterministic evidence artifacts under `TestEvidence/`

## Table of Contents

- [What LocusQ Does](#what-locusq-does)
- [Visual Overview](#visual-overview)
- [Platform and Plugin Formats](#platform-and-plugin-formats)
- [Quick Start (macOS)](#quick-start-macos)
- [Integrate Into a DAW](#integrate-into-a-daw)
- [Build From Source](#build-from-source)
- [Validation Commands](#validation-commands)
- [Validation Scope](#validation-scope)
- [Known Limitations](#known-limitations)
- [Roadmap and Backlog](#roadmap-and-backlog)
- [Repository Layout](#repository-layout)
- [Troubleshooting](#troubleshooting)
- [Documentation Map](#documentation-map)

## What LocusQ Does

LocusQ supports practical spatial mixing workflows:
- Place and move emitters in a 3D scene
- Switch emitter positioning between cartesian and spherical controls
- Render spatial output in standalone and plugin-host workflows
- Monitor headphone render requests and fallbacks through diagnostics
- Run repeatable QA checks with machine-readable outputs

Plain-language terms:
- `Emitter`: a sound source in the scene
- `Renderer`: the DSP/output stage that turns scene state into output channels
- `Monitoring path`: the active listening/output route

## Visual Overview

### Head-Tracking Companion

<p>
  <img src="Design/LocusQ%20Head-Tracking%20Companion.png" alt="LocusQ Head-Tracking Companion window showing bridge status and telemetry values." width="960" />
</p>
<p><em>Companion app view used with head-tracking workflows.</em></p>

### CALIBRATE Mode

What this mode does:
- Establishes your routing and monitoring contract before emitter/renderer work.
- Sets topology and device assumptions for downstream render behavior.
- Runs measurement/calibration passes and reports readiness/failure state.

Key settings shown in this mode:
- `Topology` (for example `Quad`) and monitoring path (for example `Steam Binaural`).
- `Device Profile` and `Mic Channel` selection for capture/verification context.
- `Output Mapping` with channel assignment (`FL/FR/RL/RR`) and `Redetect`.
- `Run and Validation` controls (`Test Type`, `Test Level`, `Start Measure`).

Key features:
- Channel-map contract visibility so routing mismatches are explicit.
- Limited/writable mapping safeguards for constrained topologies.
- Deterministic calibration gate behavior before moving to content authoring.

### EMITTER Mode

What this mode does:
- Authors and edits source behavior in the 3D scene.
- Controls emitter identity, position, motion, and shape per source.
- Exposes timeline lanes for animated movement contracts.

Key settings shown in this mode:
- `Emitter Identity`: label, color, mute, solo.
- `Position`: cartesian/spherical mode, azimuth/elevation/distance, world X/Y/Z.
- `Audio Shape`: size-linked behavior and gain/size controls.
- Timeline and motion controls including loop/sync state for lane automation.

Key features:
- Local emitter authority indicators (`LOCAL E1`) for edit ownership clarity.
- Physics/motion integration for dynamic scene behavior.
- Timeline lanes for repeatable automation of azimuth, elevation, distance, and size.

### RENDERER Mode

What this mode does:
- Converts scene state into final output and monitoring behavior.
- Chooses spatialization strategy, headphone path, and room/acoustic shaping.
- Surfaces output/meter state and runtime readiness diagnostics.

Key settings shown in this mode:
- `Spatialization`: distance model, headphone mode, headphone profile, reference distance.
- `Doppler` and `Air Absorb` toggles for motion/air modeling behavior.
- `Audition Source`: enable, signal preset, motion pattern, audition level.
- `Room Acoustics`: enable, mix, size, damping, early-reflection-only path.

Key features:
- Render-status visibility (`READY`) and output lane metering (`SPK1..SPK4`).
- Headphone route/profile pairing with fallback-aware behavior.
- Single panel for final render decisions before export or host playback checks.

## Platform and Plugin Formats

Current CMake configuration builds:
- macOS: `VST3`, `AU`, `Standalone`
- Windows: `VST3`, `Standalone`
- Linux: `VST3`, `LV2`, `Standalone`
- Optional target when enabled: `CLAP`

## Quick Start (macOS)

### 1. Build and install plugin bundles

```bash
./scripts/build-and-install-mac.sh
```

Default install paths:
- `~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3`
- `~/Library/Audio/Plug-Ins/Components/LocusQ.component`

Optional CLAP build/install:

```bash
LOCUSQ_ENABLE_CLAP=1 LOCUSQ_INSTALL_CLAP=1 ./scripts/build-and-install-mac.sh
```

Optional standalone copy to `~/Applications`:

```bash
LOCUSQ_INSTALL_STANDALONE=1 ./scripts/build-and-install-mac.sh
```

### 2. Launch

In a DAW:
- Rescan plugins and insert `LocusQ`

Standalone app:

```bash
open -na "$(pwd)/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
```

### 3. First run path

1. Open `CALIBRATE` and verify I/O and monitoring path
2. Open `EMITTER` and place at least one source
3. Open `RENDERER` and choose output/render settings
4. Play audio and verify movement/localization behavior

## Integrate Into a DAW

Use this workflow to integrate LocusQ into a host reliably.

### 1. Install the correct plugin format

- macOS hosts typically use `AU` and/or `VST3`
- Windows hosts use `VST3`
- Linux hosts use `VST3` or `LV2` (host-dependent)

On macOS, use:

```bash
./scripts/build-and-install-mac.sh
```

### 2. Rescan plugins in your DAW

- Restart your DAW after install
- Trigger a plugin rescan in host preferences
- Confirm `LocusQ` appears in effect plugin lists

### 3. Insert LocusQ on the correct track type

- Insert on an audio track or bus where you want spatial processing
- Feed source audio into LocusQ
- Keep host output/channel routing explicit during setup

### 4. Configure inside LocusQ

1. `CALIBRATE`: check monitoring path and routing state
2. `EMITTER`: place and tune emitter motion/position
3. `RENDERER`: select output/render behavior and inspect diagnostics

### 5. Verify in host playback

- Play transport and confirm output is present
- Move emitters and verify expected spatial behavior
- If host output and requested headphone mode diverge, check renderer fallback diagnostics

Hosts covered in repository QA lanes include:
- REAPER (`VST3`)
- Logic Pro (`AU`)
- Ableton Live (`VST3`)

## Build From Source

### Prerequisites

- CMake `>= 3.22`
- C++20 toolchain
- JUCE checkout available through `JUCE_DIR` or the expected sibling path
- Optional Steam Audio SDK (only if enabling `LOCUSQ_ENABLE_STEAM_AUDIO=ON`)

Install pinned Steam Audio dependency when needed:

```bash
./scripts/install-steam-audio-sdk.sh
```

The extracted SDK payload under `third_party/steam-audio/sdk/` is local-only and gitignored.
Re-run the installer after a fresh clone or when rotating local caches.

### Configure and build

```bash
cmake -S . -B build_local -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Useful flags:
- `-DLOCUSQ_ENABLE_CLAP=ON`
- `-DLOCUSQ_ENABLE_STEAM_AUDIO=ON`
- `-DBUILD_LOCUSQ_QA=ON`

Steam-enabled local configure example:

```bash
./scripts/install-steam-audio-sdk.sh
cmake -S . -B build_steam_local -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DLOCUSQ_ENABLE_STEAM_AUDIO=ON
cmake --build build_steam_local --config Release --target LocusQ_Standalone locusq_qa -j 8
```

## Validation Commands

Common local validation:

```bash
./scripts/standalone-ui-selftest-production-p0-mac.sh
./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap
./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/rt_audit.tsv
./scripts/validate-docs-freshness.sh
```

Headphone-specific lanes:

```bash
./scripts/qa-bl009-headphone-contract-mac.sh
./scripts/qa-bl009-headphone-profile-contract-mac.sh
./scripts/qa-bl033-headphone-core-lane-mac.sh --help
./scripts/qa-bl034-headphone-verification-lane-mac.sh --help
```

## Validation Scope

Routinely validated in this repo:
- Standalone UI/selftest contract lanes
- Scripted smoke lanes (including REAPER and headphone-profile governance lanes)
- RT safety static audit (`rt-safety-audit.sh`)
- Documentation freshness (`validate-docs-freshness.sh`)

Validation caveats:
- Some release-governance checks remain manual-evidence-dependent by design.
- Cross-platform coverage is not yet symmetric with macOS automation depth.

## Known Limitations

- Head-tracking quality upgrades (slerp interpolation, short-horizon prediction, and drift/re-center UX) are tracked in backlog and not fully closed in all lanes.
- Custom SOFA HRTF loading and per-source binaural expansion are planned and partially scaffolded, not yet full-production complete.
- Ambisonics/ADM/IAMF paths include roadmap and stubbed/profile-contract surfaces; stereo/quad/5.1/7.1/7.1.4 and current headphone lanes are the primary validated path.
- Renderer emitter budgeting is intentional (`top-N` active emitters per block) to preserve deterministic RT behavior under load.
- Windows/Linux build and runtime validation are improving but currently trail macOS-first QA depth.

## Roadmap and Backlog

- Canonical execution/status dashboard: [Documentation/backlog/index.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/index.md)
- Recently added architecture-derived hardening/roadmap items: `BL-043` through `BL-051`
- Done promotions finalized on 2026-02-27 for `BL-044`, `BL-046`, `BL-047`, `BL-048`, and `BL-049` (owner packet `TestEvidence/owner_done_promotion_bl044_bl046_bl047_bl048_bl049_z17_20260227T231736Z/status.tsv`)
- Done promotion finalized on 2026-02-28 for `BL-042` (owner packet `TestEvidence/owner_done_promotion_bl042_z18_20260228T163005Z/status.tsv`)
- BL-052 done closeout normalized on 2026-02-28 (runbook archived to `Documentation/backlog/done/` and owner closeout packet `TestEvidence/bl052_owner_sync_z1_20260228T175701Z/status.tsv`)
- Release governance status and blockers: [Documentation/backlog/bl-030-release-governance.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/bl-030-release-governance.md)

## Repository Layout

- `Source/`: plugin DSP/runtime/editor code
- `Source/ui/public/`: WebView UI assets
- `scripts/`: build, QA, and diagnostics automation
- `qa/scenarios/`: machine-readable QA scenarios/suites
- `Documentation/`: runbooks, standards, ADRs, and testing docs
- `TestEvidence/`: timestamped validation evidence
- `Design/`: visual/design assets (including companion screenshot)

## Troubleshooting

- Plugin does not appear in DAW:
  - run `./scripts/build-and-install-mac.sh`
  - restart DAW and rescan plugins
- Standalone app missing:
  - build `LocusQ_Standalone`
  - verify `build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app`
- Self-test exits before expected payload result:
  - inspect generated `.run.log`, `.meta.json`, and `.attempts.tsv` files
- RT safety gate fails:
  - run `./scripts/rt-safety-audit.sh --print-summary --output <out>.tsv`
  - review non-allowlisted rows

## Documentation Map

- `Documentation/README.md` (tiered source-of-truth map + freshness ownership/cadence contract)
- `Documentation/invariants.md`
- `Documentation/scene-state-contract.md`
- `Documentation/implementation-traceability.md`
- `Documentation/backlog/index.md` (canonical backlog status/order)
- `Documentation/adr/`
- `CHANGELOG.md`
