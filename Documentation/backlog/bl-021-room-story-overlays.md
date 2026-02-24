---
Title: BL-021 Room-Story Overlays
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# BL-021 Room-Story Overlays

## 1. Identity

| Field | Value |
|---|---|
| ID | BL-021 |
| Title | Room-Story Overlays |
| Status | Todo |
| Priority | P2 |
| Track | E — R&D Expansion |
| Effort | Med / M |
| Depends On | BL-014 (Done), BL-015 (Done) |
| Blocks | none |
| Annex | Inline — room analysis overlay mapping (see Section 5) |
| ADRs | ADR-0006 (RT invariant), HX-05 (payload budget) |
| Skills | `$skill_plan`, `$reactive-av`, `$threejs` |

---

## 2. Objective

Implement adaptive room-story overlays — visual layers in the Three.js 3D viewport that expose room acoustic characteristics derived from `RoomAnalyzer.h` telemetry. Three overlay types are required:

1. **Early Reflection Paths** — line geometry showing first-order reflection paths from a selected emitter to the listener, derived from early reflection data in the IR capture result.
2. **Reverb Decay Heatmap** — a volumetric or surface-mapped color zone indicating spatial decay distribution (warm colors = dense energy, cool colors = sparse).
3. **Absorption Zone Indicators** — boundary markers or surface tint indicating frequency-dependent absorption zones (low/mid/high frequency bands).

Source data: `RoomAnalyzer.h` already extracts delay, level, frequency, and early-reflection data from `IRCapture` results. This data must be published through the SceneGraph snapshot extension and consumed by the Three.js overlay system.

Room overlay payload additions must remain within HX-05 budget thresholds. Coordinate with HX-05 budget before Slice C.

---

## 3. Entry Criteria

| Gate | Condition |
|---|---|
| BL-014 | Done — SceneGraph snapshot publication path stable |
| BL-015 | Done — RoomAnalyzer.h IR data extraction is functional |
| HX-05 budget | Room overlay payload size estimated against HX-05 thresholds before Slice C |
| RT invariant | processBlock() is clean before work begins |

---

## 4. Slice Plan

### Slice A — Room Analysis Telemetry Mapping

**Scope:** Pipe `RoomAnalyzer.h` output into a `RoomSnapshot` extension on the SceneGraph. The extension carries: early reflection records (delay, level, direction vector), reverb decay envelope (RT60 per frequency band), and absorption coefficients per surface zone.

**Deliverables:**
- `RoomSnapshot` struct definition (new or in `Source/SceneGraph.h`)
- `RoomAnalyzer.h` output mapped into `RoomSnapshot` fields
- SceneGraph snapshot JSON extended with `room` object
- Payload size estimate for HX-05 compliance check

**Entry:** BL-014 done, BL-015 done.

**Exit:** Scene snapshot JSON includes `room` object with early reflection records and decay envelope; payload size measured.

---

### Slice B — Overlay Rendering in Three.js

**Scope:** `Source/ui/public/js/index.js` — three overlay render layers:

1. Early reflection path lines (THREE.Line with BufferGeometry)
2. Reverb decay heatmap (THREE.Mesh plane or volumetric sprite grid)
3. Absorption zone tint (THREE.Mesh overlay on room boundary geometry)

All overlay geometry must be created once and updated in-place.

**Deliverables:**
- Reflection path line overlay: `updateReflectionPaths(roomSnapshot)`
- Reverb decay heatmap: `updateDecayHeatmap(roomSnapshot)`
- Absorption zone tint: `updateAbsorptionZones(roomSnapshot)`
- Master overlay toggle: `setRoomOverlayVisible(enabled)`

**Entry:** Slice A merged — scene snapshot JSON includes `room` object.

**Exit:** All three overlay types render correctly on a test room geometry; no per-frame allocation.

---

### Slice C — Payload Budget Compliance

**Scope:** Measure room overlay payload contribution to the WebView postMessage size. Compare against HX-05 budget. If over budget: identify and implement reduction strategies (field compression, update rate throttling, LOD).

**Deliverables:**
- Payload measurement report (bytes per snapshot, per update rate)
- HX-05 compliance verdict: PASS or FAIL with delta
- If FAIL: mitigation implementation (throttling, field omission at low LOD, delta encoding)
- Updated validation-trend.md entry

