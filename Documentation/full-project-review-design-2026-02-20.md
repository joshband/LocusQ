Title: LocusQ Full Project Review — Design Document
Document Type: Design Document
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# LocusQ Full Project Review — Design Document

## Purpose

This document defines the structure, scope, agent assignments, and delivery format for the
LocusQ full project review. It is the approved design for the review artifact and serves as
the implementation contract for the writing-plans phase.

## Output Artifact

`Documentation/full-project-review-2026-02-20.md`

Single navigable Markdown document with metadata header, educational commentary throughout,
Mermaid diagrams rendered by GitHub and VS Code, and copy-pasteable mega-prompts.

---

## Section Structure

### Section 0 — Research & Ecosystem Landscape

**Purpose:** Establish what already exists in the domain before reading any project-specific
findings. A reader new to this space should understand the landscape after reading this section.

**Produced by:** Parallel Opus 4.6 background agent using WebSearch. Runs concurrently with
Sections 1–2. Feeds findings back into domain reviews before they are finalized.

**Five sub-areas, each with:**
- One-sentence plain-language definition (learning context)
- State of the art
- Directly relevant libraries, repos, and packages
- LocusQ overlap assessment: `build` / `use` / `augment` / `ignore`

**Sub-areas:**

| Sub-area | LocusQ Components Affected |
|----------|---------------------------|
| 0a. Spatial Audio Algorithms (VBAP, HOA, binaural, HRTF, WFS) | VBAPPanner, SpatialRenderer, FDNReverb |
| 0b. Apple Spatial Audio & Platform APIs (AVAudioEnvironmentNode, PHASE, AU3D) | ADR-0006 device profiles, headphone path |
| 0c. JUCE Ecosystem (built-in DSP, community libs, CMake tooling) | All DSP components, build system |
| 0d. Audio Plugin Standards (VST3, AU, CLAP, AAX, pluginval, notarization) | Ship/distribution readiness |
| 0e. 3D Visualization & Audio-Reactive UI (Three.js patterns, Web Audio API spatial) | WebView viewport, physics-reactive UI |

**Key libraries to evaluate:**
- `resonance-audio` (Google) — ambisonics + HRTF
- `steam-audio` (Valve) — physics-based propagation, occlusion
- `IEM Plug-in Suite` — open-source ambisonics reference
- `SPARTA` — ambisonics toolkit
- `chowdsp_utils` — JUCE community DSP extensions
- `foleys_gui_magic` — JUCE UI utilities
- `Mach1 Spatial SDK` — multi-format spatial audio
- `three.js` spatial audio examples
- Apple `PHASE` framework

---

### Section 1 — System Map

**Purpose:** Visual mental model of the entire project. Read before any findings.

**Four Mermaid diagrams:**

**1a. High-Level Architecture**
- 3 plugin modes (Calibrate / Emitter / Renderer)
- SceneGraph singleton at center
- DAW process boundary
- WebView UI layer
- Inter-instance communication arrows

**1b. Data Flow & Thread Model**
- 4 concurrent threads: audio thread, physics timer thread, UI timer thread, registration path
- Data crossing labels: lock-free double-buffer, atomic swap, spinlock (rare), timer callback
- Makes real-time safety visible at a glance

**1c. Component Dependency Graph**
- All 22 source files as nodes
- Directed edges: "A uses B"
- Clusters by domain: DSP core, room acoustics, physics, UI bridge, calibration
- Shows blast radius before touching any file

**1d. Implementation Phase Timeline**
- Mermaid Gantt: phases 2.1–2.14+
- Complete phases marked
- Current phase highlighted
- Open phases shown ahead

---

### Section 2 — Domain Reviews

**Structure per domain:** Current State → Verdict → Findings (severity-ordered) → Opinionated
recommendation. Every finding gets a clear disposition: `fix now` / `fix next` / `acceptable`
/ `explicit defer`.

**2a. Architecture Review**
- SceneGraph singleton design vs alternatives
- Thread contracts vs ADR-0002 and invariants.md
- Lock-free double-buffer correctness (including interaction force changes from this session)
- Authority precedence chain (ADR-0003): DAW/APVTS → timeline → physics offset
- Comparison against Section 0 findings (is there a better routing model available?)

**2b. Code Review**
- Audio-thread safety audit across all 22 source files
- Heap allocation risk scan in processBlock paths
- Parameter coverage gap: `emit_dir_azimuth`, `emit_dir_elevation`, `phys_vel_x/y/z` — runtime
  present, UI bridge absent (Stage 14 medium finding)
- Production UI vs Stage 12 UI drift (now partially resolved)
- Test coverage gaps per component

**2c. Design Review**
- WebView production UI (`index.js` / `index.html`) vs `.ideas/architecture.md` spec
- Stage 12 incremental UI vs production parity delta
- Three.js viewport: what's implemented vs what's specced
- Command/acknowledgment path completeness (UI resilience contract)
- `emit_dir_*` / `phys_vel_*` UI exposure — defer or bind decision

**2d. QA Review**
- Scenario coverage matrix: every DSP component vs every scenario file
- Open `DEV-01..DEV-06` manual acceptance rows
- New scenario coverage (`locusq_multi_emitter_interaction.json`)
- pluginval / auval status
- Release gate status: what is blocking `draft-pre-release` → `ga`

---

### Section 3 — Model Assignment Rationale

**Purpose:** Explain which Claude model to use for which task and why, so you can make
informed choices when running mega-prompts.

**Assignment table:**

