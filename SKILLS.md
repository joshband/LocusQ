Title: APC Skills Index
Document Type: Skill Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-01

# SKILLS.md

## Purpose
Canonical skill index and usage guide for APC workflows.
Use this file to decide which skill to load, when to load it, and what outputs to expect.

## Core Rule
Skills define the "how". Workflows define the "when".
Never execute a phase skill without first passing workflow prerequisites.

## Mandatory Load Order
For any phase execution:
1. Load `.codex/rules/agent.md`
2. Load the matching file in `.codex/workflows/`
3. Load the referenced skill in `.codex/skills/`

## Phase Skill Map
| Phase | Trigger | Primary Skill | Key Outputs |
|---|---|---|---|
| Dream | `/dream [Name]` | `.codex/skills/dream/SKILL.md` | `.ideas/creative-brief.md`, `.ideas/parameter-spec.md`, `status.json` |
| Plan | `/plan [Name]` | `.codex/skills/plan/SKILL.md` | `.ideas/architecture.md`, `.ideas/plan.md`, framework decision |
| Design | `/design [Name]` | `.codex/skills/design/SKILL.md` | `Design/` specs and mockups |
| Implement | `/impl [Name]` | `.codex/skills/impl/SKILL.md` | `Source/` DSP/UI implementation |
| Test | `/test [Name]` | `.codex/skills/test/SKILL.md` -> `.codex/skills/skill_testing/SKILL.md` | QA artifacts and status validation updates |
| Ship | `/ship [Name]` | `.codex/skills/ship/SKILL.md` | release package artifacts in `dist/` |

## Specialist Skills
| Skill | File | Use When |
|---|---|---|
| `skill_docs` | `.codex/skills/docs/SKILL.md` | Documentation governance: metadata compliance, ADR hygiene, traceability upkeep, archive-tier policy, and root-doc sync discipline |
| `documentation-hygiene-expert` | `.codex/skills/documentation-hygiene-expert/SKILL.md` | SDLC-aware documentation cleanup plus git artifact hygiene: deduplicate bloated docs, enforce freshness ownership/cadence, consolidate canonical sources, align behavior claims to ADRs, and guard against tracked ignored/archive/build artifacts |
| `juce-webview-windows` | `.codex/skills/skill_design_webview/SKILL.md` | WebView implementation details or WebView crash/order hardening |
| `juce-webview-runtime` | `.codex/skills/juce-webview-runtime/SKILL.md` | Host/runtime interop, WebView bridge timeouts, callback ordering, UI click/hit-target anomalies, startup hydration issues |
| `skill_testing` | `.codex/skills/skill_testing/SKILL.md` | Detailed harness-first testing and plugin validation workflows |
| `skill_troubleshooting` | `.codex/skills/skill_troubleshooting/SKILL.md` | Build/runtime failures, recurring errors, issue capture |
| `skill_debug` | `.codex/skills/debug/SKILL.md` | Autonomous debug sessions and debugger configuration tasks |
| `threejs` | `.codex/skills/threejs/SKILL.md` | Three.js scene architecture, JUCE WebView bridge integration, and spatial-audio UI workflows including Apple Spatial Audio, Atmos-style routing, `7.4.2` layout handling, and binaural monitoring |
| `reactive-av` | `.codex/skills/reactive-av/SKILL.md` | Audio-reactive and physics-reactive visualization mapping, smoothing, stability, and visual QA |
| `realtime-dimensional-visualization` | `.codex/skills/realtime-dimensional-visualization/SKILL.md` | Realtime information visualization across 2D/3D/4D (time-aware) views, intentional UI art direction, and plugin-host render budget discipline |
| `simulation-behavior-audio-visual` | `.codex/skills/simulation-behavior-audio-visual/SKILL.md` | Complex simulation-driven audio+visual behavior (fluid-like fields, crowd/flocking/herd dynamics) with deterministic cross-domain mapping |
| `physics-reactive-audio` | `.codex/skills/physics-reactive-audio/SKILL.md` | Simulation-driven DSP behavior (flocking/herding/crowd/fluid/0G/gravity/drag/collision responses) with realtime safety requirements |
| `temporal-effects-engineering` | `.codex/skills/temporal-effects-engineering/SKILL.md` | Temporal DSP design/validation for delay, echo, looper, feedback networks, and frippertronics-style layering |
| `auv3-plugin-lifecycle` | `.codex/skills/auv3-plugin-lifecycle/SKILL.md` | AUv3 app-extension architecture, lifecycle/sandbox constraints, JUCE integration boundaries, and host-validation lanes |
| `clap-plugin-lifecycle` | `.codex/skills/clap-plugin-lifecycle/SKILL.md` | CLAP format architecture, JUCE migration/integration, capability negotiation, BL-011 implementation planning, and CLAP host/CI validation |
| `steam-audio-capi` | `.codex/skills/steam-audio-capi/SKILL.md` | Steam Audio C API integration for runtime loading, lifecycle-safe effect ownership, and deterministic fallback validation |
| `spatial-audio-engineering` | `.codex/skills/spatial-audio-engineering/SKILL.md` | Spatial audio architecture/integration/testing across ambisonics, binaural/HRTF, multichannel layouts, and BL-018 layout-expansion automation lanes |
| `headtracking-companion-runtime` | `.codex/skills/headtracking-companion-runtime/SKILL.md` | Companion readiness/sync gating, head-tracking runtime state-machine verification, axis/frame sanity checks, and deterministic operator diagnostics |
| `apple-spatial-companion-platform` | `.codex/skills/apple-spatial-companion-platform/SKILL.md` | Swift companion platform/API ownership for AirPods workflows: CoreMotion motion capture, ear-photo/depth capture flow, privacy-retention policy, and Apple API boundary constraints for BL-057/BL-058 |
| `hrtf-rendering-validation-lab` | `.codex/skills/hrtf-rendering-validation-lab/SKILL.md` | Offline SOFA truth-render validation, realtime FIR/partitioned-convolver parity, interpolation/crossfade safety, and BL-055/BL-061 evidence gates |
| `perceptual-listening-harness` | `.codex/skills/perceptual-listening-harness/SKILL.md` | Blinded listening protocol design, trial schema/stats outputs, reproducibility checks, and BL-060/BL-061 gate decision evidence |