**Entry:** Slice B merged.

**Exit:** Room overlay payload confirmed within HX-05 budget at the defined update rate.

---

## 5. Architecture Notes — Room Analysis Overlay Mapping (Annex)

### RoomAnalyzer Data Available (from BL-015)

```
IRCapture result -> RoomAnalyzer extracts:
  - Early reflections: array of { delay_ms, level_db, direction: [az, el] }
  - RT60: per-band (125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz, 8kHz) in seconds
  - Frequency-dependent absorption: per-surface, per-band coefficient [0,1]
```

### RoomSnapshot Struct

```cpp
// Source/SceneGraph.h or Source/RoomSnapshot.h

struct EarlyReflection {
    float delay_ms;
    float level_db;
    float az_deg;   // direction from emitter
    float el_deg;
};

struct RoomDecayBand {
    float freq_hz;
    float rt60_sec;
};

struct AbsorptionZone {
    int   surface_id;
    float coeff_low;    // 125-500Hz avg
    float coeff_mid;    // 500Hz-2kHz avg
    float coeff_high;   // 2kHz-8kHz avg
};

struct RoomSnapshot {
    static constexpr int MAX_REFLECTIONS = 12;
    static constexpr int MAX_BANDS       = 7;
    static constexpr int MAX_SURFACES    = 8;

    EarlyReflection  reflections[MAX_REFLECTIONS];
    int              reflection_count = 0;

    RoomDecayBand    decay_bands[MAX_BANDS];
    int              band_count = 0;

    AbsorptionZone   absorption_zones[MAX_SURFACES];
    int              zone_count = 0;

    uint64_t         capture_timestamp_ms = 0;
};
```

### SceneGraph Snapshot JSON Extension

```json
{
  "room": {
    "capture_ts": 1708732800000,
    "reflections": [
      { "delay_ms": 12.3, "level_db": -18.5, "az": 45.0, "el": 0.0 },
      { "delay_ms": 23.1, "level_db": -24.0, "az": -30.0, "el": 15.0 }
    ],
    "decay_bands": [
      { "freq_hz": 125,  "rt60_sec": 0.42 },
      { "freq_hz": 500,  "rt60_sec": 0.35 },
      { "freq_hz": 1000, "rt60_sec": 0.28 },
      { "freq_hz": 4000, "rt60_sec": 0.20 }
    ],
    "absorption_zones": [
      { "surface_id": 0, "coeff_low": 0.12, "coeff_mid": 0.25, "coeff_high": 0.55 },
      { "surface_id": 1, "coeff_low": 0.08, "coeff_mid": 0.15, "coeff_high": 0.30 }
    ]
  }
}
```

### Data Flow

```
RoomAnalyzer.h (message thread / timer callback)
    -> IRCapture result
    -> extract early reflections, RT60, absorption
    -> populate RoomSnapshot struct

SceneGraph::updateRoom(const RoomSnapshot&)
    -> stored in SceneGraph (lock-free write)

Scene snapshot builder
    -> includes room JSON object in postMessage payload

WebView / Three.js (index.js)
    -> updateReflectionPaths(room.reflections)
    -> updateDecayHeatmap(room.decay_bands)
    -> updateAbsorptionZones(room.absorption_zones)
```

### Three.js Overlay Implementation Hints

**Early Reflection Paths:**
```javascript
// Pre-allocate MAX_REFLECTIONS line segments
const reflectionLines = Array.from({ length: 12 }, () => {
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(new Float32Array(6), 3));
    return new THREE.Line(geo, new THREE.LineBasicMaterial({ color: 0x88aaff }));
});
scene.add(...reflectionLines);

function updateReflectionPaths(reflections) {
    for (let i = 0; i < 12; i++) {
        const line = reflectionLines[i];
        if (i < reflections.length) {
            const r = reflections[i];
            // compute end point from az/el and delay_ms
            const pos = line.geometry.attributes.position;
            // ... set start (emitter pos) and end (reflection endpoint) ...
            pos.needsUpdate = true;
            line.visible = true;
        } else {
            line.visible = false;
        }
    }
}
```

