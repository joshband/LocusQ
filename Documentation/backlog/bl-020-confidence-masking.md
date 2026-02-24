---
Title: BL-020 Confidence Masking Overlay
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# BL-020 Confidence Masking Overlay

## 1. Identity

| Field | Value |
|---|---|
| ID | BL-020 |
| Title | Confidence Masking Overlay |
| Status | Todo |
| Priority | P2 |
| Track | E — R&D Expansion |
| Effort | Med / M |
| Depends On | BL-014 (Done), BL-019 (Done) |
| Blocks | none |
| Annex | Inline — perception overlay mapping (see Section 5) |
| ADRs | ADR-0006 (RT invariant) |
| Skills | `$skill_plan`, `$reactive-av`, `$threejs` |

---

## 2. Objective

Implement a confidence/masking overlay mapping for the LocusQ 3D viewport. Each active emitter slot in the SceneGraph exposes three per-emitter spatial rendering quality indicators:

- `distance_confidence` [0.0–1.0] — reliability of distance-based gain at current emitter distance
- `occlusion_probability` [0.0–1.0] — estimated probability that the emitter is occluded
- `hrtf_match_quality` [0.0–1.0] — quality of the HRTF dataset match for the emitter's current azimuth/elevation
- `combined_confidence` [0.0–1.0] — weighted aggregate of the above three fields

These values are visualized in the Three.js viewport as color-coded overlay masks rendered on or around each emitter's 3D glyph. Color gradient: green (confidence >= 0.8) -> yellow (0.4–0.8) -> red (< 0.4).

Confidence computation must live in the renderer analysis path or a dedicated post-process pass — never in `processBlock()` directly.

---

## 3. Entry Criteria

| Gate | Condition |
|---|---|
| BL-014 | Done — SceneGraph emitter slot structure is stable |
| BL-019 | Done — emitter data publication path to WebView is established |
| RT invariant | processBlock() is free of alloc/lock before this work begins |
| Schema review | Confidence field names and ranges agreed before Slice A begins |

---

## 4. Slice Plan

### Slice A — Confidence Data Schema Extension

**Scope:** Extend the SceneGraph emitter slot to carry confidence fields. Define the canonical schema. Wire placeholder computation (initially constant 1.0) into the renderer analysis path.

**Deliverables:**
- Extended emitter slot struct in `Source/SceneGraph.h` with four new float fields
- Placeholder confidence computation in `Source/SpatialRenderer.h` or a new `Source/ConfidenceAnalyzer.h`
- JSON snapshot extension: confidence fields included in scene snapshot published to WebView

**Entry:** BL-014 done, BL-019 done.

**Exit:** Scene snapshot JSON includes confidence fields for each emitter; values round-trip to WebView.

---

### Slice B — Three.js Overlay Rendering

**Scope:** `Source/ui/public/js/index.js` — render a color-coded overlay on each emitter glyph using the confidence fields received in the scene snapshot.

**Deliverables:**
- Confidence overlay render pass in Three.js (ring, halo, or glyph color modulation)
- Color mapping function: `confidenceToColor(value)` returning THREE.Color
- Toggle: overlay visible only when confidence mode is active (UI toggle or always-on initially)
- Performance guard: overlay geometry reuse (no per-frame allocation of new meshes)

**Entry:** Slice A merged — scene snapshot JSON includes confidence fields.

**Exit:** Each emitter glyph shows correct color gradient in the 3D viewport when confidence data is non-trivial.

---

### Slice C — Validation Matrix

**Scope:** Define and execute validation scenarios confirming that confidence values update correctly under spatial test cases.

**Deliverables:**
- Validation matrix document (or inline in TestEvidence) with >= 5 test scenarios
- Manual QA checklist: emitter at close/far distance, occluded/unoccluded, on-axis/off-axis HRTF position
- Automated smoke: scene snapshot JSON confidence values non-null and in [0,1] range

**Entry:** Slice B merged.

**Exit:** All validation matrix checks pass; results logged in `TestEvidence/validation-trend.md`.

---

## 5. Architecture Notes — Perception Overlay Mapping (Annex)

### Confidence Field Definitions