## Three.js Skill Bundle
The `threejs` skill is organized as one triggerable skill plus focused references:

| Component | File | Purpose |
|---|---|---|
| Core trigger + workflow | `.codex/skills/threejs/SKILL.md` | Entry criteria, execution workflow, and delivery requirements |
| Scene architecture reference | `.codex/skills/threejs/references/scene-architecture.md` | Render-loop ownership, lifecycle, resize, and teardown patterns |
| JUCE WebView bridge reference | `.codex/skills/threejs/references/juce-webview-integration.md` | C++ to JS state flow, bridge contracts, and robust fallback patterns |
| Spatial-audio integration reference | `.codex/skills/threejs/references/spatial-audio-integration.md` | Emitter/listener transport, thread-safe DSP handoff, Apple Spatial Audio, Atmos workflows, `7.4.2`, and binaural mode support |
| Performance/debugging reference | `.codex/skills/threejs/references/performance-and-debugging.md` | Frame budget strategy, memory hygiene, and regression checks |
| SDK/API/OSS/research landscape | `.codex/skills/threejs/references/sdk-api-oss-research-landscape.md` | Curated ecosystem map for SDK selection and GitHub research/project discovery |

## Natural Language Routing
Map clear intent to phase skills through workflows:
- Ideate/start plugin -> dream
- Plan architecture -> plan
- Design UI/mockup -> design
- Implement/build code -> impl
- Test/validate -> test
- Package/release -> ship
- Check progress -> status workflow
- Continue from current state -> resume workflow

## Automatic Specialist Trigger Matrix (Codex + Claude)
When specialist intent is present, auto-load the matching specialist skill(s):
- WebView runtime/interop/callback/hydration issue -> `juce-webview-runtime`
- 3D scene/render loop/performance issue -> `threejs`
- Audio-reactive or physics-reactive visual behavior -> `reactive-av`
- Realtime 2D/3D/4D visualization, beautiful operator UI direction, or information-viz contract design -> `realtime-dimensional-visualization`
- Complex simulation-driven audio+visual behavior (fluid/crowd/flocking/herd) -> `simulation-behavior-audio-visual` (plus `physics-reactive-audio` when DSP-thread behavior changes)
- Physics/simulation-driven DSP/audio behavior -> `physics-reactive-audio`
- Temporal DSP effects (delay/echo/looper/frippertronics-style behavior) -> `temporal-effects-engineering`
- AUv3 format enablement, extension boundary design, or AUv3 host validation -> `auv3-plugin-lifecycle`
- CLAP format integration/migration, BL-011 work, or CLAP host-validation lanes -> `clap-plugin-lifecycle`
- Steam Audio C API runtime integration or BL-009 headphone-path work -> `steam-audio-capi`
- Spatial audio system planning/integration/QA (ambisonics, binaural, multichannel layouts, BL-018) -> `spatial-audio-engineering`
- Repo-wide documentation cleanup/de-bloat/freshness remediation with ADR alignment -> `documentation-hygiene-expert` (plus `skill_docs` when governance sync is required)
- Git tree/history artifact hygiene (tracked ignored files, stale archives, oversized blobs, pre-commit/CI guardrails) -> `documentation-hygiene-expert` (plus `skill_docs` when governance docs need updates)
- Documentation governance metadata, ADR/invariant traceability, and root-routing parity sync -> `skill_docs`
- Companion runtime state-machine/axis/sync gating issues -> `headtracking-companion-runtime`
- AirPods companion API/capture/privacy integration work (CoreMotion/AVFoundation/Vision, BL-057/BL-058) -> `apple-spatial-companion-platform`
- FIR/interpolation parity and crossfade validation work -> `hrtf-rendering-validation-lab`
- Blind listening protocol, stats gating, and reproducibility work -> `perceptual-listening-harness`
- Broad unresolved failures -> `skill_troubleshooting`

