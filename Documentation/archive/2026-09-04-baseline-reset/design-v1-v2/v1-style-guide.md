Title: LocusQ Style Guide v1
Document Type: Style Guide
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-18

# LocusQ - Style Guide v1

**Theme:** Studio Instrument / Premium / Clean
**Principle:** Clarity over flash. Instrument-like. The UI disappears into the work.

---

## Color Palette

### Core Colors
| Role | Hex | Usage |
|------|-----|-------|
| Background | `#0A0A0A` | Main background, viewport bg |
| Surface | `#141414` | Control rail bg, panels |
| Surface Elevated | `#1E1E1E` | Dropdowns, tooltips, modal overlays |
| Border | `#2A2A2A` | Panel dividers, section separators |

### Wireframe & Structure
| Role | Hex | Opacity | Usage |
|------|-----|---------|-------|
| Wireframe Primary | `#E0E0E0` | 100% | Room edges, speaker shapes |
| Wireframe Subtle | `#E0E0E0` | 30% | Room during Calibrate mode |
| Grid | `#1A1A1A` | 100% | Floor grid lines |
| Grid Major | `#2A2A2A` | 100% | Every 5th grid line |

### Accent & Interaction
| Role | Hex | Usage |
|------|-----|-------|
| Gold Primary | `#D4A847` | Selected state, active mode indicator, accent |
| Gold Bright | `#E8C464` | Hover states, energy meters at peak |
| Gold Dim | `#8B7234` | Inactive gold elements, subtle highlights |

### Text
| Role | Hex | Size | Usage |
|------|-----|------|-------|
| Text Primary | `#E0E0E0` | 12px | Labels, values, active text |
| Text Secondary | `#AAAAAA` | 11px | Descriptions, inactive labels |
| Text Dim | `#666666` | 10px | Hints, placeholders, disabled |
| Text Warning | `#AA4444` | 12px | Missing room profile, errors |
| Text Success | `#44AA66` | 12px | Calibration complete, connected |

### Emitter Object Palette (16 colors)
Objects are assigned from this palette sequentially. High saturation, good contrast against dark background.

| Index | Hex | Name |
|-------|-----|------|
| 0 | `#FF6B6B` | Coral |
| 1 | `#4ECDC4` | Teal |
| 2 | `#45B7D1` | Sky |
| 3 | `#96CEB4` | Sage |
| 4 | `#FFEAA7` | Butter |
| 5 | `#DDA0DD` | Plum |
| 6 | `#98D8C8` | Mint |
| 7 | `#F7DC6F` | Maize |
| 8 | `#BB8FCE` | Lavender |
| 9 | `#85C1E9` | Powder |
| 10 | `#F0B27A` | Peach |
| 11 | `#82E0AA` | Spring |
| 12 | `#F1948A` | Rose |
| 13 | `#AED6F1` | Ice |
| 14 | `#D2B4DE` | Lilac |
| 15 | `#A3E4D7` | Seafoam |

### State Colors
| State | Treatment |
|-------|-----------|
| Default | Object color at 100% |
| Selected | Object color + gold ring + 120% brightness |
| Dimmed (non-selected in Emitter mode) | Object color at 30% opacity |
| Disabled/Muted | Object color at 15% opacity, strikethrough label |
| Physics Active | Subtle pulse animation (opacity 80-100% at 1Hz) |

---

## Typography

### Font Stack
```css
font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
```

If Inter is unavailable, system sans-serif fallback. No web font loading — embedded or system only.

### Scale
| Element | Size | Weight | Tracking |
|---------|------|--------|----------|
| Mode Header Tabs | 11px | 600 (semibold) | 1.5px uppercase |
| Section Headers | 10px | 600 | 1.2px uppercase |
| Control Labels | 11px | 400 | 0.3px |
| Control Values | 12px | 500 (medium) | 0 (tabular nums) |
| Emitter Labels (3D) | 10px | 400 | 0.5px |
| Viewport Overlay Text | 13px | 300 (light) | 0.5px |
| Tooltip | 11px | 400 | 0 |

