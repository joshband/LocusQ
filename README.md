Title: LocusQ
Document Type: Repository README
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-20

# LocusQ

LocusQ is a JUCE 8 spatial-audio plugin and standalone runtime for calibrating rooms, authoring emitters, and rendering immersive scenes. The repository combines a C++/WebView product runtime, a head-tracking companion app, and a documentation-plus-validation system that keeps backlog state, architecture decisions, and evidence tied together.

## Current Posture

| Surface | Current value |
|---|---|
| Version | `v1.0.0-ga` |
| Phase | `code` |
| UI framework | `webview` |
| Runtime modes | `CALIBRATE`, `EMITTER`, `RENDERER` |
| Canonical status surface | [status.json](status.json) |
| Canonical backlog surface | [Documentation/backlog/index.md](Documentation/backlog/index.md) |
| Canonical evidence surfaces | [TestEvidence/build-summary.md](TestEvidence/build-summary.md), [TestEvidence/validation-trend.md](TestEvidence/validation-trend.md) |

Platform and UI posture:

| Platform | Default targets | UI runtime |
|---|---|---|
| macOS | `VST3`, `AU`, `Standalone` | `WKWebView` |
| Windows | `VST3`, `Standalone` | `WebView2` |
| Linux | `VST3`, `LV2`, `Standalone` | `WebKitGTK` |

Optional feature gates exist for `CLAP`, `AUv3`, `Steam Audio`, `SOFA`, and the head-tracking companion bridge. These are intentionally opt-in so the default build stays lean.

## Mode Model

- `CALIBRATE`: measurement, monitoring-path preparation, and room/profile diagnostics.
- `EMITTER`: per-source authoring, motion/physics state, and scene publication.
- `RENDERER`: final spatial output authority, layout/profile mapping, and runtime diagnostics.

This mode model is described in more detail in [ARCHITECTURE.md](ARCHITECTURE.md) and remains constrained by the realtime and scene-state rules in [Documentation/invariants.md](Documentation/invariants.md).

## Backlog Snapshot

- Canonical backlog authority lives in [Documentation/backlog/index.md](Documentation/backlog/index.md).
- Latest done-transition sync on `2026-03-20`: `BL-032` source modularization is now `Done` — PluginProcessor/PluginEditor decomposed into `processor_core`, `processor_bridge`, `shared_contracts`, `editor_shell`, `editor_webview` modules; guardrails 13/13 PASS (PluginProcessor.cpp 3248 ≤ 3600, RT audit non_allowlisted=0).
- `BL-078` carries processor-side finite-output enforcement and additive diagnostics split from `BL-036` (Done).
- `BL-036`, `BL-037`, and `BL-041` were previously archived under [Documentation/backlog/done/](Documentation/backlog/done) on `2026-03-05`.

For the full decision trail, start with [TestEvidence/build-summary.md](TestEvidence/build-summary.md) and [TestEvidence/validation-trend.md](TestEvidence/validation-trend.md).

## Quick Start

### Prerequisites

- CMake `3.22+`
- A C++20-capable toolchain
- JUCE available either through `JUCE_DIR`, `$JUCE_DIR`, or the sibling fallback path `../audio-plugin-coder/_tools/JUCE`

### macOS fast path

Use the repository helper when you want a local build plus plugin install into your user Audio Plug-Ins folders:

```bash
./scripts/build-and-install-mac.sh
```

Useful macOS toggles:

```bash
LOCUSQ_INSTALL_STANDALONE=1 ./scripts/build-and-install-mac.sh
LOCUSQ_ENABLE_CLAP=1 ./scripts/build-and-install-mac.sh
```

The helper installs:

- `~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3`
- `~/Library/Audio/Plug-Ins/Components/LocusQ.component`
- `~/Library/Audio/Plug-Ins/CLAP/LocusQ.clap` when `LOCUSQ_ENABLE_CLAP=1`

### Generic CMake flow

If you want to work directly with CMake:

```bash
cmake -S . -B build -DJUCE_DIR=/path/to/JUCE
cmake --build build --config Release --target LocusQ_VST3 LocusQ_Standalone -j 8
```

Common configure-time feature gates:

```bash
-DLOCUSQ_ENABLE_CLAP=ON
-DLOCUSQ_ENABLE_AUV3=ON
-DLOCUSQ_ENABLE_SOFA=ON
-DLOCUS_HEAD_TRACKING=ON
-DLOCUSQ_ENABLE_STEAM_AUDIO=ON
```

Notes:

- `LOCUSQ_ENABLE_SOFA=ON` uses `FetchContent`, so the first configure may need network access.
- `LOCUSQ_ENABLE_AUV3=ON` is Apple-only.
- `LOCUSQ_ENABLE_CLAP=ON` expects either a local `third_party/clap-juce-extensions` checkout or fetch permission through the CMake flow.

## Validation Entry Points

- Docs freshness gate: `./scripts/validate-docs-freshness.sh`
- macOS production standalone self-test: `./scripts/standalone-ui-selftest-production-p0-mac.sh`
- Release-governance and host-validation guidance: [Documentation/testing/bl-030-release-governance-qa.md](Documentation/testing/bl-030-release-governance-qa.md)
- Standalone self-test and REAPER smoke workflow: [Documentation/testing/production-selftest-and-reaper-headless-smoke-guide.md](Documentation/testing/production-selftest-and-reaper-headless-smoke-guide.md)
- Pluginval stability guidance: [Documentation/testing/pluginval-stability-contract.md](Documentation/testing/pluginval-stability-contract.md)

Decision-grade validation summaries live under [TestEvidence/](TestEvidence), while active testing references live under [Documentation/testing/](Documentation/testing).

## Repo Map

| Path | Purpose |
|---|---|
| [Source/](Source) | JUCE plugin/runtime code, DSP, processor/editor logic, and WebView bridge integration |
| [Source/ui/public/](Source/ui/public) | Web UI assets shipped inside the plugin/standalone binary |
| [companion/](companion) | Head-tracking companion runtime and related platform code |
| [scripts/](scripts) | Build, install, validation, and docs-governance automation |
| [Documentation/](Documentation) | Tiered documentation, ADRs, backlog authority, plans, and testing guides |
| [TestEvidence/](TestEvidence) | Canonical build-summary/trend surfaces plus referenced decision artifacts |
| [third_party/](third_party) | Local dependency payloads and optional SDK integration points |

## Canonical References

Start here when you need the repository's authoritative contracts rather than a high-level overview:

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [.ideas/architecture.md](.ideas/architecture.md)
- [.ideas/parameter-spec.md](.ideas/parameter-spec.md)
- [Documentation/README.md](Documentation/README.md)
- [Documentation/standards.md](Documentation/standards.md)
- [Documentation/invariants.md](Documentation/invariants.md)
- [Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md](Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md)
- [Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md](Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md)

These references are the main anchors for architecture intent, parameter authority, invariant enforcement, closeout sync, and artifact retention policy.
