# LocusQ - UI Specification v2

**Window:** 1200 x 800px (fixed)
**Framework:** WebView + Three.js (WebGL)
**Palette:** Dark + White/Gold (Studio Instrument) — desaturated emitter palette
**Changes from v1:** 5 targeted refinements (see changelog at bottom)

---

## Layout Architecture

Unchanged from v1. Persistent viewport + adaptive right rail + collapsible timeline.

```
┌──────────────────────────────────────────────────────────────┐
│  MODE HEADER BAR (40px)                                      │
│  [LOCUSQ] [Cal|Emit|Rend] [SceneStatus] [RoomProfile] [D|F] │
├──────────────────────────────────────────┬───────────────────┤
│                                          │                   │
│           3D VIEWPORT                    │   CONTROL RAIL    │
│           + Speaker energy meters        │   (280px)         │
│           + Calibration ring overlays    │                   │
│           + Active param 3D highlights   │                   │
│                                          │                   │
├──────────────────────────────────────────┤                   │
│  KEYFRAME TIMELINE (120px, Emitter only) │                   │
└──────────────────────────────────────────┴───────────────────┘
```

---

## Change 1: Scene Status Indicator (Header)

Added between mode tabs and room profile, right-aligned.

```
[LOCUSQ] [Calibrate | Emitter | Renderer]  ··· [Scene Status] ··· [Room Profile] [DRAFT]
```

### Status States
| State | Display | Color |
|-------|---------|-------|
| Scene Stable | `STABLE` | #AAAAAA (dim, steady) |
| Physics Running | `PHYSICS` | #D4A847 (gold, subtle pulse) |
| Calibrating | `MEASURING` | #D4A847 (gold, pulse) |
| Render Ready | `READY` | #44AA66 (green) |
| No Room Profile | `NO PROFILE` | #AA4444 (warning) |

- Font: 9px, 600 weight, 1.5px tracking, uppercase
- Background: pill shape, 1px border matching text color at 30% opacity
- Only one status shown at a time (priority: Measuring > Physics > No Profile > Ready > Stable)

---

## Change 2: Physics Preset-First UX (Emitter Rail)

### Before (v1)
Flat parameter list: Mass, Drag, Elasticity, Gravity — always visible.

### After (v2)
Two-tier system: Preset selector + collapsible Advanced panel.

```
┌─────────────────────────┐
│  PHYSICS       [Off ▼]  │
│                         │
│  When mode != Off:      │
│  ┌─────────────────────┐│
│  │ Intensity: ◎ 0.5    ││
│  │ (maps to internal   ││
│  │  physics params)     ││
│  └─────────────────────┘│
│                         │
│  [THROW]  [RESET]       │
│                         │
│  ▶ Advanced             │
│  (collapsed by default) │
│                         │
│  When Advanced expanded: │
│  Mass:      ◎ 1.0 kg    │
│  Drag:      ◎ 0.5       │
│  Elasticity:◎ 0.7       │
│  Gravity:   ◎ 0.0 m/s²  │
│  Friction:  ◎ 0.3       │
│  Grav Dir:  [Down ▼]    │
└─────────────────────────┘
```

### Physics Presets
| Preset | Intensity Maps To | Character |
|--------|-------------------|-----------|
| Off | — | No physics |
| Bounce | elasticity=0.7, gravity=9.8, drag=0.3 | Ball in room |
| Float | gravity=0, drag=0.1, elasticity=0.5 | Zero-G drift |
| Orbit | centripetal force + drag=0.05 | Circular motion |
| Custom | user-defined | Full manual control |

- **Intensity knob** (0.0–1.0): Scales the preset's characteristic parameter. For Bounce: scales gravity+elasticity. For Float: scales drag (inverse). For Orbit: scales angular velocity.
- Selecting a preset auto-populates Advanced values
- Editing Advanced values auto-switches mode to Custom
- Advanced panel: collapsed by default, `▶ Advanced` disclosure triangle

---

## Change 3: Per-Speaker Energy Meters in Viewport

