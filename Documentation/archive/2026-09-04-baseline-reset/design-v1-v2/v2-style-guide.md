Title: LocusQ Style Guide v2
Document Type: Style Guide
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-18

# LocusQ - Style Guide v2

**Theme:** Studio Instrument / Premium / Clean
**Principle:** Clarity over flash. Instrument-like. The UI disappears into the work.
**Change from v1:** Desaturated emitter palette, added scene status styling, calibration overlays.

---

## Color Palette

### Core Colors
Unchanged from v1.

| Role | Hex | Usage |
|------|-----|-------|
| Background | `#0A0A0A` | Main background, viewport bg |
| Surface | `#141414` | Control rail bg, panels |
| Surface Elevated | `#1E1E1E` | Dropdowns, tooltips, modal overlays |
| Border | `#2A2A2A` | Panel dividers, section separators |

### Wireframe & Structure
Unchanged from v1.

### Accent & Interaction
Unchanged from v1.

### Text
Unchanged from v1.

---

## Emitter Object Palette v2 (Desaturated)

**Change:** Reduced saturation ~12%, increased luminance consistency. "Confident color" — readable, professional, not cheerful.

| Index | v1 Hex | v2 Hex | Name |
|-------|--------|--------|------|
| 0 | `#FF6B6B` | `#D4736F` | Rosewood |
| 1 | `#4ECDC4` | `#5BBAB3` | Verdigris |
| 2 | `#45B7D1` | `#5AADC0` | Slate Blue |
| 3 | `#96CEB4` | `#8DBEA7` | Sage |
| 4 | `#FFEAA7` | `#D8CFA0` | Sandstone |
| 5 | `#DDA0DD` | `#BF9ABD` | Mauve |
| 6 | `#98D8C8` | `#8CC5B7` | Patina |
| 7 | `#F7DC6F` | `#CCBA6E` | Brass |
| 8 | `#BB8FCE` | `#A487B5` | Wisteria |
| 9 | `#85C1E9` | `#7AAFC9` | Steel Blue |
| 10 | `#F0B27A` | `#C9A07A` | Copper |
| 11 | `#82E0AA` | `#7DC49A` | Jade |
| 12 | `#F1948A` | `#C98A84` | Terra |
| 13 | `#AED6F1` | `#96BAD0` | Chambray |
| 14 | `#D2B4DE` | `#B3A0BF` | Heather |
| 15 | `#A3E4D7` | `#8EC8BD` | Celadon |

### Design Intent
- Each color is distinguishable against #0A0A0A at arm's length
- No two adjacent colors are confusable
- Saturation is uniform (~40-50% HSL) — no hot spots
- Names reference materials/minerals, not candy — reinforces studio tool identity

### State Colors (updated for v2 palette)
| State | Treatment |
|-------|-----------|
| Default | Object color at 100% |
| Selected | Object color at 120% brightness + gold selection ring |
| Dimmed | Object color at 30% opacity |
| Disabled/Muted | Object color at 15% opacity |
| Physics Active | Subtle opacity pulse (85-100% at 1Hz) |

---

## Scene Status Indicator (New in v2)

### Styling
```css
.scene-status {
    font-size: 9px;
    font-weight: 600;
    letter-spacing: 1.5px;
    text-transform: uppercase;
    padding: 3px 8px;
    border-radius: 9px;        /* pill shape */
    border: 1px solid;         /* color varies by state */
}
```

| State | Text Color | Border | Background |
|-------|-----------|--------|------------|
| STABLE | #AAAAAA | rgba(170,170,170,0.2) | transparent |
| PHYSICS | #D4A847 | rgba(212,168,71,0.3) | transparent |
| MEASURING | #D4A847 | rgba(212,168,71,0.3) | transparent |
| READY | #44AA66 | rgba(68,170,102,0.3) | transparent |
| NO PROFILE | #AA4444 | rgba(170,68,68,0.3) | transparent |

### Animation
- PHYSICS and MEASURING: border opacity pulses 0.15–0.3 at 1Hz
- Transition between states: 200ms crossfade

---

## Physics Preset UI Styling (New in v2)

### Preset Dropdown
Same styling as other dropdowns, but wider (120px) to accommodate preset names.

### Intensity Knob
- Same as standard knob-mini (32px)
- Arc color: matches current physics preset theme
  - Bounce: white
  - Float: white
  - Orbit: white
  - Custom: gold (indicates manual territory)

