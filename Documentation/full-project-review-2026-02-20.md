Title: LocusQ Full Project Review
Document Type: Review Report
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# LocusQ Full Project Review

> This document covers the complete LocusQ project as of 2026-02-20. It is intended to
> be readable by someone learning this space: every technical concept gets a plain-language
> sentence before the detail. Findings include an opinionated disposition. Mega-prompts
> are copy-pasteable into a new Claude Code session.

---

## Section 0 — Research & Ecosystem Landscape
*[Populated in Task 8 from research agent output]*

---

## Section 1 — System Map

### What this section is

Before reading any findings, these four diagrams give you a complete mental model of what
LocusQ is and how its parts relate. Read them top-to-bottom once, then refer back when
a finding references a specific component.

### 1a. High-Level Architecture

> **Plain language:** LocusQ is one plugin binary that can run in three roles. Multiple
> "Emitter" instances send spatial position data to a central "scene" that one "Renderer"
> instance reads to produce quad spatial output. A "Calibrate" instance measures the room.

```mermaid
graph TD
    subgraph DAW["DAW Process"]
        subgraph Emitters["Emitter Instances (N tracks)"]
            E1["Emitter 1\nPluginProcessor\n(mode=emitter)"]
            E2["Emitter 2\nPluginProcessor\n(mode=emitter)"]
            EN["Emitter N\n..."]
        end
        SG["SceneGraph\nSingleton\nEmitterSlot[0..255]\nlock-free double-buffer"]
        R["Renderer Instance\nPluginProcessor\n(mode=renderer)\nmaster bus"]
        CAL["Calibrate Instance\nPluginProcessor\n(mode=calibrate)\nstandalone"]
        PE["PhysicsEngine\ntimer thread\n30–240 Hz"]
        WV["WebView UI\nHTML + Three.js\nwindow.__JUCE__ bridge"]
        E1 -->|"publish slot (atomic)"| SG
        E2 -->|"publish slot (atomic)"| SG
        EN -->|"publish slot (atomic)"| SG
        SG -->|"read all slots"| R
        CAL -->|"RoomProfile (atomic swap)"| SG
        PE -->|"position (atomic)"| SG
        SG -->|"scene snapshot (timer)"| WV
        WV -->|"param changes (bridge)"| E1
        WV -->|"param changes (bridge)"| R
    end
```

### 1b. Data Flow & Thread Model

> **Plain language:** Audio plugins have strict real-time rules — the audio thread cannot
> wait for anything. LocusQ uses lock-free data structures to let the physics timer thread
> and the audio thread share data without ever blocking each other.

```mermaid
graph LR
    subgraph AudioThread["Audio Thread (real-time, no blocking)"]
        AT["processBlock()\n• read APVTS params\n• read SceneGraph slots\n• render/spatialize\n• write position"]
    end
    subgraph PhysicsThread["Physics Timer Thread (30–240 Hz)"]
        PT["tick()\n• integrate forces\n• collision detect\n• write position"]
    end
    subgraph UIThread["UI Timer Thread (30–60 fps)"]
        UT["timerCallback()\n• serialize scene to JSON\n• evaluateJavascript"]
    end
    subgraph RegistrationPath["Registration (rare, startup/shutdown only)"]
        RP["registerEmitter()\nunregisterEmitter()\nSpinLock (acceptable)"]
    end
    SG[("SceneGraph\nEmitterSlot[N]\ndouble-buffer")]
    AT -->|"read: memory_order_acquire"| SG
    PT -->|"write: atomic swap"| SG
    UT -->|"read: memory_order_acquire"| SG
    RP -->|"write: SpinLock"| SG
    style AT fill:#1a3a5c
    style PT fill:#3a1a5c
    style UT fill:#1a5c3a
    style RP fill:#5c3a1a
```

### 1c. Component Dependency Graph

> **Plain language:** Before changing any file, know what else depends on it. This graph
> shows the blast radius of every source file.

```mermaid
graph TD
    subgraph Shell["Plugin Shell"]
        PP["PluginProcessor.cpp/h"]
        PE2["PluginEditor.cpp/h"]
    end
    subgraph DSPCore["DSP Core"]
        SR["SpatialRenderer.h"]
        VB["VBAPPanner.h"]
        DA["DistanceAttenuator.h"]
        AA["AirAbsorption.h"]
        DP["DopplerProcessor.h"]
        DF["DirectivityFilter.h"]
        SP["SpreadProcessor.h"]
    end
    subgraph RoomAcoustics["Room Acoustics"]
        ER["EarlyReflections.h"]
        FD["FDNReverb.h"]
    end
    subgraph Physics["Physics"]
        PHY["PhysicsEngine.h"]
    end
    subgraph Calibration["Calibration"]
        TSG["TestSignalGenerator.h"]
        IRC["IRCapture.h"]
        RA["RoomAnalyzer.h"]
        RPS["RoomProfileSerializer.h"]
        CE["CalibrationEngine.h"]
    end
    subgraph DataModel["Data Model"]
        SG2["SceneGraph.h"]
        KT["KeyframeTimeline.h/cpp"]
    end
    PP --> SG2
    PP --> PHY
    PP --> CE
    PP --> KT
    SR --> VB
    SR --> DA
    SR --> AA
    SR --> DP
    SR --> DF
    SR --> SP
    SR --> ER
    SR --> FD
    PP --> SR
    CE --> TSG
    CE --> IRC
    CE --> RA
    CE --> RPS
    PE2 --> PP
```

### 1d. Implementation Phase Timeline

> **Plain language:** LocusQ was built in staged phases, each adding a new capability.
> This chart shows what's done, what's current, and what's ahead.

