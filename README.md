Title: LocusQ Root README
Document Type: Project README
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-26

# LocusQ

LocusQ is a spatial audio plugin for building 3D sound scenes in your DAW or standalone app.

It has three working views:
- `CALIBRATE` for routing and measurement setup.
- `EMITTER` for placing and animating sound sources in 3D space.
- `RENDERER` for final spatial rendering, room controls, and headphone monitoring.

## What You Can Do With It

- Place sounds in 3D using cartesian or spherical positioning.
- Animate source movement with timeline/keyframe controls.
- Use physics-style motion behaviors for dynamic source movement.
- Render for speaker layouts and headphone monitoring paths.
- Tune spatial parameters such as distance model, air absorption, room mix, and damping.
- Inspect scene behavior in an interactive WebView + Three.js interface.

## Plugin Formats

- macOS: `VST3`, `AU`, `Standalone`
- Windows: `VST3`, `Standalone`
- Linux: `VST3`, `LV2`, `Standalone`
- Optional (when enabled): `CLAP`

## Quickstart (macOS)

### 1. Build and install plugins

```bash
./scripts/build-and-install-mac.sh
```

This installs to:
- `~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3`
- `~/Library/Audio/Plug-Ins/Components/LocusQ.component`

Optional CLAP install:

```bash
LOCUSQ_ENABLE_CLAP=1 LOCUSQ_INSTALL_CLAP=1 ./scripts/build-and-install-mac.sh
```

Optional standalone install to `~/Applications`:

```bash
LOCUSQ_INSTALL_STANDALONE=1 ./scripts/build-and-install-mac.sh
```

### 2. Launch in DAW or standalone

- DAW: rescan plugins and load `LocusQ` on tracks/buses.
- Standalone (local build artifact):

```bash
open -na "$(pwd)/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
```

### 3. First-use workflow

1. Open `CALIBRATE` and verify routing (topology, monitoring path, outputs).
2. Open `EMITTER` and position at least one source in 3D.
3. Open `RENDERER` and choose monitoring/render options.
4. Play audio and confirm localization/motion behavior.

## Headphone Usage

For headphone testing:
- Set `Monitoring Path` in `CALIBRATE`.
- In `RENDERER`, select headphone mode/profile.
- Use built-in movement/animation in `EMITTER` to verify front/back and motion cues.

If a profile path is unavailable, LocusQ will report fallback state in diagnostics.

## Build From Source (All Platforms)

### Prerequisites

- CMake `>= 3.22`
- C++20 toolchain
- JUCE checkout available via `JUCE_DIR` or sibling path expected by this repo
- Optional Steam Audio SDK (only when enabling `LOCUSQ_ENABLE_STEAM_AUDIO=ON`)
  - Install with:
    ```bash
    ./scripts/install-steam-audio-sdk.sh
    ```
  - Manifest of pinned version/checksum:
    - `third_party/steam-audio/dependency.env`

### Configure + build

```bash
cmake -S . -B build_local -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build_local --config Release --target LocusQ_Standalone -j 8
```

Optional flags:
- `-DLOCUSQ_ENABLE_CLAP=ON`
- `-DLOCUSQ_ENABLE_STEAM_AUDIO=ON`

## Validation (Contributor-Facing)

Common validation commands:

```bash
./scripts/standalone-ui-selftest-production-p0-mac.sh
./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap
./scripts/validate-docs-freshness.sh
```

For headphone contract validation:

```bash
./scripts/qa-bl009-headphone-contract-mac.sh
./scripts/qa-bl009-headphone-profile-contract-mac.sh
```

## UI Views

### CALIBRATE

![LocusQ CALIBRATE state](Documentation/images/readme/locusq-state-calibrate.png)

### EMITTER

![LocusQ EMITTER state](Documentation/images/readme/locusq-state-emitter.png)

### RENDERER

![LocusQ RENDERER state](Documentation/images/readme/locusq-state-renderer.png)

## Troubleshooting

- Plugin not showing in DAW:
  - run `./scripts/build-and-install-mac.sh`
  - restart DAW and rescan plugins
- Standalone app not launching:
  - rebuild with `LocusQ_Standalone` target
  - verify app exists at `build_local/LocusQ_artefacts/Standalone/LocusQ.app`
- Self-test fails before payload output:
  - inspect `.run.log`, `.meta.json`, and `.attempts.tsv` emitted by self-test script

## Learn More

- `CHANGELOG.md`
- `Documentation/README.md`
- `Documentation/scene-state-contract.md`
- `Documentation/invariants.md`
- `Documentation/adr/`
