Title: LocusQ
Document Type: Repository README
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-05

# LocusQ

LocusQ is a JUCE-based spatial audio plugin and standalone app that empowers audio creators to craft and monitor immersive 3D soundscapes effortlessly. With its intuitive interfaces, users can calibrate their audio setups, manipulate sound emitters within a three-dimensional space, and render spatialized audio with advanced diagnostics.

---

## Features at a Glance:
- Built with JUCE 8 and C++20, offering cross-platform plugin support across macOS (`VST3`, `AU`, `Standalone`), Windows (`VST3`, `Standalone`), and Linux (`VST3`, `LV2`, `Standalone`). Optional `CLAP` plugin target available.
- WebView-powered UI runtime (`WKWebView` on macOS, `WebView2` on Windows, `WebKitGTK` on Linux).
- Comprehensive QA scripted lanes for validation, with deterministic evidence under `TestEvidence/`.

---

## Backlog Snapshot

- Canonical backlog authority: `Documentation/backlog/index.md`.
- Latest done-transition sync (2026-03-04): BL-050, BL-069, and BL-070 are archived under `Documentation/backlog/done/`.

---

## Quick Start

### Step 1: Installation
#### macOS:
To build and install plugin bundles, run:
```bash
./scripts/build-and-install-mac.sh
```
- By default, plugins are installed at:
  - `~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3`
  - `~/Library/Audio/Plug-Ins/Components/LocusQ.component`

#### Windows:
Follow the setup steps in `[Documentation/backlog/windows-setup-guide.md].`

#### Optional:
To deploy standalone executables:
```bash
LOCUSQ_INSTALL_STANDALONE.
  ``Final``