```mermaid
gantt
    title LocusQ Implementation Phases
    dateFormat YYYY-MM-DD
    axisFormat %b %d

    section Complete
    2.1 Foundation & Scene Graph       :done, 2026-02-17, 1d
    2.2 Spatialization Core            :done, 2026-02-17, 1d
    2.3 Room Calibration               :done, 2026-02-18, 1d
    2.4 Physics Engine                 :done, 2026-02-18, 1d
    2.5 Room Acoustics & Advanced DSP  :done, 2026-02-19, 1d
    2.6 Keyframe Animation & Polish    :done, 2026-02-19, 1d
    2.7 UI Parity & Host Acceptance    :done, 2026-02-19, 1d
    2.8 Output Layout & Routing        :done, 2026-02-19, 1d
    2.9–2.13 QA / Hardening Stages     :done, 2026-02-20, 1d
    2.14 Stage 14 Closeout             :done, 2026-02-20, 1d

    section Current
    Stage 15 Close the Gap             :active, 2026-02-20, 3d

    section Ahead
    Stage 16 Hardening                 :2026-02-23, 4d
    Stage 17 GA Readiness              :2026-02-27, 3d
```

---

## Section 2 — Domain Reviews

### 2a. Architecture Review

**Current state:** LocusQ's architecture centers on a process-wide `SceneGraph` singleton
with lock-free double-buffered `EmitterSlot[0..255]`, a physics timer thread (30–240 Hz),
and a single `SpatialRenderer` consuming all emitter state per audio block. The design
is aligned to ADR-0002 (single-process routing with ephemeral audio fast-path) and ADR-0003
(deterministic authority precedence: DAW/APVTS > timeline > physics). The inter-instance
communication model is novel for JUCE plugins and well-suited to the quad-panner use case.

**Verdict:** Sound. One low-severity design note. Two items explicitly acceptable.

#### Findings

| ID | Severity | Finding | Disposition |
|----|----------|---------|-------------|
| A-01 | Low | SceneGraph `std::shared_ptr<RoomProfile>` uses `std::atomic_store`/`load` (deprecated C++20) | acceptable |
| A-02 | Info | `rendererRegistered` is plain `bool` protected by SpinLock, not atomic | acceptable |
| A-03 | Info | `computeEmitterInteractionForce` reads other emitters' position with 1-frame lag | acceptable |

#### A-01: `std::atomic_store` on `shared_ptr` is deprecated in C++20

> **What this means:** The way LocusQ swaps room calibration data between threads uses a
> C++ pattern that still works but is officially deprecated. The replacement (`std::atomic<shared_ptr>`)
> requires C++20, which JUCE 8 does not mandate.

`SceneGraph.h:191-198` uses `std::atomic_store(&currentRoomProfile, newProfile)`. This is correct
and safe but will generate compiler warnings under `-std=c++20`. Since JUCE 8 targets C++17,
this is acceptable today. When JUCE moves to C++20, migrate to `std::atomic<std::shared_ptr<RoomProfile>>`.

**Recommendation:** No action for v1. Add to post-v1 tech debt list.

#### A-02: `rendererRegistered` bool under SpinLock

> **What this means:** The flag tracking whether a renderer exists is a normal boolean, not
> an atomic one, but it's always accessed under a SpinLock so this is safe.

`SceneGraph.h:264` — `bool rendererRegistered = false` is only read/written inside
`registrationLock` scope. The unlocked `isRendererRegistered()` at line 178 is a read-only
fast path that tolerates staleness (renderer registration is rare and stable).

**Recommendation:** Acceptable as-is. No change needed.

#### A-03: Interaction force reads with 1-frame temporal lag

> **What this means:** When computing forces between emitters, each emitter sees the others'
> positions from the previous audio callback. This is intentional — it avoids needing
> synchronized reads across multiple slots.

`PluginProcessor.cpp:150-152` — documented in-code with explicit rationale. The 1-frame lag
at 44.1kHz/512 samples is ~11.6ms, well within perceptual tolerance for spatial interaction.

**Recommendation:** Acceptable. Already documented in source.

---

### 2b. Code Review

**Current state:** The codebase is 22 source files (19 headers, 3 .cpp). `processBlock`
is RT-safe: no heap allocations, no locks, all parameter reads via `getRawParameterValue()->load()`.
Serialization and UI bridge code correctly use heap allocation outside the audio thread.
The Stage 14 medium finding on `emit_dir_azimuth`/`emit_dir_elevation` is now resolved —
relay, attachment, and UI are fully wired. `phys_vel_x/y/z` remain unwired.

**Verdict:** Clean RT path. One medium finding (phys_vel UI gap). One resolved finding confirmed.

#### Findings

| ID | Severity | Finding | Disposition |
|----|----------|---------|-------------|
| C-01 | Medium | `phys_vel_x`, `phys_vel_y`, `phys_vel_z` have no relay/attachment/UI | fix now (Stage 15-B) |
| C-02 | Resolved | `emit_dir_azimuth`, `emit_dir_elevation` now fully wired | closed |
| C-03 | Info | `new juce::DynamicObject()` in serialization paths (lines 965–1680) | acceptable |
| C-04 | Info | Parameter creation uses `std::vector<unique_ptr>::push_back` (line 1859+) | acceptable |

#### C-01: `phys_vel_x/y/z` — DSP-backed but UI-invisible

> **What this means:** The initial velocity parameters (how fast and in which direction an
> emitter is "thrown") exist in the DSP engine but cannot be seen or edited in the production
> UI. Users must use DAW automation to change them.

Evidence:
- APVTS definition: `PluginProcessor.cpp` lines ~2046-2065
- DSP read: `PluginProcessor.cpp` lines ~698-700
- `PluginEditor.h`: no `physVelXRelay` / `physVelYRelay` / `physVelZRelay`
- `index.js`: no `phys_vel_x` in sliderStates
- `index.html`: no `val-vel-x` control rows

**Recommendation:** Wire in Stage 15-B. Follow the `phys_friction` relay/attachment/UI pattern.

