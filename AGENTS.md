Title: LocusQ Agent Dispatcher
Document Type: Agent Routing Guide
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-01

# AGENTS.md

## Intent
Repository-level operating contract for AI coding agents in the standalone `LocusQ` plugin repository.

## Repo Snapshot
- Path: `/Users/artbox/Documents/Repos/LocusQ`
- Stack: JUCE 8/C++ plugin, APC workflow contracts, local QA/test evidence tracking
- Canonical routing target: `.codex/`
- Plugin state file: `status.json`

## Instruction Priority
1. User request in current session.
2. This `AGENTS.md`.
3. `CODEX.md` (Codex) or `CLAUDE.md` (Claude).
4. `.codex/rules/agent.md` plus selected workflow/skill docs.
5. Existing repository conventions and scripts.

If instructions conflict, preserve build/test stability and phase/state contracts.

## Command Routing
Slash-command routing:
- `/dream [PluginName]` -> `.codex/workflows/dream.md`
- `/plan [PluginName]` -> `.codex/workflows/plan.md`
- `/design [PluginName]` -> `.codex/workflows/design.md`
- `/impl [PluginName]` -> `.codex/workflows/impl.md`
- `/test [PluginName]` -> `.codex/workflows/test.md`
- `/ship [PluginName]` -> `.codex/workflows/ship.md`
- `/status [PluginName]` -> `.codex/workflows/status.md`
- `/resume [PluginName]` -> `.codex/workflows/resume.md`
- `/new [PluginName]` -> `.codex/workflows/new.md`

Default `[PluginName]` to `LocusQ` when omitted.

Load order for phase execution:
1. `.codex/rules/agent.md`
2. Selected workflow in `.codex/workflows/`
3. Referenced skill in `.codex/skills/`

## Automatic Skill Routing (Codex + Claude)
- Both Codex and Claude must auto-select skills when either condition is true:
  - The user explicitly names a skill (for example `$threejs`, `$skill_docs`).
  - The task intent clearly matches a skill description in `SKILLS.md`.
- Selection method:
  1. Route to the phase workflow first (if applicable).
  2. Add the minimal specialist skills needed for the task.
  3. Keep load order: rule -> workflow -> specialist skills.
- Exemption rule: skill/runtime markdown under `.codex/skills/`, `.claude/skills/`, `.codex/workflows/`, `.claude/workflows/`, `.codex/rules/`, and `.claude/rules/` is out of scope for normal docs-hygiene/doc-governance passes unless explicitly requested.
- Specialist trigger priorities:
  - `juce-webview-runtime`: WebView host/runtime interop, click/hit-target issues, bridge timeout/callback ordering, startup hydration faults.
  - `reactive-av`: audio-reactive or physics-reactive visualization mapping, smoothing, jitter control, render behavior validation.
  - `realtime-dimensional-visualization`: realtime 2D/3D/4D (time-aware) visualization architecture, information visualization clarity, and intentional UI art direction for operator-facing plugin surfaces.
  - `simulation-behavior-audio-visual`: complex simulation-driven audio+visual behavior (fluid-like fields, crowd/flocking/herd dynamics, deterministic cross-domain mapping).
  - `physics-reactive-audio`: simulation-driven DSP/audio behavior (flocking/herding/crowd/fluid/0G/gravity/drag/collision audio responses).
  - `temporal-effects-engineering`: delay/echo/feedback-network/looper/frippertronics-style temporal effect design with deterministic realtime safety and host automation fidelity.
  - `auv3-plugin-lifecycle`: AUv3 format architecture, app-extension boundary decisions, sandbox/lifecycle constraints, and AUv3 host-validation lanes.
  - `clap-plugin-lifecycle`: CLAP format architecture, JUCE migration/integration, capability negotiation, BL-011 execution, and host/CI validation lanes.
  - `steam-audio-capi`: Steam Audio C API runtime loading, object lifecycle ownership, and BL-009 headphone-render integration/fallback verification.
  - `spatial-audio-engineering`: spatial audio architecture/integration/testing across ambisonics, binaural/HRTF, multichannel layout contracts, and BL-018 automation lanes.
  - `documentation-hygiene-expert`: SDLC-aware documentation cleanup/de-bloat, freshness ownership contracts, canonical consolidation, backlog/architecture/root-doc hygiene, API doc cleanup, stale code-comment remediation, and git artifact hygiene automation (tracked ignored/archive/build cleanup plus guardrails).
  - `skill_docs`: governance metadata compliance, ADR/invariant traceability, documentation standards/tier enforcement, and root routing-contract synchronization.
  - `headtracking-companion-runtime`: companion readiness/sync state-machine validation, axis/frame sanity diagnostics, and runtime telemetry-gated startup behavior.
  - `apple-spatial-companion-platform`: Swift companion Apple API integration (CoreMotion/AVFoundation/Vision), capture/privacy-retention contract enforcement, and BL-057/BL-058 platform-boundary decisions.
  - `hrtf-rendering-validation-lab`: offline truth-render parity, realtime FIR/partitioned-convolver validation, interpolation/crossfade gate checks.
  - `perceptual-listening-harness`: blind listening protocol execution, metric/statistical gate decisions, and reproducibility artifact contracts.
  - `threejs`: scene architecture/camera/materials/render loop/performance for 3D UI.
  - `skill_troubleshooting`: unresolved build/runtime failures and recurrent defects.
- When multiple skills apply, state chosen skills and order at task start.
- Canonical matrix: `Documentation/skill-selection-matrix.md`.


