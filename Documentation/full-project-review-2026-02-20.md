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
