Title: LocusQ UI UX Refinement Component Specs
Document Type: Design Specification
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# Component Specs

## Purpose

This packet translates the refinement pass into concrete UI building blocks for the LocusQ plugin and the Head-Tracking Companion.

Core rules:

- the viewport stays primary
- each mode gets one dominant question
- requested versus active state must always be visible for spatial trust
- diagnostics are available, but they are not the default reading path
- the companion defaults to `Focus`; `Lab` is opt-in

## System-Level Components

| Component | Surface | Default State | Primary Job | Lab/Advanced Behavior |
|---|---|---|---|---|
| Shell header | Plugin | always visible | show mode, session truth, and one structural trust badge | advanced state chips stay out of header |
| Authority card | Plugin `RENDERER` | always open | show requested path, active path, fallback reason, control owner | expands to show codec/parity diagnostics |
| Viewport pane | Plugin | always visible | maintain spatial scene truth and selection context | optional overlays for trails, vectors, packet age |
| Mode rail | Plugin | mode-dependent | expose the minimum actions needed for the current task | diagnostics collapse into drawers |
| Timeline strip | Plugin `EMITTER` | visible in emitter mode only | author motion and temporal behavior | advanced timing stats stay behind info affordances |
| Focus console | Companion | default tab | readiness, sync, center, active profile, capture readiness | no dense telemetry here |
| Lab console | Companion | closed/secondary | axis sweeps, packet age, matcher scores, debug views | full diagnostics and evidence view |
| Capture tray | Companion | guided stepper | acquire left/right/front views and apply result safely | developer details live in expandable rows |

## Plugin Components

### Shell Header

Content order:

1. `LocusQ` brand
2. mode tabs
3. session state pill
4. trust or live-device badge
5. utility affordances

Behavior:

- maximum of two persistent pills
- if more than two states are active, the least critical ones roll into the rail summary row
- pills never duplicate rail-card titles

States:

| State | Color | Meaning | Copy Pattern |
|---|---|---|---|
| neutral | dim | idle / no claim | `Session Idle` |
| ready | ready | all requested services available | `Ready` |
| live | live | realtime tracking active | `Tracking Live` |
| warn | warn | degraded but usable | `Fallback Stereo` |
| error | error | blocked or failed | `Companion Unavailable` |

### Authority Card

Purpose:

- make spatial trust legible without opening diagnostics

Required fields:

- `Requested`
- `Active`
- `Why this changed`
- `Control owner`

Interaction:

- card is always visible in `RENDERER`
- in other modes it may appear as a compact summary row
- the explanation field is plain-language, never raw internal identifiers

Accessibility:

- announce changes in a polite live region
- ensure every state pair is readable without color

### Mode Rails

`CALIBRATE`

- top card: measurement readiness
- second card: target/config selection
- lower cards: routing, measurement, and profile actions
- diagnostics drawer: speaker histories, raw status details

`EMITTER`

- top card: selected emitter identity + audible state
- second card: motion model
- third card: timeline/temporal controls
- optional lab drawer: simulation parameters, debug values, reactive overlays

`RENDERER`

- top card: authority card
- second card: spatialization / output format
- third card: room / post stage
- diagnostics drawer: parity, profile internals, packet stats, Steam/HRTF evidence

### Viewport Pane

Minimum always-on encodings:

- emitter position
- listener orientation
- selection state
- layout anchors
- active or degraded render mode badge

Optional overlays:

- motion trail
- velocity vectors
- force field or density layers
- packet age ribbon
- temporal history ribbon

Rules:

- overlays do not compete with object selection contrast
- diagnostic overlays default off in shipping operator mode
- any reactive layer needs smoothing, deadband, and a visible label

## Companion Components

### Focus Console

Top-to-bottom order:

1. readiness ladder
2. live device status
3. `Center / Sync`
4. active profile summary
5. capture tray
6. apply/send confirmation

The first screen should answer:

- is my device supported?
- is the stream safe to send?
- do I need to center?
- what profile is active?

### Lab Console

Contains:

- packet age and effective rate
- yaw/pitch/roll breakdown
- axis sanity views
- matcher scores and fallback reason
- profile artifacts and debug details

Rules:

- lab opens from a secondary segmented control or drawer
- no lab-only metric may block the focus flow
- stale-pose behavior must be visible here

### Capture Tray

Slots:

- left ear
- right ear
- front / fit reference

Required guidance:

- local-only processing statement
- capture-quality hint
- explicit fallback outcome when confidence is insufficient

Buttons:

- capture / retake
- clear
- apply profile

## Cross-Format And Runtime Notes

AUv3:

- avoid relying on app-only services inside the plugin
- show a limited-capability message instead of hiding unavailable functions

CLAP:

- keep the same information architecture as AU/VST3
- do not introduce format-only tabs or copy changes

WKWebView and WebView2:

- preserve browser-preview fallback
- keep boot and error surfaces explicit
- avoid visual polish that depends on one backend to remain legible

## Accessibility And Input

- minimum target size: `32px` desktop, `28px` dense utility chips only when non-critical
- keyboard order follows task order, not visual novelty
- tab focus always includes mode tabs, authority card actions, and companion sync/apply actions
- tabular numerals for metering, rates, and packet age
- icons never carry state alone

## Parameter And Telemetry Mapping Notes

| Domain | Must Be Visible | May Be Hidden In Lab |
|---|---|---|
| output trust | requested vs active, fallback reason | parity counters, mode internals |
| head tracking | readiness, sync, center, stale/live state | packet history, axis sweep diagnostics |
| reactive visuals | current overlay mode label | raw source features and smoothing coefficients |
| temporal behavior | timeline state and selection | loop safety and drift diagnostics |
