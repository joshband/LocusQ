Title: LocusQ Skill Selection Matrix
Document Type: Routing Guide
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-03-01

# Skill Selection Matrix (Codex + Claude)

## Purpose
Single reference for automatic skill selection in LocusQ so Codex and Claude choose the same skill set for the same intent.

## Automatic Selection Rules
1. If user explicitly names a skill token, load that skill.
2. If task intent clearly matches a skill description, auto-load that skill.
3. Choose minimal skill set required; do not load unrelated skills.
4. For phase work, always load order: `rules/agent.md` -> workflow -> selected skills.

## All Repo Skills
| Skill | Path | Typical Trigger |
|---|---|---|
| `skill_dream` | `.codex/skills/dream/SKILL.md` | plugin ideation/start concepts |
| `skill_plan` | `.codex/skills/plan/SKILL.md` | architecture/planning/phase strategy |
| `skill_design` | `.codex/skills/design/SKILL.md` | UI design/spec/mockup tasks |
| `skill_impl` | `.codex/skills/impl/SKILL.md` | implementation of DSP/UI wiring |
| `skill_test` | `.codex/skills/test/SKILL.md` | testing phase routing |
| `skill_ship` | `.codex/skills/ship/SKILL.md` | packaging/release preparation |
| `skill_docs` | `.codex/skills/docs/SKILL.md` | ADR/docs/metadata/traceability updates |
| `documentation-hygiene-expert` | `.codex/skills/documentation-hygiene-expert/SKILL.md` | SDLC-aware documentation cleanup plus git artifact hygiene (tracked ignored/archive/build cleanup and guardrails), deduplication, freshness governance, and ADR alignment hardening |
| `skill_debug` | `.codex/skills/debug/SKILL.md` | debugger-led investigation workflow |
| `skill_testing` | `.codex/skills/skill_testing/SKILL.md` | detailed harness-first QA execution |
| `skill_troubleshooting` | `.codex/skills/skill_troubleshooting/SKILL.md` | unresolved defects/root-cause capture |
| `juce-webview-windows` | `.codex/skills/skill_design_webview/SKILL.md` | JUCE WebView2 setup and ordering on Windows |
| `juce-webview-runtime` | `.codex/skills/juce-webview-runtime/SKILL.md` | host/runtime interop, bridge timing, callback/hit-target issues |
| `threejs` | `.codex/skills/threejs/SKILL.md` | Three.js scene/render/performance integration |
| `reactive-av` | `.codex/skills/reactive-av/SKILL.md` | audio-/physics-reactive visualization mapping and QA |
| `realtime-dimensional-visualization` | `.codex/skills/realtime-dimensional-visualization/SKILL.md` | realtime 2D/3D/4D information visualization, visual hierarchy, and intentional UI art direction under plugin-host constraints |
| `simulation-behavior-audio-visual` | `.codex/skills/simulation-behavior-audio-visual/SKILL.md` | complex simulation-driven audio+visual behavior (fluid/crowd/flocking/herd) with deterministic mapping contracts |
| `physics-reactive-audio` | `.codex/skills/physics-reactive-audio/SKILL.md` | simulation-driven DSP/audio behavior with realtime guarantees |
| `temporal-effects-engineering` | `.codex/skills/temporal-effects-engineering/SKILL.md` | delay/echo/looper/feedback-network/frippertronics-style temporal DSP architecture and QA |
| `auv3-plugin-lifecycle` | `.codex/skills/auv3-plugin-lifecycle/SKILL.md` | AUv3 app-extension architecture, lifecycle/sandbox boundaries, and AUv3 host validation |
| `clap-plugin-lifecycle` | `.codex/skills/clap-plugin-lifecycle/SKILL.md` | CLAP architecture/integration, BL-011 planning and execution, host/CI validation |
| `steam-audio-capi` | `.codex/skills/steam-audio-capi/SKILL.md` | Steam Audio C API integration, runtime fallback ownership, and BL-009 headphone lane validation |
| `spatial-audio-engineering` | `.codex/skills/spatial-audio-engineering/SKILL.md` | ambisonic/binaural/multichannel spatial audio design, integration, and deterministic QA lanes (including BL-018) |
| `headtracking-companion-runtime` | `.codex/skills/headtracking-companion-runtime/SKILL.md` | companion readiness/sync state-machine validation, axis/frame diagnostics, and runtime startup gating checks |
| `apple-spatial-companion-platform` | `.codex/skills/apple-spatial-companion-platform/SKILL.md` | Swift companion platform/API work for AirPods workflows (CoreMotion motion stream, capture/privacy contracts, BL-057/BL-058 platform boundaries) |
| `hrtf-rendering-validation-lab` | `.codex/skills/hrtf-rendering-validation-lab/SKILL.md` | offline SOFA truth lanes, realtime FIR/partitioned parity, interpolation/crossfade gates |
| `perceptual-listening-harness` | `.codex/skills/perceptual-listening-harness/SKILL.md` | blind listening protocol design/execution, metrics/stats gates, reproducibility evidence |