| Task Type | Model | Reasoning |
|-----------|-------|-----------|
| Architecture decisions, ADR authoring, cross-cutting multi-file design | Opus 4.6 | Needs deep context retention, multi-file causal reasoning, few shots at correctness |
| DSP implementation, UI wiring, focused C++/JS code review fixes | Sonnet 4.6 | Best cost/capability ratio for implementation with clear spec |
| Scenario JSON authoring, doc metadata fixes, constants/comments | Haiku 4.5 | Fast, cheap, deterministic for well-scoped single-file tasks |
| Phase closeout validation, build scripts, grep/search sweeps | Haiku 4.5 | No depth needed; high-volume, repetitive |
| Multi-file refactors touching invariants or ADRs | Opus 4.6 | Wrong assumption mid-refactor is expensive to undo |
| Research + web search (Section 0) | Opus 4.6 | Synthesis across large heterogeneous sources requires full context window |
| Parallel execution of implementation tasks | Codex 5.3 (if available) | Same assignments as Sonnet; sandboxed session per task |

**Decision rule (plain language):** Start with Sonnet. Upgrade to Opus if the task touches
an ADR, crosses 3+ files, or requires a judgment call that affects system design. Downgrade
to Haiku if the task is one file, has a clear template, and success is binary.

---

### Section 4 — Phased Work Plan with Mega-Prompts

**Three new phases.** Each task is a complete mega-prompt: context block, exact goal, files
to read first, expected output, success criteria, and validation command.

#### Stage 15 — Close the Gap (Priority: Now)

Goal: close all open Stage 14 findings and achieve `draft-pre-release` readiness.

| Task | Model | Parallel? |
|------|-------|-----------|
| 15-A: Bind `emit_dir_azimuth` + `emit_dir_elevation` relay/attachment/UI | Sonnet 4.6 | Yes — with 15-B |
| 15-B: Bind `phys_vel_x/y/z` relay/attachment/UI | Sonnet 4.6 | Yes — with 15-A |
| 15-C: Author ADR or explicit defer record for 15-A/B | Opus 4.6 | After 15-A/B |
| 15-D: Execute `DEV-01..DEV-06` manual DAW acceptance | Human | After 15-C |
| 15-E: Update implementation-traceability.md | Haiku 4.5 | After 15-A/B |
| 15-F: Cut `draft-pre-release` tag + update CHANGELOG | Sonnet 4.6 | After 15-D |

#### Stage 16 — Hardening (Priority: Next)

Goal: QA coverage completeness, RT-safety audit, allocation tracking, research integration.

| Task | Model | Parallel? |
|------|-------|-----------|
| 16-A: QA scenario coverage expansion (per Section 2d matrix gaps) | Sonnet 4.6 | Yes — with 16-B, 16-C |
| 16-B: RT-safety audit — processBlock allocation scan | Haiku 4.5 | Yes — with 16-A, 16-C |
| 16-C: Research integration — evaluate Section 0 library candidates | Opus 4.6 | Yes — with 16-A, 16-B |
| 16-D: Three.js viewport gap assessment + ADR for post-v1 features | Opus 4.6 | After 16-C |
| 16-E: `emit_dir_*` directivity DSP coverage in QA scenarios | Sonnet 4.6 | After 16-A |

#### Stage 17 — GA Readiness (Priority: After Stage 16)

Goal: device profile signoff, changelog freeze, `draft` → `ga` promotion.

| Task | Model | Parallel? |
|------|-------|-----------|
| 17-A: Portable device profile acceptance (`DEV-01..DEV-06` headphone/laptop) | Human + Sonnet 4.6 | Sequential |
| 17-B: Final docs freshness gate (`./scripts/validate-docs-freshness.sh`) | Haiku 4.5 | After 17-A |
| 17-C: CHANGELOG freeze + version bump | Sonnet 4.6 | After 17-B |
| 17-D: Promote `draft-pre-release` → `ga` | Human | After 17-C |

---

### Section 5 — Parallel Agent Dependency Graph

**Purpose:** Show exactly which tasks can start immediately in parallel, which are blocked,
and which require human steps.

**Format:** Mermaid `graph TD` with:
- Nodes colored by model (Opus = red, Sonnet = blue, Haiku = green, Human = gold)
- Solid edges = hard dependency
- Dashed edges = soft dependency (can start but benefits from predecessor)
- Tasks with zero incoming solid edges = safe to run right now in separate sessions

**Immediate parallel-safe tasks (zero blockers):**
- 15-A (Sonnet) — emit_dir relay/attachment
- 15-B (Sonnet) — phys_vel relay/attachment
- Section 0 research (Opus) — ecosystem landscape

Everything else gates on one of these three.

---

## Execution Notes

### How to Run a Mega-Prompt

Each mega-prompt in Section 4 is structured for copy-paste into a new Claude Code session:

```
CONTEXT: [project summary, relevant files, current phase]
GOAL: [exact deliverable]
READ FIRST: [file list]
CONSTRAINTS: [invariants, ADRs, thread-safety rules]
OUTPUT: [expected artifact]
SUCCESS CRITERIA: [how to verify]
VALIDATION: [command to run]
```

### On Reinventing the Wheel

Section 0 research findings gate the Stage 16 decisions. The standing rule: if a library
covers ≥80% of a LocusQ component's function with acceptable licensing and JUCE compatibility,
the default recommendation is `use` or `augment`, not `build`. The only exception is where
LocusQ's lock-free SceneGraph integration requires tight coupling that an external library
cannot provide without allocation on the audio thread.

### On Learning This Space

Every finding in Sections 2a–2d includes a "what this means" sentence in plain language
before the technical detail. Section 0 has a one-sentence definition for every concept.
The goal is that reading the full review teaches you the domain, not just the project state.

---

## Approval Record

| Date | Reviewer | Decision |
|------|----------|----------|
| 2026-02-20 | User | Approved via session dialogue |