#### C-02: `emit_dir_azimuth` + `emit_dir_elevation` — now resolved

> **What this means:** The directivity aim parameters are now fully exposed in the production UI.

Evidence:
- Relay: `PluginEditor.h:75-76` (`dirAzimuthRelay`, `dirElevationRelay`)
- Attachment: `PluginEditor.cpp:343-346`
- UI sliderStates: `index.js:261-262`
- UI bindValueStepper: `index.js:1663-1665`
- UI valueChangedEvent: `index.js:2004-2010`
- HTML controls: `index.html:568-569`

**Recommendation:** Closed. Stage 14 medium finding is fully resolved.

#### C-03: Heap allocation in serialization paths

> **What this means:** The scene-to-JSON serialization code allocates memory on the heap,
> but this code runs on the UI timer thread, not the audio thread, so it's safe.

`new juce::DynamicObject()` at lines 965, 1031, 1080, 1092, 1099, etc. — all inside
`getSceneStateJSON()`, `serializeTimeline()`, or WebView command handlers. These run in
`timerCallback()` or message thread context. No RT violation.

**Recommendation:** Acceptable. No change needed.

#### C-04: Parameter tree construction uses `push_back`

> **What this means:** The parameter list is built with vector push_back during plugin
> construction (once at startup), not during audio processing.

`createParameterLayout()` at line 1859+ — runs once during `AudioProcessor` construction.
Not an RT path.

**Recommendation:** Acceptable. No change needed.

---

### 2c. Design Review

**Current state:** The production UI (`index.js` + `index.html`) exposes all emitter-mode
parameters that have relays, including the newly wired directivity aim controls. The UI
resilience contract (BOOT_START -> RUNNING) is implemented. The Three.js viewport is a
placeholder — the spec in `.ideas/architecture.md` Section 7 describes room wireframe,
speaker positions, emitter objects, motion trails, velocity vectors, and interactive drag/rotate,
but the production UI implements only the control panel, not the full viewport.

**Verdict:** Control panel is complete. Viewport is placeholder. One medium finding.

#### Findings

| ID | Severity | Finding | Disposition |
|----|----------|---------|-------------|
| D-01 | Medium | Three.js viewport not implemented — only control panel UI exists | explicit defer (post-v1) |
| D-02 | Low | `phys_vel_x/y/z` not in production UI | fix now (Stage 15-B) |
| D-03 | Info | Keyframe editor UI not implemented | explicit defer (post-v1) |

#### D-01: Three.js 3D viewport is placeholder

> **What this means:** The architecture spec describes a full 3D viewport with room wireframe,
> speaker cones, draggable emitter objects, motion trails, and orbit controls. The production
> UI has the control panel and parameter bindings but not the 3D visualization.

This is a large feature (~500-1000 LOC of Three.js) that is not blocking v1 functionality.
All spatial parameters are editable via the control panel. The viewport adds visual feedback
but is not required for audio correctness.

**Recommendation:** Explicit defer to post-v1. Record in ADR-0008 (Stage 16-D).

> **See Section 0e:** Three.js spatial audio visualization patterns may inform viewport scope.

#### D-02: Initial velocity controls missing from UI

> **What this means:** Users cannot see or adjust the throw velocity from the plugin UI.

Same as C-01. Fixed in Stage 15-B.

#### D-03: Keyframe timeline editor not in production UI

> **What this means:** The keyframe animation system works via internal presets and DAW
> automation, but there is no visual timeline editor in the production UI.

Like D-01, this is a significant UI feature that is not blocking v1 audio functionality.
Default keyframe presets are loaded automatically.

**Recommendation:** Explicit defer to post-v1.

---

### 2d. QA Review

**Current state:** 43 scenario files covering DSP components, output layouts, snapshot
migration, physics, RT safety, and CPU budget. Automated lanes are green. Manual DAW
acceptance (DEV-01..DEV-06) remains unexecuted.

**Verdict:** Strong automated coverage. Four component gaps. Manual acceptance still open.

#### Coverage Matrix

| Component | Scenario File(s) | Gap? |
|-----------|-----------------|------|
| VBAPPanner | locusq_renderer_spatial_output.json | — |
| DistanceAttenuator | locusq_renderer_distance_attenuation.json | — |
| AirAbsorption | (covered indirectly by quality/distance scenarios) | **gap: no dedicated scenario** |
| FDNReverb | locusq_25_room_size_small/large.json | partial |
| DopplerProcessor | locusq_25_doppler_motion.json | — |
| DirectivityFilter | locusq_25_directivity_focus.json | — |
| SpreadProcessor | locusq_25_spread_diffuse.json | — |
| PhysicsEngine | locusq_24_physics_spatial_motion/zero_g_drift.json | — |
| computeEmitterInteractionForce | locusq_multi_emitter_interaction.json | — |
| CalibrationEngine | (no scenario) | **gap** |
| KeyframeTimeline | locusq_26_animation_internal_smoke.json | — |
| emit_dir DSP path | (no dedicated scenario) | **gap** |
| Output layouts | locusq_phase_2_8_output_layout_*.json (mono/stereo/quad) | — |
| Snapshot migration | locusq_phase_2_11*.json (5 scenarios) | — |
| RT safety | locusq_rt_safety_emitter.json | — |

#### Findings

| ID | Severity | Finding | Disposition |
|----|----------|---------|-------------|
| Q-01 | Medium | DEV-01..DEV-06 manual DAW acceptance unexecuted | fix now (Stage 15-D) |
| Q-02 | Low | No dedicated AirAbsorption scenario | fix next (Stage 16-A) |
| Q-03 | Low | No CalibrationEngine scenario | fix next (Stage 16-A) |
| Q-04 | Low | No emit_dir DSP effect scenario | fix next (Stage 16-E) |