## Specialist Composition Order
When multiple specialist intents appear, compose skills in this order:
1. `juce-webview-runtime`
2. `reactive-av`
3. `realtime-dimensional-visualization`
4. `simulation-behavior-audio-visual`
5. `physics-reactive-audio`
6. `temporal-effects-engineering`
7. `auv3-plugin-lifecycle`
8. `clap-plugin-lifecycle`
9. `steam-audio-capi`
10. `spatial-audio-engineering`
11. `documentation-hygiene-expert`
12. `headtracking-companion-runtime`
13. `apple-spatial-companion-platform`
14. `hrtf-rendering-validation-lab`
15. `perceptual-listening-harness`
16. `threejs`
17. `skill_troubleshooting`

When both documentation skills are required, run:
1. `documentation-hygiene-expert` first for cleanup/consolidation.
2. `skill_docs` second for governance/metadata/traceability closeout.

Phase skills remain first when a phase command is active.

## Common Task Mappings
| Task Pattern | Skills |
|---|---|
| "UI button clicks fail in host but work in browser preview" | `juce-webview-runtime`, optionally `skill_troubleshooting` |
| "WKWebView and WebView2 behave differently for the same UI action" | `juce-webview-runtime`, optionally `threejs` |
| "Need to decide between multi-bus and cross-instance awareness" | `skill_plan`, `physics-reactive-audio` |
| "Improve Three.js frame rate and memory stability" | `threejs` |
| "Make visuals respond to spectrum/onsets and avoid jitter" | `reactive-av`, optionally `threejs` |
| "Design a beautiful realtime plugin UI with high information density and operator clarity" | `realtime-dimensional-visualization`, optionally `threejs`, `reactive-av` |
| "Add 2D/3D/4D time-aware telemetry views for spatial/simulation diagnostics" | `realtime-dimensional-visualization`, optionally `simulation-behavior-audio-visual`, `threejs` |
| "Add fluid/crowd/flocking/herd behavior that drives both sound and visuals" | `simulation-behavior-audio-visual`, `physics-reactive-audio`, optionally `reactive-av` |
| "Add flocking/crowd/drag/gravity model driving sound behavior" | `physics-reactive-audio`, optionally `skill_impl` |
| "Implement delay/echo/looper/frippertronics-style temporal behavior" | `temporal-effects-engineering`, optionally `spatial-audio-engineering`, `skill_testing` |
| "Add AUv3 support and validate extension lifecycle behavior" | `auv3-plugin-lifecycle`, optionally `skill_plan`, `skill_impl`, `skill_test`, `skill_docs` |
| "Add CLAP format support, host checks, and CI lanes (BL-011)" | `clap-plugin-lifecycle`, `skill_plan`, `skill_impl`, `skill_test`, optionally `skill_docs` |
| "Add or validate ambisonic/binaural/multichannel layout lanes (BL-018 or similar)" | `spatial-audio-engineering`, optionally `steam-audio-capi`, `threejs`, `skill_testing` |
| "Companion launches in wrong readiness state / sync gate behavior is inconsistent" | `headtracking-companion-runtime`, optionally `juce-webview-runtime`, `skill_troubleshooting` |
| "Implement/harden AirPods companion API + capture/privacy pipeline (CoreMotion/AVFoundation/Vision, BL-057/BL-058)" | `apple-spatial-companion-platform`, optionally `headtracking-companion-runtime`, `spatial-audio-engineering`, `skill_troubleshooting` |
| "Need offline/realtime HRTF parity and crossfade artifact checks" | `hrtf-rendering-validation-lab`, `spatial-audio-engineering`, optionally `steam-audio-capi` |
| "Need blinded listening protocol + gate decision artifacts" | `perceptual-listening-harness`, optionally `skill_docs`, `skill_testing` |
| "Docs are bloated/stale/scattered and need SDLC cleanup + ADR alignment" | `documentation-hygiene-expert`, `skill_docs`, optionally `skill_plan` |
| "Root docs (`README.md`/`CHANGELOG.md`/routing contracts) are stale or inconsistent" | `documentation-hygiene-expert`, `skill_docs` |
| "Backlog authority is drifting between `Documentation/backlog/index.md` and `Documentation/backlog/done/*`" | `documentation-hygiene-expert`, `skill_docs` |
| "Critical intent docs (for example `ARCHITECTURE.md`) conflict with active runbooks/status claims" | `documentation-hygiene-expert`, `skill_docs`, optionally `skill_plan` |
| "API docs/examples are stale after implementation changes" | `documentation-hygiene-expert`, `skill_docs`, optionally `skill_impl` |
| "Code comments are stale or misleading relative to runtime behavior" | `documentation-hygiene-expert`, `skill_impl`, optionally `skill_troubleshooting` |
| "Git tree/history has tracked ignored artifacts, stale evidence archives, or large blobs and needs safe cleanup + guardrails" | `documentation-hygiene-expert`, optionally `skill_docs`, `skill_troubleshooting` |
| "Skill/runtime markdown under `.codex/*` or `.claude/*` needs edits" | Exempt from docs skills by default; only route when explicitly requested | If requested: route to the explicitly named skill/runtime contract owner |
| "Implement optional host-specific cross-instance coordination" | `physics-reactive-audio`, `skill_troubleshooting`, optionally `skill_docs` |
| "Update ADR/traceability after implementation changes" | `skill_docs` |
| "Run formal test phase with harness evidence" | `skill_test`, `skill_testing` |

