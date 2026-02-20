Title: LocusQ Skill Selection Matrix
Document Type: Routing Guide
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

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
| `skill_debug` | `.codex/skills/debug/SKILL.md` | debugger-led investigation workflow |
| `skill_testing` | `.codex/skills/skill_testing/SKILL.md` | detailed harness-first QA execution |
| `skill_troubleshooting` | `.codex/skills/skill_troubleshooting/SKILL.md` | unresolved defects/root-cause capture |
| `juce-webview-windows` | `.codex/skills/skill_design_webview/SKILL.md` | JUCE WebView2 setup and ordering on Windows |
| `juce-webview-runtime` | `.codex/skills/juce-webview-runtime/SKILL.md` | host/runtime interop, bridge timing, callback/hit-target issues |
| `threejs` | `.codex/skills/threejs/SKILL.md` | Three.js scene/render/performance integration |
| `reactive-av` | `.codex/skills/reactive-av/SKILL.md` | audio-/physics-reactive visualization mapping and QA |
| `physics-reactive-audio` | `.codex/skills/physics-reactive-audio/SKILL.md` | simulation-driven DSP/audio behavior with realtime guarantees |

## Specialist Composition Order
When multiple specialist intents appear, compose skills in this order:
1. `juce-webview-runtime`
2. `reactive-av`
3. `physics-reactive-audio`
4. `threejs`
5. `skill_troubleshooting`

Phase skills remain first when a phase command is active.

## Common Task Mappings
| Task Pattern | Skills |
|---|---|
| "UI button clicks fail in host but work in browser preview" | `juce-webview-runtime`, optionally `skill_troubleshooting` |
| "Improve Three.js frame rate and memory stability" | `threejs` |
| "Make visuals respond to spectrum/onsets and avoid jitter" | `reactive-av`, optionally `threejs` |
| "Add flocking/crowd/drag/gravity model driving sound behavior" | `physics-reactive-audio`, optionally `skill_impl` |
| "Update ADR/traceability after implementation changes" | `skill_docs` |
| "Run formal test phase with harness evidence" | `skill_test`, `skill_testing` |

## Agent Output Requirement
At task start, state selected skills and execution order when one or more skills are active.