#### Q-01: Manual DAW acceptance rows still open

> **What this means:** Six checks that require a human to plug in headphones, play audio,
> and verify output have not been run. These are the portable-device profile gates from
> ADR-0006.

**Recommendation:** Execute in Stage 15-D. Blocks draft-pre-release promotion.

#### Q-02: AirAbsorption lacks dedicated scenario

> **What this means:** Air absorption (high-frequency rolloff with distance) is tested
> indirectly by other scenarios but has no scenario that specifically validates the filter
> cutoff vs distance relationship.

**Recommendation:** Author scenario in Stage 16-A.

#### Q-03: CalibrationEngine has no automated scenario

> **What this means:** The room measurement system has no automated test scenario. It was
> validated manually during Phase 2.3 but has no regression coverage.

**Recommendation:** Author scenario in Stage 16-A.

#### Q-04: Directivity aim effect has no dedicated scenario

> **What this means:** The directivity aim parameters now have UI exposure (C-02 resolved)
> but no scenario verifying the DSP effect of aim direction on spatial output.

**Recommendation:** Author `locusq_directivity_aim.json` in Stage 16-E.

---

## Section 3 — Model Assignment Rationale

> **What this means:** Different Claude models have different strengths and costs. Choosing
> the right model per task saves time and money without sacrificing quality. These rules
> are calibrated for LocusQ's specific task mix.

### Decision Rule (Plain Language)

Start with **Sonnet 4.6**. Upgrade to **Opus 4.6** if the task:
- touches an ADR or invariants.md
- crosses 3+ files with causal dependencies
- requires a judgment call that affects system design
- synthesises research from many external sources

Downgrade to **Haiku 4.5** if the task:
- touches one file
- has a clear template to follow
- success is binary (it either compiles / passes / matches a pattern, or it doesn't)
- is high-volume and repetitive

### Assignment Table

| Task Type | Model | Reasoning |
|-----------|-------|-----------|
| Architecture decisions, ADR authoring | Opus 4.6 | Multi-file causal reasoning; wrong call is expensive |
| Cross-cutting design (invariant changes) | Opus 4.6 | Must hold entire constraint graph in context |
| Research synthesis (Section 0) | Opus 4.6 | Heterogeneous sources; needs judgment on relevance |
| DSP implementation (C++) | Sonnet 4.6 | Clear spec; best cost/quality ratio |
| UI wiring (JS + HTML + C++ relay) | Sonnet 4.6 | Follows a template; needs to see 4 files at once |
| Code review fixes (bounded scope) | Sonnet 4.6 | Single finding -> single fix |
| QA scenario JSON authoring | Haiku 4.5 | Template-driven; parameters are known |
| Doc metadata fixes | Haiku 4.5 | Pattern match; binary correctness |
| Constant/comment additions | Haiku 4.5 | Single file; deterministic |
| Phase closeout validation | Haiku 4.5 | Run scripts; check output |
| Build scripts, grep sweeps | Haiku 4.5 | No reasoning depth needed |
| Parallel Codex 5.3 tasks | Codex 5.3 | Same as Sonnet; separate sandboxed session |

### Cost Intuition

Running Opus when Haiku suffices costs ~20x more per token. For LocusQ's Stage 15-17
work, roughly 60% of tasks are Haiku-appropriate. Defaulting everything to Opus would
be both wasteful and unnecessary — the quality ceiling is determined by spec clarity,
not model tier, for well-scoped tasks.

---

## Section 4 — Phased Work Plan with Mega-Prompts

> **What this means:** Each task below is a complete instruction set for Claude Code.
> Copy the block under "Mega-Prompt" into a new Claude Code session. The session will
> know exactly what to do without needing prior context.

---

### Stage 15 — Close the Gap

**Goal:** Close all open Stage 14 findings. Reach `draft-pre-release` readiness.
**Prerequisite:** Current build passes `./scripts/build-and-install-mac.sh`.

---

#### Task 15-A: Bind `emit_dir_azimuth` + `emit_dir_elevation` (Relay / Attachment / UI)

**Status: RESOLVED.** This task was completed prior to the review. Evidence confirmed
in Section 2b finding C-02. No mega-prompt needed — skip to 15-B.

---

#### Task 15-B: Bind `phys_vel_x`, `phys_vel_y`, `phys_vel_z` (Relay / Attachment / UI)

**Model:** Sonnet 4.6
**Parallel with:** None (15-A is already done)
**Blocks:** 15-C, 15-E

**What this task does (plain language):** The initial velocity parameters set how fast and
in which direction an emitter is launched when you press "throw." They exist in the DSP
but are currently invisible in the UI.

**Mega-Prompt:**

````
CONTEXT:
LocusQ is a JUCE 8 spatial audio plugin. PluginEditor.h declares parameter relays
(juce::WebSliderRelay, etc.) that bridge APVTS parameters to the WebView UI.
PluginEditor.cpp creates WebSliderParameterAttachment objects that connect each relay
to its parameter. Source/ui/public/js/index.js registers sliderStates and toggleStates
using Juce.getSliderState() / Juce.getToggleState(), binds value steppers, adds
valueChangedEvent listeners, and syncs initial values. Source/ui/public/index.html
contains the control rows.

CURRENT GAP:
phys_vel_x, phys_vel_y, phys_vel_z are float sliders (range -50..50 m/s, default 0).
They are read in DSP at PluginProcessor.cpp ~lines 698–700 but have no relay/attachment/UI.

GOAL:
Wire phys_vel_x, phys_vel_y, phys_vel_z end-to-end. Follow the pattern of phys_friction.

READ FIRST:
1. Source/PluginEditor.h lines 77–90 (physics relay group)
2. Source/PluginEditor.cpp lines 344–360 (physics attachment creation)
3. Source/ui/public/js/index.js lines 265–278 (physics sliderStates)
4. Source/ui/public/js/index.js lines 1662–1666 (physics bindValueStepper)
5. Source/ui/public/js/index.js lines 2008–2020 (physics valueChangedEvent)
6. Source/ui/public/index.html lines 594–602 (physics-advanced disclosure section)

CONSTRAINTS:
- Same member order rules: relays before webView, attachments after addAndMakeVisible.
- Place relays in PluginEditor.h after physResetRelay.
- Place HTML controls inside the physics-advanced disclosure div, after the Direction row.
- Naming convention:
    relays: physVelXRelay / physVelYRelay / physVelZRelay
    attachments: physVelXAttachment / physVelYAttachment / physVelZAttachment
    HTML ids: val-vel-x / val-vel-y / val-vel-z
    sliderStates keys: phys_vel_x / phys_vel_y / phys_vel_z

EXACT CHANGES:

1. PluginEditor.h — add after physResetRelay (in relay section):
   juce::WebSliderRelay physVelXRelay { "phys_vel_x" };
   juce::WebSliderRelay physVelYRelay { "phys_vel_y" };
   juce::WebSliderRelay physVelZRelay { "phys_vel_z" };

2. PluginEditor.h — add after physResetAttachment (in attachments section):
   std::unique_ptr<juce::WebSliderParameterAttachment> physVelXAttachment;
   std::unique_ptr<juce::WebSliderParameterAttachment> physVelYAttachment;
   std::unique_ptr<juce::WebSliderParameterAttachment> physVelZAttachment;

3. PluginEditor.cpp — add after physResetAttachment = ... creation:
   physVelXAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
       *audioProcessor.apvts.getParameter ("phys_vel_x"), physVelXRelay);
   physVelYAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
       *audioProcessor.apvts.getParameter ("phys_vel_y"), physVelYRelay);
   physVelZAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
       *audioProcessor.apvts.getParameter ("phys_vel_z"), physVelZRelay);