### Visualization
- Small vertical bar (4px wide, max 20px tall) rendered in 3D space directly adjacent to each speaker octahedron
- Position: offset slightly upward from speaker mesh
- Color: white (#E0E0E0) at low levels → gold (#D4A847) at peak
- Opacity: 40% at idle → 100% at signal
- Updates at render frame rate (30-60fps)

### Implementation (Three.js)
- 4 `THREE.Mesh` planes (thin box geometry) anchored to speaker positions
- Scale Y dynamically based on speaker output level (0.0–1.0 normalized)
- Color lerp: `white → gold` based on level

### Behavior per Mode
| Mode | Energy Meters |
|------|---------------|
| Calibrate | Show test signal output level per speaker |
| Emitter | Show this emitter's contribution to each speaker |
| Renderer | Show total mix output per speaker (most useful) |

---

## Change 4: Timeline ↔ 3D Highlight Link

### When user selects a timeline lane:
| Selected Lane | 3D Highlight |
|---------------|-------------|
| Azimuth | Thin horizontal arc at object height showing azimuth sweep range. Gold, 40% opacity. |
| Elevation | Thin vertical arc from object showing elevation sweep. Gold, 40% opacity. |
| Distance | Concentric ring on floor plane at object's distance. Gold, 20% opacity. |
| Size | Object wireframe pulses slightly (opacity 0.6–1.0 at 0.5Hz). |

### Visual Treatment
- Highlight geometry: `THREE.Line` arcs or `THREE.RingGeometry`
- Color: gold (#D4A847), transparent (20-40% opacity)
- Appears on lane click/focus, disappears on lane blur
- Duration: 100ms fade in, 200ms fade out

### No lane selected
No additional geometry. Clean viewport.

---

## Change 5: Elevated Calibration Visual Feedback

### During Measurement (per speaker)
**Expanding ring pulse:**
- A ring emanates from active speaker outward
- Geometry: `THREE.RingGeometry`, expanding radius over 2 seconds
- Color: gold (#D4A847), starts at 60% opacity, fades to 0%
- One ring per sweep cycle
- Ring expands to approximate room size, then disappears
- Speaker octahedron glows gold with 1Hz pulse

**Waveform capture hint (in rail):**
- Below the status dots, when actively measuring:
- Small waveform display (40px tall, full rail width)
- Shows real-time mic input level as a simple amplitude bar
- Color: gold
- Label: "Capturing..." in dim text

### Analysis Step
- All 4 speakers show captured checkmarks (green dots)
- Rail shows: "Analyzing..." with a subtle horizontal progress bar (gold fill on dark track)

### Room Solved
**Viewport overlay animation:**
- Room wireframe edges briefly flash gold (300ms)
- Opacity: 0% → 60% → 0% (triangle envelope)
- One-shot animation, not looping
- Simultaneous: all 4 speaker dots turn green in rail

**Rail update:**
- Status changes to "Profile Complete"
- Save button appears: `SAVE PROFILE` (gold primary button)
- Results summary: delay values, level trims, room dimensions

---

## Emitter Rail (v2 — updated physics section only)

```
┌─────────────────────────┐
│  << EMITTER             │
│─────────────────────────│
│  Label: [My Synth    ]  │
│  Color: ● (click=picker)│
│─────────────────────────│
│  POSITION               │
│  Mode: [Spherical ▼]    │
│  Azimuth:  ◎ -45.0°     │
│  Elevation:◎ 0.0°       │
│  Distance: ◎ 2.5m       │
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
│  PHYSICS       [Off ▼]  │
│                         │  ← When not Off:
│  Intensity: ◎ 0.5       │
│  [THROW]  [RESET]       │
│                         │
│  ▶ Advanced             │  ← Collapsed by default
│─────────────────────────│
│  ANIMATION     [Off ▼]  │
│  Source: [DAW ▼]        │
│  Loop: [○] Off          │
│  Speed: ◎ 1.0x          │
└─────────────────────────┘
```

---

## All Other Sections

Unchanged from v1-ui-spec.md. Refer to that document for:
- Mode Header Bar (plus new Scene Status indicator)
- 3D Viewport base scene
- Renderer Mode Rail
- Keyframe Timeline base layout
- Interaction Model
- Keyboard Shortcuts
- AI/ML Architecture Hooks

---

## v1 → v2 Changelog

| # | Area | Change | Rationale |
|---|------|--------|-----------|
| 1 | Header | Added Scene Status indicator pill | System awareness without leaving viewport |
| 2 | Emitter Rail | Physics → preset-first (Off/Bounce/Float/Orbit/Custom) + collapsed Advanced | Musical over mechanical. "Float gently" > "drag=0.1, mass=1.0" |
| 3 | Viewport | Per-speaker energy meters (3D bars near speakers) | Visual link between sound output and speaker geometry |
| 4 | Timeline+Viewport | Selected lane highlights corresponding 3D geometry | Timeline and object feel physically linked |
| 5 | Calibrate Mode | Expanding rings, waveform hint, "Room Solved" gold flash | Calibration as deliberate ritual, not wizard checkbox |
| — | Style Guide | Emitter palette desaturated 10-15% | "Confident color" not "colorful". Studio, not dashboard. |