| Field | Range | Computation hint |
|---|---|---|
| `distance_confidence` | [0, 1] | 1.0 at near-field reference distance, decays with inverse distance model deviation |
| `occlusion_probability` | [0, 1] | 0.0 = fully visible, 1.0 = fully occluded; from Steam Audio occlusion query |
| `hrtf_match_quality` | [0, 1] | 1.0 = HRTF dataset covers this azimuth/elevation exactly, degrades at sparse coverage |
| `combined_confidence` | [0, 1] | Weighted average: 0.4 * distance + 0.3 * (1 - occlusion) + 0.3 * hrtf_match |

### SceneGraph Emitter Slot Extension

```cpp
// Source/SceneGraph.h — added to EmitterSlot
struct ConfidenceMask {
    float distance_confidence   = 1.0f;
    float occlusion_probability = 0.0f;
    float hrtf_match_quality    = 1.0f;
    float combined_confidence   = 1.0f;
};

struct EmitterSlot {
    // ... existing fields ...
    ConfidenceMask confidence;   // NEW — updated by renderer analysis pass
};
```

### Color Gradient Mapping

```javascript
// Source/ui/public/js/index.js
function confidenceToColor(value) {
    // value in [0, 1]
    if (value >= 0.8) return new THREE.Color(0x00cc44);  // green
    if (value >= 0.4) {
        // lerp green -> yellow in [0.4, 0.8]
        const t = (value - 0.4) / 0.4;
        return new THREE.Color().lerpColors(
            new THREE.Color(0xffcc00),  // yellow
            new THREE.Color(0x00cc44),  // green
            t
        );
    }
    // lerp red -> yellow in [0.0, 0.4]
    const t = value / 0.4;
    return new THREE.Color().lerpColors(
        new THREE.Color(0xff2200),  // red
        new THREE.Color(0xffcc00),  // yellow
        t
    );
}
```

### Computation Path

```
processBlock() [audio thread]
    -> NO confidence computation here

SpatialRenderer analysis pass [called from timer callback or message thread]
    -> ConfidenceAnalyzer::update(EmitterSlot[], int count)
       for each emitter:
         distance_confidence  <- f(emitter.distance)
         occlusion_probability <- Steam Audio occlusion result (cached)
         hrtf_match_quality   <- HRTF coverage table lookup
         combined_confidence  <- weighted avg
    -> SceneGraph::updateConfidence(slot_idx, ConfidenceMask)

SceneGraph snapshot -> JSON -> WebView postMessage
    -> Three.js overlay render
```

### JSON Snapshot Extension

```json
{
  "emitters": [
    {
      "id": 0,
      "x": 1.2, "y": 0.0, "z": -2.1,
      "confidence": {
        "distance_confidence": 0.91,
        "occlusion_probability": 0.05,
        "hrtf_match_quality": 0.78,
        "combined_confidence": 0.85
      }
    }
  ]
}
```

---

## 6. RT Invariant Checklist

| Check | Rule |
|---|---|
| No confidence computation in processBlock | All analysis in timer callback or message thread |
| No allocation on SceneGraph write path | ConfidenceMask is a plain struct embedded in EmitterSlot |
| No lock in processBlock | SceneGraph lock-free read; confidence write uses existing update mechanism |
| Three.js geometry reuse | Overlay meshes created once per emitter, updated in-place |

---

## 7. Risks

| Risk | Severity | Mitigation |
|---|---|---|
| Confidence metrics may be subjective / poorly calibrated | Med | Define explicit computation formulas in schema (Section 5); validate with listening tests |
| Overlay visual clarity degrades at high emitter counts (>= 16) | Med | Reduce overlay opacity or simplify to glyph color only at high counts |
| HRTF coverage table not yet available | Med | Use placeholder 1.0 in Slice A; replace in Slice B or as follow-up |
| Scene snapshot payload size increase | Low | Four floats per emitter; negligible at <= 32 emitters |
| Combined confidence weighting is arbitrary | Low | Document formula and allow future tuning; note in ADR if changed |

---

## 8. Validation Plan