4. Source/ui/public/js/index.js — add to sliderStates after phys_friction:
   phys_vel_x: Juce.getSliderState("phys_vel_x"),
   phys_vel_y: Juce.getSliderState("phys_vel_y"),
   phys_vel_z: Juce.getSliderState("phys_vel_z"),

5. Source/ui/public/index.html — add inside physics-advanced div, after Direction row:
   <div class="control-row"><span class="control-label">Init Vel X</span><span class="control-value" id="val-vel-x">0.0<span class="control-unit">m/s</span></span></div>
   <div class="control-row"><span class="control-label">Init Vel Y</span><span class="control-value" id="val-vel-y">0.0<span class="control-unit">m/s</span></span></div>
   <div class="control-row"><span class="control-label">Init Vel Z</span><span class="control-value" id="val-vel-z">0.0<span class="control-unit">m/s</span></span></div>

6. Source/ui/public/js/index.js — add to bindValueStepper block after val-friction:
   bindValueStepper("val-vel-x", sliderStates.phys_vel_x,
       { step: 1.0, min: -50.0, max: 50.0, roundDigits: 1 });
   bindValueStepper("val-vel-y", sliderStates.phys_vel_y,
       { step: 1.0, min: -50.0, max: 50.0, roundDigits: 1 });
   bindValueStepper("val-vel-z", sliderStates.phys_vel_z,
       { step: 1.0, min: -50.0, max: 50.0, roundDigits: 1 });

7. Source/ui/public/js/index.js — add to valueChangedEvent block after phys_friction:
   sliderStates.phys_vel_x.valueChangedEvent.addListener(() => {
       updateValueDisplay("val-vel-x",
           sliderStates.phys_vel_x.getScaledValue().toFixed(1), "m/s");
   });
   sliderStates.phys_vel_y.valueChangedEvent.addListener(() => {
       updateValueDisplay("val-vel-y",
           sliderStates.phys_vel_y.getScaledValue().toFixed(1), "m/s");
   });
   sliderStates.phys_vel_z.valueChangedEvent.addListener(() => {
       updateValueDisplay("val-vel-z",
           sliderStates.phys_vel_z.getScaledValue().toFixed(1), "m/s");
   });

8. Source/ui/public/js/index.js — add to initial sync block:
   updateValueDisplay("val-vel-x",
       sliderStates.phys_vel_x.getScaledValue().toFixed(1), "m/s");
   updateValueDisplay("val-vel-y",
       sliderStates.phys_vel_y.getScaledValue().toFixed(1), "m/s");
   updateValueDisplay("val-vel-z",
       sliderStates.phys_vel_z.getScaledValue().toFixed(1), "m/s");

OUTPUT: 4 files modified

SUCCESS CRITERIA:
- grep "physVelXRelay" Source/PluginEditor.h -> found before webView declaration
- grep "phys_vel_x" Source/ui/public/js/index.js -> in sliderStates, bindValueStepper,
  valueChangedEvent, and initial sync
- grep "val-vel-x" Source/ui/public/index.html -> found inside physics-advanced div

VALIDATION:
./scripts/build-and-install-mac.sh

COMMIT:
git commit -m "feat(stage15): wire phys_vel_x/y/z relay/attachment/UI"
````

---

#### Task 15-C: Author ADR-0007 for emit_dir and phys_vel UI Exposure Decision

**Model:** Opus 4.6
**Blocked by:** 15-B
**Blocks:** 15-E

**What this task does (plain language):** An Architecture Decision Record (ADR) documents
*why* a decision was made, so future developers don't accidentally undo it. This ADR
records that directivity aim and initial velocity are now UI-exposed in v1.

**Mega-Prompt:**

````
CONTEXT:
LocusQ uses Architecture Decision Records (ADRs) in Documentation/adr/. The last is
ADR-0006. Each ADR has Title, Status, Context, Decision, Consequences sections and
the project metadata header (Title, Document Type, Author, Created Date, Last Modified Date).