**Reverb Decay Heatmap — color by RT60:**
```javascript
function rt60ToColor(rt60_sec) {
    // warm (long decay) -> cool (short decay)
    const t = Math.min(rt60_sec / 1.0, 1.0);  // normalize to 1.0s
    return new THREE.Color().lerpColors(
        new THREE.Color(0x0044ff),  // cool blue: short decay
        new THREE.Color(0xff4400),  // warm red: long decay
        t
    );
}
```

### Payload Size Estimate

| Component | Fields | Size per snapshot |
|---|---|---|
| 12 early reflections | 4 floats each | 192 bytes (raw) / ~250 bytes JSON |
| 7 decay bands | 2 floats each | 56 bytes (raw) / ~100 bytes JSON |
| 8 absorption zones | 4 fields each | 128 bytes (raw) / ~180 bytes JSON |
| Total room JSON | — | ~530 bytes per snapshot |

HX-05 budget check required in Slice C to confirm this is within threshold.

---

## 6. RT Invariant Checklist

| Check | Rule |
|---|---|
| No RoomAnalyzer execution in processBlock | All IR analysis in message thread or timer |
| No allocation in SceneGraph room update | RoomSnapshot is fixed-size struct, pre-allocated |
| No lock in processBlock | SceneGraph uses existing lock-free read path |
| Three.js geometry reuse | Reflection line BufferGeometry positions updated in-place |

---

## 7. Risks

| Risk | Severity | Mitigation |
|---|---|---|
| Room overlay payload exceeds HX-05 budget | High | Measure in Slice A; throttle update rate (e.g., 1 Hz instead of scene rate) or omit high-LOD fields |
| Visual complexity vs clarity at complex geometries | Med | Default: show only reflection paths; decay/absorption optional toggles |
| Early reflection direction vectors may be in wrong coordinate frame | Med | Validate coordinate frame in Slice A data mapping against existing emitter coordinate conventions |
| RoomAnalyzer update rate too slow for dynamic scenes | Low | Room overlays are designed for static/semi-static acoustics; document this constraint |
| Three.js heatmap performance at high resolution | Low | Use low-resolution grid (8x8 max); pre-allocate sprite array |

---

## 8. Validation Plan

| Step | Method | Pass Criteria |
|---|---|---|
| Slice A: JSON includes room object | Log scene snapshot | Room object present; reflection_count > 0 in test room |
| Slice A: payload size | Measure JSON string byte length | < HX-05 budget threshold (TBD) |
| Slice B: reflection lines render | Manual QA in Three.js viewport with test room | Lines visible from emitter position toward boundary |
| Slice B: heatmap color correct | Set RT60 = 0.1s and 1.0s; observe heatmap | Short decay = blue, long decay = red |
| Slice B: no per-frame allocation | Chrome DevTools memory timeline (30s) | Zero GC pressure from overlay updates |
| Slice C: HX-05 compliance | Payload measurement at default update rate | PASS or documented mitigation applied |

---

## 9. Files Touched

| File | Action |
|---|---|
| `Source/SceneGraph.h` | MODIFY — Slice A: add RoomSnapshot struct and SceneGraph member |
| `Source/RoomAnalyzer.h` | MODIFY — Slice A: populate RoomSnapshot from IR results |
| `Source/PluginProcessor.cpp` | MODIFY — Slice A: wire RoomAnalyzer update trigger |
| `Source/ui/public/js/index.js` | MODIFY — Slice B: three overlay render functions |
| `TestEvidence/validation-trend.md` | UPDATE — Slice C payload measurement and HX-05 result |
| `status.json` | UPDATE per slice completion |

---

## 10. ADR References

| ADR | Relevance |
|---|---|
| ADR-0006 | RT invariant — room analysis must not execute in processBlock() |
| HX-05 | Payload budget — room overlay additions must not exceed threshold |

If HX-05 budget is exceeded and mitigation changes the snapshot schema, record the tradeoff decision in a new ADR or HX-05 amendment before closing Slice C.

---

## 11. Agent Mega-Prompts

### Slice A — Skill-Aware Prompt