## Repo Skill Catalog (All Skills)
All currently supported repo skills that must be considered for routing:
- `skill_dream` -> `.codex/skills/dream/SKILL.md`
- `skill_plan` -> `.codex/skills/plan/SKILL.md`
- `skill_design` -> `.codex/skills/design/SKILL.md`
- `skill_impl` -> `.codex/skills/impl/SKILL.md`
- `skill_test` -> `.codex/skills/test/SKILL.md`
- `skill_ship` -> `.codex/skills/ship/SKILL.md`
- `skill_docs` -> `.codex/skills/docs/SKILL.md`
- `documentation-hygiene-expert` -> `.codex/skills/documentation-hygiene-expert/SKILL.md`
- `skill_debug` -> `.codex/skills/debug/SKILL.md`
- `skill_testing` -> `.codex/skills/skill_testing/SKILL.md`
- `skill_troubleshooting` -> `.codex/skills/skill_troubleshooting/SKILL.md`
- `juce-webview-windows` -> `.codex/skills/skill_design_webview/SKILL.md`
- `juce-webview-runtime` -> `.codex/skills/juce-webview-runtime/SKILL.md`
- `threejs` -> `.codex/skills/threejs/SKILL.md`
- `reactive-av` -> `.codex/skills/reactive-av/SKILL.md`
- `realtime-dimensional-visualization` -> `.codex/skills/realtime-dimensional-visualization/SKILL.md`
- `simulation-behavior-audio-visual` -> `.codex/skills/simulation-behavior-audio-visual/SKILL.md`
- `physics-reactive-audio` -> `.codex/skills/physics-reactive-audio/SKILL.md`
- `temporal-effects-engineering` -> `.codex/skills/temporal-effects-engineering/SKILL.md`
- `auv3-plugin-lifecycle` -> `.codex/skills/auv3-plugin-lifecycle/SKILL.md`
- `steam-audio-capi` -> `.codex/skills/steam-audio-capi/SKILL.md`
- `clap-plugin-lifecycle` -> `.codex/skills/clap-plugin-lifecycle/SKILL.md`
- `spatial-audio-engineering` -> `.codex/skills/spatial-audio-engineering/SKILL.md`
- `headtracking-companion-runtime` -> `.codex/skills/headtracking-companion-runtime/SKILL.md`
- `apple-spatial-companion-platform` -> `.codex/skills/apple-spatial-companion-platform/SKILL.md`
- `hrtf-rendering-validation-lab` -> `.codex/skills/hrtf-rendering-validation-lab/SKILL.md`
- `perceptual-listening-harness` -> `.codex/skills/perceptual-listening-harness/SKILL.md`

## Phase Discipline
- Enforce one phase at a time.
- Read and update `status.json` during phase work.
- Do not auto-advance phases after one command completes.
- Respect `ui_framework` in `status.json` (`visage` vs `webview`) as a hard gate.

## Core Rules
- Make scoped changes only; avoid unrelated edits.
- Do not revert user work outside requested scope.
- Prefer repository scripts over ad-hoc build flows.
- Report validation status explicitly: `tested`, `partially tested`, or `not tested`.

## Documentation Archive Contract
- Use `Documentation/README.md` as the tiered source-of-truth map (`Tier 0..3`).
- Treat `Documentation/exports/` as generated scratch only; it must remain empty/absent at closeout.
- `Documentation/reports/` may hold active non-canonical report artifacts when intentionally referenced by current docs.
- Archive generated bundles under `Documentation/archive/<YYYY-MM-DD>-<slug>/` and record manifests in that archive set.
- When docs are archived or promoted, update both:
  - `Documentation/README.md`
  - `Documentation/standards.md`
- Run `./scripts/validate-docs-freshness.sh` before closeout; this gate now fails if generated top-level docs folders contain files.

## Root Docs Sync
When execution posture, routing, or acceptance claims change, keep these root docs aligned in the same change set:
- `README.md`
- `CHANGELOG.md`
- `AGENTS.md`
- `CODEX.md`
- `CLAUDE.md`
- `SKILLS.md`
- `AGENT_RULE.md` (then sync with: `cp AGENT_RULE.md .codex/rules/agent.md && cp AGENT_RULE.md .claude/rules/agent.md`)

## Multi-Agent Runtime (Codex, Optional)
- Disabled by default for normal Codex sessions in this repo.
- Do not run watchdog/bootstrap/thread-heartbeat flows automatically.
- Use only when explicitly requested for parallel-agent experiments or diagnostics.
- Optional session bootstrap:
  - `./scripts/codex-session-bootstrap.sh`
- Optional thread contract updates:
  - `./scripts/codex-init --thread-id <id> --task "<task>" --expected-outputs "<artifact1|artifact2>" --timeout-minutes <N> --owner <name> --role <worker|coordinator>`
- Optional heartbeat updates:
  - `./scripts/codex-init --heartbeat-only --thread-id <id> --status "WORKING <step>" --last-artifact <path-or-commit>`
- Optional closeout gate:
  - `./scripts/thread-watchdog`
- Keep all scripts/docs/artifacts for future exploration, but treat them as opt-in tooling.

## High-Value Commands
```bash
./scripts/validate-docs-freshness.sh
```

Optional multi-agent tooling:
```bash
./scripts/codex-init --help
./scripts/thread-watchdog
```

## Handoff Checklist
- Changed files are listed and scoped to request.
- Validation commands/results are reported or explicitly skipped.
- `status.json` and phase docs are updated when phase work is performed.