### Numeric Display
- Always use tabular (monospace) figures for values
- Right-align numeric values in controls
- Format: `0.0°`, `2.0m`, `-12.0 dB`, `1.0x`
- Units in dim text (#666) after value

---

## Spacing & Layout

### Grid System
- Base unit: **4px**
- Control rail internal padding: **12px** (3 units)
- Section gap: **16px** (4 units)
- Control vertical spacing: **8px** (2 units)
- Header bar height: **40px** (10 units)

### Control Rail
- Width: **280px**
- Background: `#141414`
- Left border: `1px solid #2A2A2A`
- Sections divided by: `1px solid #2A2A2A` + 16px gap
- Section header: uppercase, 10px, #666, 1.2px tracking
- Scrollable if content exceeds height

### Knob Controls
- Size: **32px** diameter
- Track: 270 degree arc, `#2A2A2A`
- Value arc: 270 degree arc, `#E0E0E0`
- Pointer: small dot at end of value arc
- Active/dragging: value arc turns gold (#D4A847)
- Layout: `[Label] [Knob] [Value+Unit]` in a row, or `[Knob above] [Label below]` for compact

### Toggle Switches
- Size: **28px x 14px**
- Off: `#2A2A2A` track, `#666666` thumb
- On: `#D4A847` track, `#E0E0E0` thumb
- Transition: 150ms ease

### Buttons
- **Primary** (e.g., START MEASURE): Gold bg (#D4A847), black text, 32px height
- **Secondary** (e.g., THROW, RESET): Border (#2A2A2A), white text, 28px height
- **Ghost** (e.g., rail toggle): No border, icon only, hover: #1E1E1E bg
- Border radius: **4px** for all buttons
- Hover: brighten 10%
- Active: darken 10%

### Dropdowns
- Background: `#1E1E1E`
- Border: `1px solid #2A2A2A`
- Selected item: gold text
- Hover: `#2A2A2A` bg
- Height: 28px
- Arrow: small chevron, #666

---

## 3D Viewport Styling

### Materials (Three.js)
```javascript
// Room wireframe
const roomMaterial = new THREE.LineBasicMaterial({
    color: 0xE0E0E0,
    opacity: 0.3,
    transparent: true
});

// Floor grid
const gridColor = 0x1A1A1A;
const gridCenterColor = 0x2A2A2A;

// Speaker (default)
const speakerMaterial = new THREE.MeshBasicMaterial({
    color: 0xE0E0E0,
    wireframe: true
});

// Speaker (active/measuring)
const speakerActiveMaterial = new THREE.MeshBasicMaterial({
    color: 0xD4A847,
    wireframe: true
});

// Emitter object
const emitterMaterial = new THREE.MeshBasicMaterial({
    color: emitterPalette[colorIndex],
    wireframe: true
});

// Selection ring
const selectionMaterial = new THREE.LineBasicMaterial({
    color: 0xD4A847,
    linewidth: 2
});

// Motion trail
const trailMaterial = new THREE.LineBasicMaterial({
    color: emitterPalette[colorIndex],
    opacity: 0.6,
    transparent: true
});

// Velocity vector (arrow)
const velocityMaterial = new THREE.LineBasicMaterial({
    color: 0xD4A847
});
```

### Lighting
- No scene lighting (wireframe materials are unlit)
- Background: `#0A0A0A` (matches page background, seamless)

### Camera Defaults
- FOV: 60
- Near: 0.1, Far: 1000
- Default position: `(5, 4, 5)` looking at `(0, 0, 0)`
- OrbitControls: damping enabled (0.1), smooth rotation

---

## Animation & Transitions

| Element | Duration | Easing | Trigger |
|---------|----------|--------|---------|
| Mode tab underline slide | 200ms | ease-out | Mode switch |
| Rail content crossfade | 150ms | ease-in-out | Mode switch |
| Viewport overlay fade | 200ms | ease-out | Mode switch |
| Rail collapse/expand | 200ms | ease-out | Tab key / toggle |
| Keyframe panel collapse | 200ms | ease-out | Toggle |
| Knob value change | 50ms | linear | Parameter change |
| Toggle switch | 150ms | ease | Click |
| Speaker glow pulse | 1000ms | sine | During measurement |
| Emitter physics pulse | 1000ms | sine | Physics active |
| Selection ring appear | 100ms | ease-out | Object selected |

### Prohibited Animations
- Camera auto-movement on mode switch
- Page transitions / full-screen fades
- Bouncy/spring animations (not instrument-like)
- Loading spinners in viewport (use overlay text instead)

---

## Responsive Behavior

### Rail Collapsed State
- Viewport fills full width (1200px)
- Small floating mode indicator in top-right corner of viewport
- Re-expand button: `<<` floating at right edge

### Keyframe Panel Collapsed (Emitter mode)
- Viewport gains 120px additional height
- Small `▲ Timeline` tab at bottom edge

### Future Considerations
- If window becomes resizable: viewport scales, rail stays 280px, minimum window 1000x700
- Touch support: larger hit targets for knobs (40px), swipe gestures for rail