```
SKILLS: $skill_plan $reactive-av
BACKLOG: BL-021 Slice A
TASK: Pipe RoomAnalyzer telemetry into SceneGraph snapshot extension

CONTEXT:
- LocusQ is a JUCE VST3/AU/CLAP spatial audio plugin with WebView (Three.js) UI.
- Source/RoomAnalyzer.h extracts from IRCapture results:
    early reflections: array of {delay_ms, level_db, direction (az/el deg)}
    RT60: per frequency band (125Hz-8kHz)
    absorption: per surface, per band coefficient
- BL-014 (SceneGraph) and BL-015 (RoomAnalyzer) are Done.
- RT invariant (ADR-0006): no room analysis in processBlock().
- HX-05 payload budget applies — must measure snapshot size increase.

OBJECTIVE:
1. Define RoomSnapshot struct in Source/SceneGraph.h (or new Source/RoomSnapshot.h):
   - EarlyReflection { delay_ms, level_db, az_deg, el_deg }
   - RoomDecayBand { freq_hz, rt60_sec }
   - AbsorptionZone { surface_id, coeff_low, coeff_mid, coeff_high }
   - RoomSnapshot { EarlyReflection[12], RoomDecayBand[7], AbsorptionZone[8],
                    reflection_count, band_count, zone_count, capture_timestamp_ms }
   - All arrays are fixed-size (no heap allocation).

2. In RoomAnalyzer.h (analysis pass, NOT processBlock), populate RoomSnapshot
   from the IRCapture result. Map delay/level/early-reflection data to EarlyReflection.
   Map RT60 data to RoomDecayBand. Map absorption to AbsorptionZone.

3. Add SceneGraph::updateRoom(const RoomSnapshot&) — lock-free write.

4. Extend scene snapshot JSON builder to include "room" object with reflections,
   decay_bands, and absorption_zones arrays.

5. Measure snapshot JSON byte size increase and compare against HX-05 budget.

CONSTRAINTS:
- All structs fixed-size (no std::vector, no heap).
- Room analysis triggered from message thread or timer callback, never processBlock().
- JSON extension must be backward-compatible (room key absent = old behavior).

DELIVERABLES:
1. RoomSnapshot struct definition (complete)
2. RoomAnalyzer mapping code snippet
3. SceneGraph update method signature
4. JSON snapshot builder diff
5. Payload size table (bytes raw / bytes JSON / HX-05 budget status)
```

---

### Slice A — Standalone Fallback Prompt

```
CONTEXT (no skill files available):
You are extending a spatial audio plugin (LocusQ) to publish room acoustic data
to its Three.js WebView UI. The plugin has:

- Source/RoomAnalyzer.h: already extracts early reflection paths (delay, level,
  direction as azimuth/elevation), RT60 per frequency band, and frequency-dependent
  absorption coefficients per room surface from IRCapture results.
- Source/SceneGraph.h: singleton holding scene state, published as JSON to WebView.
- The audio thread (processBlock()) must NOT perform room analysis — use a timer
  callback or message thread.

WHAT TO IMPLEMENT (Slice A):
1. Define a RoomSnapshot struct with fixed-size arrays (NO std::vector):
   - Up to 12 EarlyReflection records (delay_ms float, level_db float, az_deg float, el_deg float)
   - Up to 7 RoomDecayBand records (freq_hz float, rt60_sec float)
   - Up to 8 AbsorptionZone records (surface_id int, coeff_low/mid/high float)
   Include counts and a capture_timestamp_ms field.

2. Map RoomAnalyzer.h output into RoomSnapshot fields in the analysis pass
   (outside processBlock).

3. Extend SceneGraph with updateRoom(const RoomSnapshot&).

4. Add "room" key to the JSON snapshot output with reflections/decay_bands/
   absorption_zones arrays.

5. Estimate payload size and note whether it is likely within a typical
   100-500 byte budget increment for WebView messages.

OUTPUT:
- Struct definition
- Analysis pass mapping snippet
- JSON snapshot extension
- Payload size estimate
- Confirm: no room analysis in processBlock()
```

---

### Slice B — Skill-Aware Prompt