## Active Bundle: Head-Tracking + Calibration (BL-053..BL-060)

Use this bundle for the current calibration/head-tracking execution lane.

### Skill Order (default)
1. `skill_plan`
2. `skill_docs`
3. `spatial-audio-engineering`
4. `steam-audio-capi` (when Steam/monitoring-path behavior is in scope)
5. `threejs` (when companion/WebView visualization behavior is in scope)
6. `apple-spatial-companion-platform` (when companion Apple API/capture/privacy behavior is in scope)
7. `skill_impl` (when code edits are required)
8. `skill_troubleshooting` (for repro-first regression triage)
9. `skill_test` + `skill_testing` (for lane execution/replay evidence)

### Triggered Scenarios
| Scenario | Skills |
|---|---|
| BL-053 orientation path appears wired but no audible effect in `virtual_binaural` | `spatial-audio-engineering`, `steam-audio-capi`, `skill_impl`, `skill_testing` |
| Companion axis/pose display is odd (for example up/down appears lateral) | `threejs`, `skill_troubleshooting`, optionally `spatial-audio-engineering` |
| BL-058 companion profile acquisition requires CoreMotion/capture/privacy hardening | `apple-spatial-companion-platform`, `headtracking-companion-runtime`, optionally `skill_impl`, `skill_testing` |
| Companion/WebView bridge behavior differs by host/backend (`WKWebView` vs `WebView2`) | `juce-webview-runtime`, optionally `threejs`, `skill_troubleshooting` |
| Visualization should react deterministically to audio/pose/calibration features | `reactive-av`, optionally `threejs`, `skill_testing` |
| Physics/simulation signals are introduced into calibration or renderer behavior | `physics-reactive-audio`, optionally `reactive-av`, `skill_impl` |
| Converting Calibration POC findings into production backlog language | `skill_plan`, `skill_docs`, `spatial-audio-engineering` |
| Updating runbooks/intake templates/index with replay/evidence governance | `skill_docs`, optionally `skill_plan` |
| Deciding standalone POC MVP vs direct integration slice | `skill_plan`, `spatial-audio-engineering`, `skill_docs` |
| CLAP compatibility constraints affect execution/validation plan | `clap-plugin-lifecycle`, `skill_plan`, optionally `skill_test` |
| New concept exploration before backlog intake formalization | `skill_dream`, then `skill_plan`, then `skill_docs` |
| UI spec/prototype work for companion/plugin calibration surfaces | `skill_design`, optionally `threejs`, `juce-webview-runtime` |
| Release/distribution closeout after calibration lane acceptance | `skill_ship`, `skill_test`, `skill_docs` |

### Output Discipline
- At task start, state selected skills and order.
- When task intent changes materially (for example from docs to runtime bug triage), restate active skills.
- Keep evidence linked to repo-local `TestEvidence/` and referenced backlog runbooks.

## Agent Output Requirement
At task start, state selected skills and execution order when one or more skills are active.

## 2026-03-01 Reconciliation Skill-Use Map

Use this sequence when reconciling research -> backlog -> evidence for BL-053..BL-061:

| Step | Primary Outcome | Skills (ordered) |
|---|---|---|
| 1 | Build scoped plan and decide integration vs refactor posture | `skill_plan` |
| 2 | Normalize methodology/review/runbook language and metadata | `skill_docs` |
| 3 | Validate quaternion/renderer contracts and monitoring-path math expectations | `spatial-audio-engineering`, `steam-audio-capi` |
| 4 | Validate companion visualization frame mapping, axis sweeps, and UI diagnostics | `threejs`, `skill_troubleshooting` |
| 5 | Encode deterministic acceptance/replay evidence requirements in runbooks | `skill_docs`, `skill_testing` |
| 6 | Apply targeted implementation fixes only when evidence cannot be satisfied by docs/process alignment | `skill_impl` |
| 7 | Execute replay/manual lanes and capture canonical evidence under `TestEvidence/` | `skill_test`, `skill_testing` |