GOAL:
Write Documentation/adr/ADR-0007-emitter-directivity-velocity-ui-exposure.md.

READ FIRST:
1. Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md
   (for format reference)
2. Documentation/invariants.md (check if any invariant is affected)
3. Documentation/implementation-traceability.md (to understand what traceability means here)

CONTENT:
- Status: Accepted
- Context: emit_dir_azimuth, emit_dir_elevation, phys_vel_x/y/z were DSP-backed but not
  exposed in the production UI. Stage 14 review flagged this as a medium-severity gap.
  Stage 15 tasks 15-A and 15-B close this gap.
- Decision: Expose all five parameters via relay/attachment/UI in the production index.js
  and index.html. No defer — v1 ships with full directivity aim and initial velocity
  editability.
- Consequences: (a) Users can now set directivity aim direction and throw velocity without
  DAW automation. (b) implementation-traceability.md must be updated (Task 15-E).
  (c) QA should add a scenario covering directivity aim effect on spatial output.

OUTPUT: Documentation/adr/ADR-0007-emitter-directivity-velocity-ui-exposure.md

COMMIT:
git commit -m "docs(adr): add ADR-0007 emit_dir and phys_vel UI exposure decision"
````

---

#### Task 15-D: Execute Manual DAW Acceptance (DEV-01..DEV-06)

**Model:** Human task (you execute; Claude Code assists with build and log capture)
**Blocked by:** 15-C

**What this task does (plain language):** These are the portable-device profile checks —
verifying that LocusQ works on laptop speakers, built-in microphone, and headphones.
They cannot be automated; a human must plug in headphones, press play, and listen.

**Checklist:**
- [ ] Read `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` rows DEV-01..DEV-06
- [ ] Run `./scripts/build-and-install-mac.sh` first
- [ ] Execute each check in REAPER (or Logic) with the specified device profile
- [ ] Fill in the result column for each row
- [ ] Save the file

**Assist mega-prompt (run in parallel while executing checks):**

````
CONTEXT:
TestEvidence/phase-2-7a-manual-host-ui-acceptance.md contains manual acceptance rows
DEV-01..DEV-06 for portable device profile checks (laptop speakers, mic, headphones).

GOAL:
After the human fills in the result column for each row, update:
1. TestEvidence/validation-trend.md — add a new trend entry for Stage 15 DAW acceptance
2. TestEvidence/build-summary.md — add a summary line for this run

READ FIRST:
1. TestEvidence/phase-2-7a-manual-host-ui-acceptance.md (full)
2. TestEvidence/validation-trend.md (last 5 entries for format)
3. TestEvidence/build-summary.md (last 3 entries for format)

OUTPUT: Both trend files updated.

COMMIT:
git commit -m "docs(evidence): record Stage 15 manual DAW acceptance results"
````

---

#### Task 15-E: Update implementation-traceability.md

**Model:** Haiku 4.5
**Blocked by:** 15-B, 15-C

**Mega-Prompt:**

````
CONTEXT:
Documentation/implementation-traceability.md maps every parameter to its APVTS
definition, DSP usage, and UI relay/attachment. Five parameters are now newly wired:
emit_dir_azimuth, emit_dir_elevation (already done), phys_vel_x, phys_vel_y, phys_vel_z.

GOAL:
Update the traceability table rows for these five parameters to show:
- Relay: PluginEditor.h (dirAzimuthRelay/dirElevationRelay/physVelXRelay etc.)
- Attachment: PluginEditor.cpp
- UI: index.js (sliderStates + bindValueStepper) / index.html (val-dir-azimuth etc.)

READ FIRST:
1. Documentation/implementation-traceability.md
2. Source/PluginEditor.h (to confirm exact relay names)

OUTPUT: Documentation/implementation-traceability.md updated.

VALIDATION:
grep "emit_dir_azimuth" Documentation/implementation-traceability.md
# Must show relay + UI columns filled

COMMIT:
git commit -m "docs(traceability): update emit_dir and phys_vel UI binding coverage"
````

---

#### Task 15-F: Cut draft-pre-release Tag

**Model:** Sonnet 4.6
**Blocked by:** 15-D (manual acceptance must pass)

**Mega-Prompt:**

````
CONTEXT:
LocusQ is at draft-pre-release hold per Documentation/stage14-comprehensive-review-2026-02-20.md.
Stage 15 has closed: emit_dir/phys_vel UI gap, ADR-0007 recorded, manual DAW acceptance
complete. Automated lanes are green.

GOAL:
1. Update CHANGELOG.md with Stage 15 entries (15-A resolved, 15-B, 15-C close-out).
2. Update status.json: set notes field to "draft-pre-release candidate".
3. Verify ./scripts/validate-docs-freshness.sh passes.
4. Create annotated git tag v0.15.0-draft.

READ FIRST:
1. CHANGELOG.md (last 3 entries for format)
2. status.json
3. Documentation/stage14-comprehensive-review-2026-02-20.md (release recommendation section)

CONSTRAINTS:
- Do NOT push to remote. Tag locally only. User will push when ready.
- Do NOT modify status.json current_phase field.

VALIDATION:
./scripts/validate-docs-freshness.sh
git tag --list | grep draft

COMMIT:
git commit -m "chore(release): Stage 15 closeout — draft-pre-release candidate"
git tag -a v0.15.0-draft -m "Stage 15 closeout: emit_dir/phys_vel UI, manual DAW acceptance"
````

---

### Stage 16 — Hardening

**Goal:** Expand QA coverage, audit RT safety, integrate research findings, scope viewport.
**Prerequisite:** Stage 15 draft tag cut. All five tasks are parallelizable.

---

#### Task 16-A: QA Scenario Expansion

**Model:** Sonnet 4.6 (Haiku 4.5 for individual scenario JSON files)
**Parallel with:** 16-B, 16-C, 16-D