```
SKILLS: $reactive-av $threejs
BACKLOG: BL-021 Slice B
TASK: Implement room-story overlay rendering in Three.js

CONTEXT:
- Slice A is merged: scene snapshot JSON includes "room" object.
- Source/ui/public/js/index.js handles Three.js rendering.
- Three overlay types required: reflection path lines, reverb decay heatmap,
  absorption zone tint.
- All geometry must be pre-allocated and updated in-place (no per-frame allocation).

OBJECTIVE:
1. updateReflectionPaths(reflections):
   - Pre-allocate 12 THREE.Line objects with BufferGeometry.
   - On update: set start=emitter position, end=computed reflection endpoint
     from az/el + delay_ms (distance = delay_ms * speed_of_sound).
   - Lines colored by level_db (brighter = louder reflection).
   - Hide unused lines (visible = false).

2. updateDecayHeatmap(decay_bands):
   - Render as a color-mapped grid overlay or ambient fog volume.
   - Color: warm (red) = long RT60, cool (blue) = short RT60.
   - Use pre-allocated sprite array or PlaneGeometry (no new allocation on update).

3. updateAbsorptionZones(absorption_zones):
   - Tint room boundary surfaces by absorption coefficient.
   - Low absorption = hard/reflective color, high = soft/absorptive.
   - Per-surface mesh material color update only.

4. setRoomOverlayVisible(type, enabled):
   - type: 'reflections' | 'decay' | 'absorption' | 'all'
   - Toggle visibility of overlay group.

DELIVERABLES:
1. updateReflectionPaths() implementation
2. updateDecayHeatmap() implementation
3. updateAbsorptionZones() implementation
4. setRoomOverlayVisible() implementation
5. Geometry lifecycle notes (create once, update in-place)
6. Confirm no per-frame THREE.Mesh or Material creation
```

---

### Slice C — Skill-Aware Prompt

```
SKILLS: $skill_plan $skill_testing
BACKLOG: BL-021 Slice C
TASK: Measure room overlay payload and verify HX-05 budget compliance

CONTEXT:
- Slices A and B are merged.
- Room overlay adds ~530 bytes to each scene snapshot (estimate from Slice A).
- HX-05 defines a payload budget for WebView postMessage updates.
- Need to verify compliance and implement mitigation if over budget.

OBJECTIVE:
1. Instrument the scene snapshot builder to log JSON byte length per postMessage.
2. Run at default update rate (e.g., 30 Hz scene updates) and record:
   - Mean payload size (bytes)
   - Max payload size (bytes)
   - Room object contribution (bytes)
3. Compare against HX-05 budget threshold.
4. If PASS: document result in TestEvidence/validation-trend.md.
5. If FAIL: implement one of:
   - Throttle room snapshot to 1 Hz (separate from emitter snapshot)
   - Omit high-LOD fields (absorption_zones) at lower LOD setting
   - Delta encode: only send room snapshot when values change by > threshold
6. Re-measure after mitigation and confirm PASS.

DELIVERABLES:
1. Measurement methodology description
2. Payload size table: baseline / with room overlay / delta / HX-05 budget
3. Compliance verdict: PASS or FAIL+mitigation
4. Mitigation implementation (if needed)
5. TestEvidence/validation-trend.md entry format
```

---

## 12. Closeout Checklist

- [ ] Slice A: RoomSnapshot struct defined with fixed-size arrays
- [ ] Slice A: RoomAnalyzer output mapped into RoomSnapshot in analysis pass (not processBlock)
- [ ] Slice A: Scene snapshot JSON includes "room" object
- [ ] Slice A: Payload size measured and HX-05 estimate documented
- [ ] Slice B: Reflection path lines rendered and updated in-place
- [ ] Slice B: Reverb decay heatmap renders with correct warm/cool color mapping
- [ ] Slice B: Absorption zone tint applied to room boundary surfaces
- [ ] Slice B: No per-frame geometry allocation confirmed
- [ ] Slice C: Payload measurement complete
- [ ] Slice C: HX-05 compliance PASS (or mitigation applied and re-measured PASS)
- [ ] Slice C: Results in `TestEvidence/validation-trend.md`
- [ ] RT invariant confirmed: no room analysis in processBlock()
- [ ] `status.json` updated with BL-021 completion