| Step | Method | Pass Criteria |
|---|---|---|
| Slice A: snapshot includes confidence | Log scene snapshot JSON | All four fields present for each emitter, values in [0, 1] |
| Slice B: color gradient correct | Manual visual QA — set emitter confidence to 0.0, 0.5, 1.0 via override | Red, yellow, green overlays correct |
| Slice B: no per-frame allocation | Chrome DevTools memory timeline during 30s run | Zero GC pressure from overlay code |
| Slice C: validation matrix | 5 spatial scenarios (see Section 4 Slice C) | All checks PASS |
| Regression | Existing emitter rendering tests | No visual regressions on non-confidence emitter properties |

---

## 9. Files Touched

| File | Action |
|---|---|
| `Source/SceneGraph.h` | MODIFY — Slice A: add ConfidenceMask struct and EmitterSlot field |
| `Source/SpatialRenderer.h` | MODIFY — Slice A: confidence update call in analysis pass |
| `Source/ConfidenceAnalyzer.h` | CREATE (optional) — Slice A: extract confidence logic if complex |
| `Source/ui/public/js/index.js` | MODIFY — Slice B: overlay render pass, confidenceToColor() |
| `TestEvidence/validation-trend.md` | UPDATE — Slice C results |
| `status.json` | UPDATE per slice completion |

---

## 10. ADR References

| ADR | Relevance |
|---|---|
| ADR-0006 | RT invariant — confidence computation must NOT occur in processBlock() |

If the combined_confidence weighting formula changes substantially from the documented default, record a decision note in an ADR or inline annotation before closing Slice C.

---

## 11. Agent Mega-Prompts

### Slice A — Skill-Aware Prompt

```
SKILLS: $skill_plan $reactive-av
BACKLOG: BL-020 Slice A
TASK: Extend SceneGraph emitter slot with confidence data schema

CONTEXT:
- LocusQ is a JUCE VST3/AU/CLAP spatial audio plugin with a WebView (Three.js) UI.
- SceneGraph is a singleton, lock-free structure holding emitter slots.
- BL-014 (SceneGraph emitter slot structure) and BL-019 (data publication to WebView) are Done.
- RT invariant (ADR-0006): no computation in processBlock() — confidence must be
  computed in a timer callback or message thread analysis pass.
- WebView receives scene snapshots as JSON via postMessage bridge.

OBJECTIVE:
1. Define ConfidenceMask struct in Source/SceneGraph.h:
   Fields: distance_confidence float (1.0), occlusion_probability float (0.0),
   hrtf_match_quality float (1.0), combined_confidence float (1.0). All in [0,1].
   Embed in EmitterSlot.

2. Add placeholder confidence update logic — either inline in SpatialRenderer.h
   analysis pass or in a new Source/ConfidenceAnalyzer.h:
   - combined_confidence = 0.4 * distance_confidence
                         + 0.3 * (1.0 - occlusion_probability)
                         + 0.3 * hrtf_match_quality
   - For Slice A: distance_confidence = 1.0, occlusion_probability = 0.0,
     hrtf_match_quality = 1.0 (all placeholders).

3. Extend scene snapshot JSON serialization to include confidence fields per emitter:
   "confidence": {
     "distance_confidence": float,
     "occlusion_probability": float,
     "hrtf_match_quality": float,
     "combined_confidence": float
   }

CONSTRAINTS:
- No confidence computation in processBlock().
- ConfidenceMask must be a plain struct (no heap allocation).
- JSON extension must not break existing snapshot consumers.

DELIVERABLES:
1. Source/SceneGraph.h diff showing ConfidenceMask struct and EmitterSlot extension
2. Confidence update call site (analysis pass location and code snippet)
3. JSON serialization diff
4. Self-check: confirm RT invariant respected
```

---

### Slice A — Standalone Fallback Prompt