**What this task does (plain language):** Fills the four component coverage gaps identified
in Section 2d: AirAbsorption, CalibrationEngine, KeyframeTimeline, and emit_dir DSP path.

**Mega-Prompt:**

````
CONTEXT:
LocusQ uses JSON scenario files in qa/scenarios/ to define automated test cases.
Each scenario specifies stimulus parameters, expected assertions, and severity.

GOAL:
Author 4 new scenario files, one per coverage gap:
1. qa/scenarios/locusq_air_absorption_distance.json — verify HF rolloff at 3 distances
2. qa/scenarios/locusq_calibration_sweep_capture.json — basic calibration state machine exercise
3. qa/scenarios/locusq_keyframe_loop_playback.json — verify keyframe evaluation over 8s loop
4. qa/scenarios/locusq_emit_dir_spatial_effect.json — directivity aim vs spatial output

READ FIRST:
1. qa/scenarios/locusq_25_directivity_focus.json (template for DSP-focused scenario)
2. qa/scenarios/locusq_24_physics_spatial_motion.json (template for multi-step scenario)
3. Source/AirAbsorption.h (understand cutoff calculation)
4. Source/CalibrationEngine.h (understand state machine)

OUTPUT: 4 new JSON files in qa/scenarios/

VALIDATION:
ls qa/scenarios/locusq_air_absorption_distance.json
ls qa/scenarios/locusq_calibration_sweep_capture.json
ls qa/scenarios/locusq_keyframe_loop_playback.json
ls qa/scenarios/locusq_emit_dir_spatial_effect.json

COMMIT:
git commit -m "test(stage16): add AirAbsorption, Calibration, Keyframe, and emit_dir scenarios"
````

---

#### Task 16-B: RT-Safety Audit

**Model:** Haiku 4.5
**Parallel with:** 16-A, 16-C, 16-D

**What this task does (plain language):** Scans the processBlock call stack for any
accidental heap allocation, logging, or string construction that would violate real-time
constraints.

**Mega-Prompt:**

````
CONTEXT:
LocusQ's processBlock must be allocation-free. The invariant is in Documentation/invariants.md.

