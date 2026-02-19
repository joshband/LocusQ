Title: LocusQ UI Specification v1
Document Type: UI Specification
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-18

# LocusQ - UI Specification v1

**Window:** 1200 x 800px (fixed)
**Framework:** WebView + Three.js (WebGL)
**Palette:** Dark + White/Gold (Studio Instrument)

---

## Layout Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  MODE HEADER BAR (40px)                                      │
│  [Calibrate | Emitter | Renderer]  RoomProfile  [Draft|Final]│
├──────────────────────────────────────────┬───────────────────┤
│                                          │                   │
│           3D VIEWPORT                    │   CONTROL RAIL    │
│           (fluid, fills space)           │   (280px, right)  │
│                                          │                   │
│   - Room wireframe                       │   Mode-specific   │
│   - Speakers (4)                         │   controls that   │
│   - Listener (center)                    │   morph per mode  │
│   - Emitter objects                      │                   │
│   - Motion trails                        │   Collapsible     │
│   - Velocity vectors                     │   via toggle      │
│   - Grid floor                           │                   │
│   - OrbitControls (mouse)                │                   │
│                                          │                   │
├──────────────────────────────────────────┤                   │
│  KEYFRAME TIMELINE (120px, Emitter only) │                   │
│  Collapsible. Transport controls + lanes │                   │
└──────────────────────────────────────────┴───────────────────┘
```

### Dimensions
| Region | Width | Height | Notes |
|--------|-------|--------|-------|
| Mode Header | 100% | 40px | Fixed, always visible |
| 3D Viewport | 100% - 280px (rail open) / 100% (rail closed) | Remaining | Fluid, responds to rail toggle |
| Control Rail | 280px | 100% - 40px | Right side, collapsible |
| Keyframe Timeline | 100% - 280px | 120px | Bottom, Emitter mode only, collapsible |

### Rail Collapse Behavior
- Toggle button: `>>` / `<<` icon at rail top-left corner
- Animation: 200ms ease-out slide
- Viewport stretches to fill on collapse
- Hotkey: `Tab` toggles rail

---

## Mode Header Bar

### Layout (left to right)
1. **LocusQ Logo** — Text logotype, gold accent, 14px
2. **Mode Selector** — Three tab buttons: `CALIBRATE` | `EMITTER` | `RENDERER`
   - Active mode: white text, gold underline (2px)
   - Inactive: gray text (#666), no underline
   - Transition: underline slides between tabs (200ms)
3. **Room Profile Indicator** — Center-right
   - No profile: `No Room Profile` in dim red (#AA4444)
   - Loaded: Profile name in white, small green dot
   - Click to load/save/manage profiles
4. **Quality Badge** — Far right
   - `DRAFT` (default): white outline badge
   - `FINAL`: gold filled badge
   - Click to toggle

---

## 3D Viewport

### Camera
- **Type:** Perspective, 60 FOV
- **Default position:** Elevated 45 degrees, looking at room center
- **Controls:** OrbitControls (left-drag rotate, right-drag pan, scroll zoom)
- **RULE:** Camera position persists across mode switches. Never auto-animate camera.

### Room Wireframe
- 6 edges of a box (room dimensions from Room Profile, or default 6x4x3m)
- Color: #E0E0E0 at 30% opacity (subtle, not dominant)
- Floor grid: 0.5m spacing, color #1A1A1A

### Speakers (4)
- Shape: Small pyramids (pointing inward toward listener) or octahedrons
- Color: #E0E0E0 (white) default
- Labels: `SPK1`, `SPK2`, `SPK3`, `SPK4` floating above
- **Calibrate mode:** Active speaker glows gold (#D4A847), pulsing during measurement
- **Renderer mode:** Energy meters — color shifts from white → gold based on output level

### Listener
- Shape: Small cross/plus at room center
- Color: #666666 (subtle, not distracting)
- Always visible, never selectable

### Emitter Objects
- Shape: Sphere wireframe (radius proportional to `size_uniform`)
- Color: Per-emitter from palette (see Style Guide for 16-color palette)
- Selected emitter: brighter, thicker lines, gold selection ring
- Directivity: Cone wireframe extending from object in aim direction (when directivity > 0)
- **Labels:** Floating text above object (from `emit_label`)

### Motion Trails
- Type: Line strip, last N seconds of positions
- Color: Same as emitter, fading to transparent over time
- Visible when `rend_viz_trails` is On

### Velocity Vectors
- Type: Arrow from object center in velocity direction
- Length proportional to speed
- Color: gold (#D4A847)
- Visible when `rend_viz_vectors` is On

### View Modes
- Perspective (default) — free orbit
- Top Down — locked overhead, no orbit pitch
- Front — locked front view
- Side — locked side view
- Switch via buttons in viewport corner or keyboard shortcuts (1/2/3/4)

---

## Control Rail — Per-Mode Content

### Calibrate Mode Rail

```
┌─────────────────────────┐
│  << CALIBRATION         │
│─────────────────────────│
│  Step 1 of 4            │
│  ■ ○ ○ ○  progress dots │
│                         │
│  SPEAKER SETUP          │
│  Config: [4xMono ▼]     │
│  SPK1 Out: [1 ▼]        │
│  SPK2 Out: [2 ▼]        │
│  SPK3 Out: [3 ▼]        │
│  SPK4 Out: [4 ▼]        │
│                         │
│  MIC INPUT              │
│  Channel: [1 ▼]         │
│  Level: ████░░ -12dB    │
│                         │
│  TEST SIGNAL            │
│  Type: [Sweep ▼]        │
│  Level: -20 dBFS [knob] │
│                         │
│  ┌───────────────────┐  │
│  │   START MEASURE   │  │
│  │   (gold button)   │  │
│  └───────────────────┘  │
│                         │
│  Status: Idle           │
│  SPK1: ○ Not measured   │
│  SPK2: ○ Not measured   │
│  SPK3: ○ Not measured   │
│  SPK4: ○ Not measured   │
└─────────────────────────┘
```

Wizard steps:
1. Speaker & Mic Setup (channel assignments)
2. Measuring (sequential per-speaker, auto-advance)
3. Analysis (processing captured IRs)
4. Review & Save (show results, save Room Profile)

### Emitter Mode Rail

```
┌─────────────────────────┐
│  << EMITTER             │
│─────────────────────────│
│  Label: [My Synth    ]  │
│  Color: ● (click=picker)│
│─────────────────────────│
│  POSITION               │
│  Mode: [Spherical ▼]    │
│  Azimuth:  ◎ 0.0°       │
│  Elevation:◎ 0.0°       │
│  Distance: ◎ 2.0m       │
│─────────────────────────│
│  SIZE                   │
│  Link: [■] On           │
│  Scale:  ◎ 0.5m         │
│─────────────────────────│
│  AUDIO                  │
│  Gain:   ◎ 0.0 dB       │
│  Spread: ◎ 0.0          │
│  Direct: ◎ 0.5          │
│─────────────────────────│
│  PHYSICS    [Off ▼]     │
│  Mass:      ◎ 1.0 kg    │
│  Drag:      ◎ 0.5       │
│  Elasticity:◎ 0.7       │
│  Gravity:   ◎ 0.0 m/s²  │
│                         │
│  [THROW]  [RESET]       │
│─────────────────────────│
│  ANIMATION  [Off ▼]     │
│  Source: [DAW ▼]        │
│  Loop: [○] Off          │
│  Speed: ◎ 1.0x          │
└─────────────────────────┘
```

Knob controls (◎) are small rotary encoders:
- Drag up/down to adjust
- Double-click to type value
- Right-click for range/reset

### Renderer Mode Rail

```
┌─────────────────────────┐
│  << RENDERER            │
│─────────────────────────│
│  SCENE (3 emitters)     │
│  ┌─────────────────────┐│
│  │● My Synth    S M    ││
│  │● Bass         S M   ││
│  │● Vocals       S M   ││
│  └─────────────────────┘│
│  (S=Solo, M=Mute)       │
│─────────────────────────│
│  MASTER                 │
│  Gain: ◎ 0.0 dB         │
│─────────────────────────│
│  SPEAKERS               │
│  SPK1: ◎ 0.0dB  ◎ 0ms  │
│  SPK2: ◎ 0.0dB  ◎ 0ms  │
│  SPK3: ◎ 0.0dB  ◎ 0ms  │
│  SPK4: ◎ 0.0dB  ◎ 0ms  │
│─────────────────────────│
│  SPATIALIZATION         │
│  Distance: [Inv² ▼]     │
│  Ref Dist: ◎ 1.0m       │
│  Doppler:  [○] Off      │
│  Air Abs:  [■] On       │
│─────────────────────────│
│  ROOM                   │
│  Enable:   [■] On       │
│  Mix:      ◎ 0.3        │
│  Size:     ◎ 1.0x       │
│  Damping:  ◎ 0.5        │
│  ER Only:  [○] Off      │
│─────────────────────────│
│  PHYSICS (Global)       │
│  Rate:   [60Hz ▼]       │
│  Walls:  [■] On         │
│  [PAUSE ALL]            │
└─────────────────────────┘
```

---

## Keyframe Timeline (Emitter Mode Only)

```
┌────────────────────────────────────────────────────────────┐
│  ◀ ■ ▶  00:00.000 / 00:32.000   Loop:[○]  Sync:[■]       │
│────────────────────────────────────────────────────────────│
│  Azimuth  │──●────────●──────────●───────│                 │
│  Elevation│──────────●───────────────────│                 │
│  Distance │──●──────────────●────────────│                 │
│  Size     │──────────────────────────────│                 │
│────────────────────────────────────────────────────────────│
│  ▲ Scrub bar / playhead                                    │
└────────────────────────────────────────────────────────────┘
```

- Each parameter lane shows keyframe dots (●)
- Drag keyframes to reposition
- Click empty space to add keyframe
- Right-click keyframe for curve type (Linear/EaseIn/EaseOut/Step)
- Playhead syncs with DAW transport (when sync enabled)
- Collapsible: `▼` / `▲` toggle at left edge

---

## Mode Transition Behavior

### Core Rule
> The viewport is the world. The rail is the toolset.
> Mode switching changes tools, not the world.
> Camera orientation is ALWAYS persistent.

### Calibrate Mode Overlays
- Room wireframe: increases to 50% opacity, becomes semi-transparent grid
- Speakers: glow gold when actively being measured, pulse animation
- Microphone icon appears at mic position
- Measurement visualization: radial pulse rings emanating from active speaker
- Subtle guidance text overlaid in viewport (e.g., "Measuring Speaker 2...")
- Emitter objects: hidden (not relevant during calibration)

### Emitter Mode Overlays
- Selected emitter: gold selection ring, brighter wireframe
- Size sphere: visible wireframe sphere showing object extent
- Velocity vector: gold arrow (when physics active)
- Motion trail: colored line behind object
- Other emitters: visible but dimmed (30% opacity)
- Directivity cone: wireframe cone from object

### Renderer Mode Overlays
- All emitters: full visibility, own colors
- Speaker energy meters: small bar or glow intensity near each speaker
- Collision boundaries: faint room edge highlights (when walls enabled)
- Reflection hints (Final mode): faint lines showing early reflection paths
- Scene is fully populated, everything visible at once

### Transition Animation
- Rail content: 150ms crossfade between mode panels
- Viewport overlays: 200ms fade in/out
- No camera movement
- Mode header underline: 200ms slide

---

## Interaction Model

### 3D Viewport Interactions
| Action | Behavior |
|--------|----------|
| Left-drag (empty space) | Orbit camera |
| Right-drag | Pan camera |
| Scroll | Zoom |
| Click emitter | Select emitter (Emitter/Renderer mode) |
| Drag emitter | Move in XY plane (Emitter mode) |
| Shift+drag emitter | Move in Z (height) |
| Double-click emitter | Focus camera on object |
| Click speaker | Select speaker (Calibrate mode) |

### Control Rail Interactions
| Control | Behavior |
|---------|----------|
| Rotary knob (◎) | Drag up/down to adjust |
| Rotary knob double-click | Enter precise value |
| Rotary knob right-click | Reset to default |
| Toggle [■]/[○] | Click to toggle |
| Dropdown [▼] | Click to expand options |
| Button [THROW] | Momentary trigger |

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| Tab | Toggle control rail |
| 1/2/3/4 | View modes (Perspective/Top/Front/Side) |
| Space | Play/Pause (keyframe timeline) |
| R | Reset selected emitter physics |
| T | Throw selected emitter |
| G | Toggle grid |
| L | Toggle labels |
| Escape | Deselect all |

---

## AI/ML Architecture Hooks (v2+)

### Hook Points (designed into v1 architecture, UI not exposed)

1. **Auto-Spatialize Button** (future: appears in Emitter rail)
   - Analyzes audio features → suggests position/size/motion
   - Presents suggestion as ghost object user can accept/reject
   - `window.__JUCE__.requestAutoSpatialize(emitterId)`

2. **Motion Language Input** (future: appears in Emitter rail)
   - Text field: "describe motion..."
   - LLM parses → parameterized motion preset
   - Preview as ghost trail before applying
   - `window.__JUCE__.generateMotionFromText(description)`

3. **Neural Room Toggle** (future: appears in Renderer rail)
   - Alternative to FDN reverb: ML-based room model
   - Toggle: `Algorithmic` / `Neural`
   - `window.__JUCE__.setRoomEngine("neural")`

### Design Principle for AI Features
> AI is a brilliant assistant engineer who proposes ideas — never a co-producer who takes over the session.
- All AI suggestions appear as previews (ghost objects, ghost trails)
- User explicitly accepts or rejects
- Scene graph remains deterministic
- AI writes structured state changes, never raw audio manipulation