### Advanced Disclosure
```css
.disclosure {
    font-size: 10px;
    color: var(--text-dim);
    cursor: pointer;
    padding: 8px 0;
    display: flex;
    align-items: center;
    gap: 6px;
}

.disclosure:hover {
    color: var(--text-secondary);
}

.disclosure-arrow {
    font-size: 8px;
    transition: transform 150ms;
}

.disclosure-arrow.open {
    transform: rotate(90deg);
}
```

---

## Speaker Energy Meter Styling (New in v2)

### In Viewport (Three.js)
```javascript
// Energy meter bar per speaker
const meterGeo = new THREE.BoxGeometry(0.04, 0.01, 0.04); // thin, will scale Y
const meterMat = new THREE.MeshBasicMaterial({
    color: 0xE0E0E0,
    transparent: true,
    opacity: 0.4
});

// Position: offset 0.25m above speaker
// Scale Y: 0.0 to 0.4 (maps to 0% to 100% output)

// Color interpolation:
// level 0.0-0.7: white (#E0E0E0)
// level 0.7-1.0: lerp white → gold (#D4A847)
```

### Behavior
- Smoothing: 50ms attack, 200ms release (follows audio, not twitchy)
- Minimum visible height: 2px equivalent (don't disappear completely when signal present)
- At zero signal: 40% opacity, minimum height
- At peak: 100% opacity, full height, gold color

---

## Calibration Overlay Styling (New in v2)

### Expanding Ring
```javascript
// Ring per measurement sweep
const ringGeo = new THREE.RingGeometry(0.2, 0.22, 48);
const ringMat = new THREE.MeshBasicMaterial({
    color: 0xD4A847,
    side: THREE.DoubleSide,
    transparent: true,
    opacity: 0.6
});

// Animation: scale from 1.0 to ~room radius over 2s
// Opacity: 0.6 → 0.0 over 2s (linear)
// Orientation: horizontal (rotation.x = -PI/2), at speaker Y height
// Origin: active speaker position
```

### Waveform Capture (Rail)
```css
.capture-meter {
    height: 40px;
    background: var(--surface-elevated);
    border-radius: 4px;
    margin: 8px 0;
    position: relative;
    overflow: hidden;
}

.capture-bar {
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    background: var(--gold);
    opacity: 0.6;
    transition: height 50ms;
    /* Height driven by mic input level */
}

.capture-label {
    position: absolute;
    top: 4px;
    left: 8px;
    font-size: 9px;
    color: var(--text-dim);
    letter-spacing: 0.5px;
}
```

### Room Solved Flash
```javascript
// One-shot animation on room wireframe
// Duration: 300ms
// Envelope: 0% → 60% → 0% opacity (triangle)
// Color: gold (#D4A847) applied to room edge material
// Trigger: when analysis completes and profile is generated

function flashRoomSolved() {
    const startTime = performance.now();
    const duration = 300;

    function tick(now) {
        const t = (now - startTime) / duration;
        if (t > 1) {
            roomLines.material.color.setHex(0xE0E0E0);
            roomLines.material.opacity = 0.3;
            return;
        }
        roomLines.material.color.setHex(0xD4A847);
        roomLines.material.opacity = t < 0.5 ? t * 1.2 : (1 - t) * 1.2;
        requestAnimationFrame(tick);
    }
    requestAnimationFrame(tick);
}
```

---

## Timeline ↔ 3D Highlight Styling (New in v2)

### Azimuth Arc
```javascript
const arcGeo = createArcGeometry(0, Math.PI * 2, radius=emitter.distance, segments=64);
const arcMat = new THREE.LineBasicMaterial({
    color: 0xD4A847,
    transparent: true,
    opacity: 0.25
});
// Horizontal ring at emitter Y height
```

### Elevation Arc
```javascript
// Vertical arc from -90 to +90 degrees
// At emitter's azimuth angle
// Radius = emitter distance
// Gold, 25% opacity
```

### Distance Ring
```javascript
const ringGeo = new THREE.RingGeometry(
    emitter.distance - 0.02,
    emitter.distance + 0.02,
    64
);
// Horizontal on floor plane (y=0)
// Gold, 15% opacity
```

### Size Pulse
```javascript
// Existing emitter sphere, opacity oscillates
// 0.6 → 1.0 → 0.6 at 0.5Hz (2 second cycle)
// Uses requestAnimationFrame, not CSS animation
```

### Transitions
- Highlight appear: 100ms fade in
- Highlight disappear: 200ms fade out
- Only one highlight active at a time (selecting new lane crossfades)

---

## All Other Styling

Unchanged from v1-style-guide.md:
- Typography
- Spacing & Layout
- Knob Controls
- Toggle Switches
- Buttons
- Dropdowns
- Scrollbar
- 3D Camera Defaults
- Base Animation Timing