GOAL:
Grep PluginProcessor.cpp for patterns that indicate RT violations:
  new , std::vector, .push_back, .resize, std::string(, juce::Logger, std::cout

For each hit, determine if it's in the processBlock call stack or in a non-RT path
(serialization, parameter creation, WebView command handlers).

Report findings as a table:
| Line | Pattern | In processBlock stack? | Severity |

Expected: zero RT-path violations (all hits should be in non-RT paths).

READ FIRST:
1. Source/PluginProcessor.cpp (full file, focus on processBlock and functions it calls)
2. Documentation/invariants.md

OUTPUT: Findings table. If zero RT violations found, document as "clean" with evidence.

COMMIT:
git commit -m "docs(stage16): RT-safety audit — processBlock allocation-free confirmed"
````

---

#### Task 16-C: Research Integration Recommendations

**Model:** Opus 4.6
**Blocked by:** Section 0 research (soft dependency — can start with draft)

**What this task does (plain language):** Reads the Section 0 research findings and produces
3-5 concrete integration recommendations for LocusQ.

**Mega-Prompt:**

````
CONTEXT:
Documentation/full-project-review-2026-02-20.md Section 0 contains ecosystem research
covering spatial audio algorithms, Apple APIs, JUCE ecosystem, plugin standards, and
3D visualization. LocusQ implements custom VBAP, FDN reverb, physics engine, and Doppler.

GOAL:
Compare Section 0 findings against LocusQ's current implementation. Produce 3-5 concrete
integration recommendations, each with:
- What to integrate (library/pattern)
- What it replaces or augments
- LOC estimate for change
- Risk assessment
- Recommended timing (v1.1, v2, or never)

Write recommendations to:
Documentation/research/section0-integration-recommendations-2026-02-20.md

READ FIRST:
1. Documentation/full-project-review-2026-02-20.md (Section 0 — full)
2. Source/SpatialRenderer.h (current DSP chain)
3. Source/FDNReverb.h (current reverb implementation)
4. .ideas/architecture.md (design intent)

OUTPUT: Markdown file with 3-5 opinionated recommendations.

COMMIT:
git commit -m "docs(research): Section 0 integration recommendations for LocusQ"
````

---

#### Task 16-D: Three.js Viewport Gap Assessment + ADR-0008

**Model:** Opus 4.6
**Parallel with:** 16-A, 16-B

**What this task does (plain language):** Compares the architecture spec's UI section
against the actual production UI to list every missing viewport feature, then decides
which gaps are v1-required vs post-v1.

**Mega-Prompt:**

````
CONTEXT:
.ideas/architecture.md Section 7 specifies a full Three.js viewport with room wireframe,
speaker cones, draggable emitters, motion trails, velocity vectors, and orbit controls.
The production UI (index.js + index.html) has the control panel but not the viewport.

GOAL:
1. List every specced viewport feature that is absent from production UI
2. Write Documentation/adr/ADR-0008-viewport-scope-v1-vs-post-v1.md scoping which
   features are v1-required (none, if control panel is sufficient) vs post-v1
3. Be opinionated: the right answer is likely "viewport is post-v1"

READ FIRST:
1. .ideas/architecture.md (Section 7 UI spec)
2. Source/ui/public/js/index.js (viewport-related code)
3. Source/ui/public/index.html (canvas/viewport elements)
4. Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md
   (format reference)

OUTPUT: ADR-0008 file + gap assessment in review notes.

COMMIT:
git commit -m "docs(adr): add ADR-0008 viewport scope — post-v1 deferral"
````

---

#### Task 16-E: Directivity Aim QA Scenario

**Model:** Haiku 4.5
**Blocked by:** 16-A (for template consistency)

**Mega-Prompt:**

````
CONTEXT:
emit_dir_azimuth and emit_dir_elevation are now fully wired. The DirectivityFilter
applies a cardioid-like gain shaping based on the angle between the emitter's aim
direction and each speaker direction.

GOAL:
Author qa/scenarios/locusq_directivity_aim.json covering:
1. Default aim (0,0) — verify baseline spatial output
2. Aim rotated 90 degrees — verify gain shift toward aimed speaker
3. Aim at max elevation — verify vertical directivity effect

Follow the format of locusq_25_directivity_focus.json.

READ FIRST:
1. qa/scenarios/locusq_25_directivity_focus.json
2. Source/DirectivityFilter.h

OUTPUT: qa/scenarios/locusq_directivity_aim.json

COMMIT:
git commit -m "test(stage16): add directivity aim QA scenario"
````

---

### Stage 17 — GA Readiness

**Goal:** Final validation pass, docs freeze, version bump, release.

---

#### Task 17-A: Portable Device Acceptance Repeat

**Model:** Human + Sonnet 4.6
**Blocked by:** Stage 16 complete

**What this task does:** Same as 15-D but with Stage 16 hardening in place. Focus on
headphone profile (ADR-0006 gate).

**Checklist:**
- [ ] Fresh build with `./scripts/build-and-install-mac.sh`
- [ ] Re-execute DEV-01..DEV-06 with focus on headphone and laptop speaker profiles
- [ ] Update validation-trend.md with Stage 17 entry
- [ ] If any check fails, create issue and block GA

---

#### Task 17-B: Docs Freshness Gate

**Model:** Haiku 4.5

**Mega-Prompt:**

````
CONTEXT:
LocusQ docs require metadata freshness. ADR-0005 mandates synchronized updates.

GOAL:
1. Run ./scripts/validate-docs-freshness.sh
2. Resolve any failures (update Last Modified Date metadata on stale files)
3. Verify all metadata headers are present

VALIDATION:
./scripts/validate-docs-freshness.sh (must return 0)

COMMIT:
git commit -m "docs(stage17): resolve docs freshness gate violations"
````

---

#### Task 17-C: CHANGELOG Freeze + Version Bump

**Model:** Sonnet 4.6

**Mega-Prompt:**

````
CONTEXT:
LocusQ is preparing for v1.0.0-ga release.

GOAL:
1. Finalize CHANGELOG.md for v1.0.0-ga (include Stage 15-17 entries)
2. Bump version in CMakeLists.txt (search for VERSION field)
3. Update README.md release section
4. Verify build succeeds with new version

READ FIRST:
1. CHANGELOG.md
2. CMakeLists.txt (version field)
3. README.md

VALIDATION:
./scripts/build-and-install-mac.sh
grep VERSION CMakeLists.txt

COMMIT:
git commit -m "chore(release): CHANGELOG freeze and version bump to v1.0.0-ga"
````

---

#### Task 17-D: GA Promotion

**Model:** Human

**Checklist:**
- [ ] Review `draft-pre-release` tag
- [ ] Resolve any final concerns from Stage 17-A/B/C
- [ ] Push tag `v1.0.0-ga`
- [ ] Publish GitHub release with changelog excerpt
- [ ] Update status.json to record GA milestone

---

## Section 5 — Parallel Agent Dependency Graph

> **What this means:** Tasks with no incoming arrows can start immediately in separate
> Claude Code sessions. Red = Opus 4.6, Blue = Sonnet 4.6, Green = Haiku 4.5,
> Gold = Human. Solid arrow = hard dependency. Dashed = benefits from but can start.

```mermaid
graph TD
    RES["Section 0 Research\nOpus 4.6"]:::opus
    B["15-B phys_vel UI\nSonnet 4.6"]:::sonnet
    C["15-C ADR-0007\nOpus 4.6"]:::opus
    D["15-D Manual DAW\nHuman"]:::human
    E["15-E Traceability\nHaiku 4.5"]:::haiku
    F["15-F Draft Tag\nSonnet 4.6"]:::sonnet
    A16["16-A QA Scenarios\nSonnet 4.6"]:::sonnet
    B16["16-B RT Safety Audit\nHaiku 4.5"]:::haiku
    C16["16-C Research Integration\nOpus 4.6"]:::opus
    D16["16-D Viewport ADR\nOpus 4.6"]:::opus
    E16["16-E Dir Aim Scenario\nHaiku 4.5"]:::haiku
    A17["17-A Device Acceptance\nHuman"]:::human
    B17["17-B Docs Freshness\nHaiku 4.5"]:::haiku
    C17["17-C CHANGELOG\nSonnet 4.6"]:::sonnet
    D17["17-D GA Promotion\nHuman"]:::human

    B --> C
    C --> E
    C --> D
    D --> F
    E --> F
    F --> A16
    F --> B16
    F -.-> C16
    RES -.-> C16
    C16 --> D16
    A16 --> E16
    A16 --> A17
    B16 --> A17
    C16 --> A17
    D16 --> A17
    E16 --> A17
    A17 --> B17
    B17 --> C17
    C17 --> D17

    classDef opus fill:#5c1a1a,color:#fff
    classDef sonnet fill:#1a3a5c,color:#fff
    classDef haiku fill:#1a5c1a,color:#fff
    classDef human fill:#5c4a1a,color:#fff
```

### Updated Start Now (Post 15-A Resolution)

Task 15-A (emit_dir UI) is already complete. The immediate next action is:

| Session | Mega-Prompt | Model | Notes |
|---------|------------|-------|-------|
| 1 | Task 15-B (phys_vel UI) | Sonnet 4.6 | Only remaining code gap |
| 2 | Section 0 Research | Opus 4.6 | Already running in background |

After 15-B completes, run 15-C -> 15-D -> 15-E -> 15-F sequentially.

---
