Title: LocusQ Root README
Document Type: Project README
Author: Josh Band
Created Date: 2026-02-19
Last Modified Date: 2026-02-20

# LocusQ

**LocusQ** is a JUCE-based spatial audio plugin with a WebView-powered UI for 3D scene interaction.

It is designed for modern plugin workflows (VST3/AU/Standalone) and includes a built-in QA evidence trail for repeatable validation.

## Table of Contents
- [What LocusQ Does](#what-locusq-does)
- [Current Project Status](#current-project-status)
- [Tech Stack](#tech-stack)
- [Quick Start](#quick-start)
  - [macOS (recommended)](#macos-recommended)
  - [Manual CMake build](#manual-cmake-build)
- [Testing and Validation](#testing-and-validation)
- [Repository Layout](#repository-layout)
- [Documentation](#documentation)
- [Contributing](#contributing)

## What LocusQ Does

LocusQ provides a spatial mixing and motion workflow centered on:

- 3D emitter positioning
- Room and distance effects
- Motion/physics behaviors
- Keyframe/timeline-style animation
- Calibration and profile-aware rendering
- Multi-layout output support (including quad)

## Current Project Status

> Last synced from `status.json`.

- **Version:** `v1.0.0-ga`
- **Phase:** `code`
- **UI framework:** `webview`
- **GA milestone:** promoted (`notes: "v1.0.0-ga milestone promoted"`)

For machine-readable status details, see [`status.json`](status.json).

## Tech Stack

- **DSP / Plugin host:** C++20 + JUCE 8
- **UI host:** JUCE WebView integration
- **UI runtime assets:** `Source/ui/public/*`
- **Build system:** CMake (3.22+)
- **Validation:** local QA harness + scripted evidence logs (`TestEvidence/`, `qa_output/`)

## Quick Start

### macOS (recommended)

Use the project’s install script for local DAW-refreshable binaries:

```bash
./scripts/build-and-install-mac.sh
```

This script builds and installs:

- `~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3`
- `~/Library/Audio/Plug-Ins/Components/LocusQ.component`

Optional standalone app install:

```bash
LOCUSQ_INSTALL_STANDALONE=1 ./scripts/build-and-install-mac.sh
```

> The script is macOS-only and includes optional AU/REAPER cache refresh behavior.

### Manual CMake build

If you prefer a manual build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target LocusQ_VST3 -j
```

Add `LocusQ_AU` and/or `LocusQ_Standalone` targets as needed for your platform.

### Prerequisites

- CMake 3.22+
- A C++20-capable toolchain
- JUCE available via one of:
  - `-DJUCE_DIR=/path/to/JUCE`
  - `JUCE_DIR` environment variable
  - sibling checkout at `../audio-plugin-coder/_tools/JUCE`

## Testing and Validation

Common project checks:

```bash
# Documentation freshness gate
./scripts/validate-docs-freshness.sh

# Primary macOS UI PR gate
./scripts/ui-pr-gate-mac.sh
```

Primary evidence outputs:

- [`TestEvidence/build-summary.md`](TestEvidence/build-summary.md)
- [`TestEvidence/test-summary.md`](TestEvidence/test-summary.md)
- [`TestEvidence/validation-trend.md`](TestEvidence/validation-trend.md)
- [`qa_output/suite_result.json`](qa_output/suite_result.json)

## Repository Layout

```text
Source/            Plugin DSP/editor sources and embedded UI assets
scripts/           Build, QA, and automation scripts
Documentation/     ADRs, standards, contracts, and reviews
TestEvidence/      Validation logs, summaries, and artifacts
qa/                QA harness adapter and runner sources
status.json        Canonical machine-readable project state
```

## Documentation

Start with:

- [Documentation/README.md](Documentation/README.md)
- [Documentation/invariants.md](Documentation/invariants.md)
- [Documentation/scene-state-contract.md](Documentation/scene-state-contract.md)
- [Documentation/implementation-traceability.md](Documentation/implementation-traceability.md)
- [Documentation/adr/](Documentation/adr/)

## Contributing

1. Keep changes scoped to the task.
2. Prefer existing scripts over ad-hoc commands.
3. Update relevant docs/status artifacts when behavior or phase state changes.
4. Include validation evidence in PRs when possible.

---

If you are looking for agent workflow/routing instructions, see [AGENTS.md](AGENTS.md).
