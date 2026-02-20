Title: APC Skills Index
Document Type: Skill Index
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-20

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
| `skill_docs` | `.codex/skills/docs/SKILL.md` | Documentation standardization, metadata compliance, ADR hygiene, traceability upkeep |
| `juce-webview-windows` | `.codex/skills/skill_design_webview/SKILL.md` | WebView implementation details or WebView crash/order hardening |
| `skill_testing` | `.codex/skills/skill_testing/SKILL.md` | Detailed harness-first testing and plugin validation workflows |
| `skill_troubleshooting` | `.codex/skills/skill_troubleshooting/SKILL.md` | Build/runtime failures, recurring errors, issue capture |
| `skill_debug` | `.codex/skills/debug/SKILL.md` | Autonomous debug sessions and debugger configuration tasks |
| `threejs` | `.codex/skills/threejs/SKILL.md` | Three.js scene architecture, JUCE WebView bridge integration, and spatial-audio UI workflows including Apple Spatial Audio, Atmos-style routing, `7.4.2` layout handling, and binaural monitoring |

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

## Skill Execution Checklist
Before skill execution:
1. Confirm plugin exists (except dream/new plugin entry).
2. Read `plugins/[Name]/status.json`.
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

## State Contract Reference
State is managed via `scripts/state-management.ps1` and template files:
- `.codex/templates/status-template.json`
- `.claude/templates/status-template.json`

Do not invent new required fields in `status.json` unless state-management and templates are updated together.

## Maintenance
When adding or changing a skill:
1. Update this `SKILLS.md` entry.
2. Keep matching workflow references current in `.codex/workflows/`.
3. Keep `AGENTS.md` routing aligned if command behavior changes.
4. If phase/routing contracts change, update `AGENT_RULE.md` and run `pwsh ./scripts/sync-agent-contract.ps1`.
5. For parity-managed files, update corresponding `.codex` and `.claude` entries and run `pwsh ./scripts/sync-agent-contract.ps1`.