If multiple specialist intents apply, compose skills in this order:
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
1. `documentation-hygiene-expert` (cleanup/consolidation and freshness remediation)
2. `skill_docs` (governance metadata/traceability/routing sync closeout)

Canonical matrix and examples: `Documentation/skill-selection-matrix.md`.

## Documentation Skill Ownership Map
| Task Pattern | Primary Skill | Secondary Skill(s) |
|---|---|---|
| Backlog index/runbook cleanup and stale authority claims (`Documentation/backlog/index.md`, `Documentation/backlog/done/*`) | `documentation-hygiene-expert` | `skill_docs` |
| `ARCHITECTURE.md` and behavior-doc drift cleanup | `documentation-hygiene-expert` | `skill_docs`, optionally `skill_plan` |
| Root docs content freshness (`README.md`, `CHANGELOG.md`) | `documentation-hygiene-expert` | `skill_docs` |
| Routing/governance contract sync (`AGENTS.md`, `CODEX.md`, `CLAUDE.md`, `SKILLS.md`, `AGENT_RULE.md`) | `skill_docs` | `documentation-hygiene-expert` |
| API documentation cleanup and example correctness | `documentation-hygiene-expert` | `skill_docs`, optionally `skill_impl` |
| Code comment hygiene (stale/misleading comments) | `documentation-hygiene-expert` | `skill_impl`, optionally `skill_troubleshooting` |
| Git artifact hygiene audit/cleanup (tracked ignored files, stale archives, history bloat, guardrails) | `documentation-hygiene-expert` | `skill_docs`, optionally `skill_troubleshooting` |
| Validation evidence/trend governance updates | `skill_docs` | `documentation-hygiene-expert` |
| Skill/runtime markdown under `.codex/*` and `.claude/*` (skills/workflows/rules) | Exempt from docs skills by default | explicit user request only |


## Skill Execution Checklist
Before skill execution:
1. Confirm plugin exists (except dream/new plugin entry).
2. Read repository state from `status.json`.
3. Verify prerequisites from the selected workflow.
4. Confirm `ui_framework` if framework-specific output is required.

During execution:
1. Keep edits local to the current phase scope.
2. Write required artifacts for that phase only.
3. Update state fields and validation flags through state-management flow.

After execution:
1. Validate output files exist.
2. Record test/build results where applicable.
3. Stop and return the next expected command.

## Documentation Hygiene Hooks
- Canonical docs tiers and archive policy:
  - `Documentation/README.md`
  - `Documentation/standards.md`
- Archive root:
  - `Documentation/archive/`
- Generated scratch directories (non-canonical):
  - `Documentation/reports/`
  - `Documentation/exports/`
- Freshness gate (must pass before closeout):
  - `./scripts/validate-docs-freshness.sh`

## State Contract Reference
State is tracked in `status.json` with template references:
- `.codex/templates/status-template.json`
- `.claude/templates/status-template.json`

Do not invent new required fields in `status.json` unless state-management and templates are updated together.

## Maintenance
When adding or changing a skill:
1. Update this `SKILLS.md` entry.
2. Keep matching workflow references current in `.codex/workflows/`.
3. Keep `AGENTS.md` routing aligned if command behavior changes.
4. If phase/routing contracts change, update `AGENT_RULE.md` and sync parity copies:
   - `cp AGENT_RULE.md .codex/rules/agent.md && cp AGENT_RULE.md .claude/rules/agent.md`
5. For parity-managed files, update corresponding `.codex` and `.claude` entries in the same change set.