```
CONTEXT (no skill files available):
You are adding a confidence data schema to a spatial audio plugin's (LocusQ) scene graph.

PLUGIN ARCHITECTURE:
- Source/SceneGraph.h: singleton holding EmitterSlot array (lock-free).
- Source/SpatialRenderer.h: renderer that processes emitters; has an analysis pass
  called outside processBlock() (on a timer or message thread).
- Source/ui/public/js/index.js: Three.js UI receiving scene snapshots as JSON.
- processBlock() is a real-time audio thread — NO computation allowed there.

WHAT TO IMPLEMENT (Slice A):
1. Add to SceneGraph.h a ConfidenceMask struct with four float fields:
   - distance_confidence (default 1.0)
   - occlusion_probability (default 0.0)
   - hrtf_match_quality (default 1.0)
   - combined_confidence (default 1.0)
   Embed ConfidenceMask as a member of EmitterSlot.

2. In the SpatialRenderer analysis pass (NOT processBlock), add placeholder logic:
   - All placeholders return default values for now (real computation comes in Slice B).
   - Compute combined_confidence = 0.4*distance + 0.3*(1-occlusion) + 0.3*hrtf.

3. Extend the JSON snapshot builder to include confidence fields under each emitter.

OUTPUT:
- SceneGraph.h struct addition
- Analysis pass placeholder code
- JSON snapshot extension
- Confirm: no confidence code in processBlock()
```

---

### Slice B — Skill-Aware Prompt

```
SKILLS: $reactive-av $threejs
BACKLOG: BL-020 Slice B
TASK: Implement confidence overlay rendering in Three.js viewport

CONTEXT:
- Slice A is merged: scene snapshot JSON includes confidence fields per emitter.
- Source/ui/public/js/index.js handles the Three.js scene and receives snapshots.
- Emitter glyphs are already rendered as 3D objects in the scene.
- Overlay must not allocate new meshes per frame — reuse geometry.

OBJECTIVE:
1. Implement confidenceToColor(value) function returning THREE.Color:
   - value >= 0.8: green (#00cc44)
   - value 0.4-0.8: lerp yellow->green
   - value 0.0-0.4: lerp red->yellow
2. On each scene snapshot update, for each emitter:
   - Read combined_confidence from snapshot.
   - Update the emitter glyph's material color or an overlay ring mesh color.
3. Overlay meshes must be created once per emitter slot and reused (update color only).
4. Add a toggleConfidenceOverlay(enabled) function callable from UI state.

DELIVERABLES:
1. confidenceToColor() implementation
2. Overlay render update loop (snapshot -> per-emitter color update)
3. Geometry lifecycle: creation on slot init, color update on snapshot
4. Performance check: confirm no per-frame THREE.Mesh or material creation
```

---

### Slice C — Skill-Aware Prompt

```
SKILLS: $skill_plan $skill_testing
BACKLOG: BL-020 Slice C
TASK: Define and execute confidence masking validation matrix

CONTEXT:
- Slices A and B are merged.
- Confidence values are computed by the analysis pass and visualized in Three.js.
- Need to confirm values update correctly under spatial test scenarios.

OBJECTIVE:
Define a validation matrix with >= 5 test scenarios:
1. Emitter at 0.5m (near-field) — expect high distance_confidence
2. Emitter at 10m (far-field) — expect lower distance_confidence
3. Emitter occluded by geometry — expect high occlusion_probability
4. Emitter at on-axis HRTF position (0 az, 0 el) — expect high hrtf_match_quality
5. Emitter at off-axis extreme (180 az, 45 el) — expect lower hrtf_match_quality

For each scenario:
- Setup: describe emitter configuration
- Expected confidence values: approximate ranges
- Verification method: log snapshot JSON or visual overlay color
- Pass criteria

DELIVERABLES:
1. Validation matrix table (scenario | setup | expected values | verification | pass criteria)
2. Manual QA checklist for visual overlay color
3. Automated smoke test outline: parse snapshot JSON, assert confidence fields in [0,1]
4. Results entry format for TestEvidence/validation-trend.md
```

---

## 12. Closeout Checklist

- [ ] Slice A: ConfidenceMask struct added to SceneGraph.h
- [ ] Slice A: Placeholder confidence values flowing through analysis pass
- [ ] Slice A: Scene snapshot JSON includes confidence fields for all emitters
- [ ] Slice B: confidenceToColor() implemented in index.js
- [ ] Slice B: Overlay geometry lifecycle correct (no per-frame alloc)
- [ ] Slice B: Color gradient visually correct for low/mid/high confidence values
- [ ] Slice C: Validation matrix >= 5 scenarios, all PASS
- [ ] Slice C: Results logged in `TestEvidence/validation-trend.md`
- [ ] RT invariant confirmed: zero confidence computation in processBlock()
- [ ] `status.json` updated with BL-020 completion
