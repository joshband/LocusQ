---
Title: BL-028 Spatial Output Matrix
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# BL-028 — Spatial Output Matrix

## 1. Summary

Enforce spatial output matrix (SOM) legality through a `RendererDomain` enum (`InternalBinaural` / `Multichannel` / `ExternalSpatial`) with seven SOM rules that block invalid domain + bus + head-tracking combinations. Delivers a device profile contract and a structured head-tracking state model, with full scene-state telemetry additions and integration validation across CALIBRATE and RENDERER panels.

| Field | Value |
|---|---|
| ID | BL-028 |
| Status | In Planning |
| Priority | P2 |
| Track | A (DSP/Architecture) + C (UX Authoring) |
| Effort | High / L total; Med / M per slice |
| Depends | BL-017, BL-026, BL-027 |
| Blocks | BL-029 |
| Annex | `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md` |

---

## 2. Objective

The LocusQ renderer currently allows configurations that are physically or computationally invalid — for example, enabling head-tracking on a multichannel bus that has no head-tracking source, or activating external spatial routing when the plugin is running in a binaural-only context. This item enforces legality at the domain boundary.

Deliverables:

1. `RendererDomain` enum in `SpatialRenderer.h` with three values: `InternalBinaural`, `Multichannel`, `ExternalSpatial`.
2. Seven SOM enforcement rules evaluated in `PluginProcessor.cpp` on every parameter change, blocking illegal combinations before they reach `processBlock()`.
3. A device profile contract defining four profile archetypes: `generic`, `airpods_pro_2`, `sony_wh1000xm5`, `custom_sofa`.
4. A `HeadTrackingSource` enum and `HeadTrackingState` struct covering source type, confidence, and latency.
5. Scene-state telemetry additions: `domain`, `tracking`, and `device` fields in the bridge payload.
6. Integration validation confirming that CALIBRATE and RENDERER panels display the correct domain and tracking state and enforce SOM rules in their selectors.

---

## 3. Normative References

- `Source/SpatialRenderer.h` — renderer domain and profile enum definition target
- `Source/PluginProcessor.cpp` — SOM enforcement logic, bridge payload serialization
- `Source/PluginEditor.cpp` — panel bridge integration
- `Source/ui/public/js/index.js` — CALIBRATE and RENDERER panel domain/tracking display
- `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md` — SOM rule table, RendererDomain enum definition, HeadTrackingState struct
- `Documentation/invariants.md` — RT invariant: no alloc/lock/blocking in processBlock()
- `Documentation/adr/ADR-0006.md` — device profile authority
- `Documentation/adr/ADR-0012.md` — renderer domain exclusivity and matrix gating
- `Documentation/scene-state-contract.md` — bridge payload schema

---

## 4. SOM Rule Table

The seven SOM rules from the annex spec are reproduced here for agent reference. Every rule is evaluated on parameter change, not in processBlock().

| Rule | Domain | Bus Config | Head Tracking | Decision |
|---|---|---|---|---|
| SOM-1 | InternalBinaural | Stereo (2ch) | Any source | ALLOW |
| SOM-2 | InternalBinaural | >2ch | Any source | BLOCK — binaural is stereo only |
| SOM-3 | Multichannel | >=4ch | None | ALLOW |
| SOM-4 | Multichannel | >=4ch | HeadTracking active | BLOCK — HT not valid on multichannel |
| SOM-5 | ExternalSpatial | Any ch | Any source | ALLOW with passthrough warning |
| SOM-6 | Any | Stereo (2ch) | HeadTracking active | ALLOW only if domain == InternalBinaural |
| SOM-7 | InternalBinaural | Stereo (2ch) | Steam HT active + no HRTF | BLOCK — requires HRTF |

On BLOCK: parameter change is rejected, previous legal state is restored, and a diagnostic event is emitted to the bridge payload (`som_block_reason` field).

---

## 5. Entry Criteria

### Global Entry
- BL-017 (foundational renderer plumbing) is merged.
- BL-026 Slice A (shared alias dictionary) is merged.
- BL-027 Slice E (cross-panel coherence) is merged.
- `status.json` reflects all three dependencies in done/verified state.
- No open RT-safety violations in `TestEvidence/build-summary.md`.

### Per-Slice Entry
| Slice | Entry Gate |
|---|---|
| A | BL-026, BL-027 done; SpatialRenderer.h profile enums stable |
| B | Slice A merged; RendererDomain enforced in PluginProcessor |
| C | Slice B merged; device profiles accepted by PluginProcessor |
| D | Slice C merged; HeadTrackingState struct defined and initialized |
| E | Slice D merged; telemetry fields in bridge payload |

---

## 6. Slices

### Slice A — RendererDomain Enum + Matrix Enforcement Logic

**Goal:** Define `RendererDomain` enum in `SpatialRenderer.h` and implement the seven SOM rule evaluator in `PluginProcessor.cpp`. The evaluator runs on every parameter change (APVTS listener), never in processBlock(). On BLOCK, restore previous parameter values and emit `som_block_reason`.

**Files:** `Source/SpatialRenderer.h`, `Source/PluginProcessor.cpp`

**C++ target definitions (from annex spec):**
```cpp
enum class RendererDomain {
    InternalBinaural,   // Stereo binaural output, HRTF required
    Multichannel,       // 4+ channel speaker output, no head tracking
    ExternalSpatial     // Plugin is downstream of external spatializer
};

struct SomResult {
    bool allowed;
    const char* block_reason; // nullptr if allowed
};

SomResult evaluateSom(RendererDomain domain,
                      int busChannelCount,
                      bool headTrackingActive,
                      bool hrtfLoaded);
```

**Acceptance:**
- All seven SOM rules produce correct ALLOW/BLOCK for synthesized inputs (unit test).
- BLOCK path restores previous parameter values without audio dropout.
- `som_block_reason` appears in bridge payload within one poll cycle of a BLOCK event.
- No allocation in processBlock().
- UI-P2-028A lane passes.

---

### Slice B — Device Profile Contract

**Goal:** Define and register four device profile archetypes. Each profile specifies: domain, output channel count, head-tracking capability, and SOFA file path or "none". PluginProcessor validates that active domain is consistent with the selected device profile via SOM rules.

**Profiles:**
| ID | Label | Domain | Channels | HT Capable | SOFA |
|---|---|---|---|---|---|
| generic | Generic | Multichannel | 2..8 | No | none |
| airpods_pro_2 | AirPods Pro 2 | InternalBinaural | 2 | Yes | bundled |
| sony_wh1000xm5 | Sony WH-1000XM5 | InternalBinaural | 2 | No | bundled |
| custom_sofa | Custom SOFA | InternalBinaural | 2 | Optional | user-provided |

**Files:** `Source/SpatialRenderer.h`, `Source/PluginProcessor.cpp`

**Acceptance:**
- Selecting a profile automatically sets domain to the profile's default domain.
- Cross-profile SOM rule SOM-7 blocks airpods_pro_2 profile if HRTF load fails.
- Custom SOFA profile defers HT capability to user configuration.
- UI-P2-028B lane passes.

---

### Slice C — Head Tracking State Model

**Goal:** Define `HeadTrackingSource` enum and `HeadTrackingState` struct in `SpatialRenderer.h`. Maintain live state in `PluginProcessor`. Source types: `None`, `SteamAudio`, `CoreMotion`, `External`. State fields: source, confidence (0.0–1.0), latency_ms, last_update_time.

**Files:** `Source/SpatialRenderer.h`, `Source/PluginProcessor.cpp`

**C++ target definitions (from annex spec):**
```cpp
enum class HeadTrackingSource {
    None,
    SteamAudio,
    CoreMotion,
    External
};

struct HeadTrackingState {
    HeadTrackingSource source;
    float confidence;       // 0.0 - 1.0
    float latency_ms;
    int64_t last_update_time_us; // microseconds, monotonic
};
```

**Acceptance:**
- HeadTrackingState is updated from the appropriate source without blocking the audio thread (SPSC or atomic snapshot).
- SOM evaluator reads HeadTrackingState atomically.
- UI-P2-028C lane passes.

---

### Slice D — Scene-State Telemetry Additions

**Goal:** Add `domain`, `tracking`, and `device` fields to the bridge scene-state payload. `domain` is a string matching the RendererDomain enum name. `tracking` is an object with source, confidence, latency_ms. `device` is an object with profile id and label.

**Bridge payload additions (JSON):**
```json
{
  "renderer": {
    "domain": "InternalBinaural",
    "device": { "id": "airpods_pro_2", "label": "AirPods Pro 2" },
    "tracking": {
      "source": "SteamAudio",
      "confidence": 0.97,
      "latency_ms": 4.2
    },
    "som_block_reason": null
  }
}
```

**Files:** `Source/PluginProcessor.cpp`

**Acceptance:**
- Fields are present in every bridge payload after Slice C is merged.
- `som_block_reason` is null when no BLOCK has occurred since last poll.
- Bridge serialization does not allocate on the audio thread.
- UI-P2-028D lane passes.

---

### Slice E — Integration Validation Across CALIBRATE/RENDERER Panels

**Goal:** CALIBRATE and RENDERER panels display domain, tracking, and device fields from the bridge payload. Domain selector in RENDERER enforces SOM rules visually (disabled states for illegal domain options given current device profile and HT state). CALIBRATE panel shows tracking confidence badge.

**Files:** `Source/ui/public/js/index.js`, `Source/PluginEditor.cpp`

**Acceptance:**
- Domain selector disables illegal options per SOM rules at runtime.
- Tracking confidence badge updates within one poll cycle.
- SOM block events surface a non-modal inline warning in the RENDERER panel.
- UI-P2-028E lane passes.
- Docs freshness gate passes.

---

## 7. ADR Obligations

| ADR | Obligation |
|---|---|
| ADR-0006 | Device profile authority: profile archetypes defined in PluginProcessor, not in UI |
| ADR-0012 | Renderer domain exclusivity: only one RendererDomain active; switching domains must pass SOM evaluator |

If SOM rule additions require relaxing ADR-0012 (for example, a transitional domain state during switching), record a new ADR before closing the slice that introduces the relaxation.

---

## 8. RT-Safety Checklist

For every code change touching `Source/PluginProcessor.cpp` or `Source/SpatialRenderer.h`:

- [ ] No `new` / `delete` / `malloc` / `free` on the audio thread
- [ ] No `std::mutex`, `std::lock_guard`, or any blocking primitive in `processBlock()`
- [ ] SOM evaluator called from APVTS listener (message thread), never from processBlock()
- [ ] HeadTrackingState read in processBlock() via atomic snapshot or SPSC ring, never under lock
- [ ] Bridge payload serialization occurs on message thread, not audio thread
- [ ] Parameter restore on BLOCK path uses APVTS setValue from message thread only

---

## 9. Validation Lanes

| Lane | Trigger | Pass Criteria |
|---|---|---|
| UI-P2-028A | Slice A merge | All 7 SOM rules produce correct ALLOW/BLOCK in unit test |
| UI-P2-028B | Slice B merge | Device profile selection sets domain and triggers SOM evaluation |
| UI-P2-028C | Slice C merge | HeadTrackingState updates without audio thread blocking |
| UI-P2-028D | Slice D merge | Bridge payload includes domain, tracking, device, som_block_reason |
| UI-P2-028E | Slice E merge | RENDERER panel disables illegal domain options; CALIBRATE shows tracking badge |
| FRESHNESS | Each slice merge | `./scripts/validate-docs-freshness.sh` exits 0 |

---

## 10. Risks and Mitigations

| Risk | Severity | Mitigation |
|---|---|---|
| Domain switching during active processing causes audio dropout | High | Domain switch routed through APVTS listener; processBlock reads domain atomically |
| Double-spatialization detection reliability (ExternalSpatial domain) | High | SOM-5 emits passthrough warning but does not block; detection is advisory not enforcement |
| HeadTrackingState latency inconsistency across sources | Medium | Normalize to microsecond monotonic timestamps in all sources |
| Custom SOFA path validation at runtime (missing file) | Medium | Defer to SOM-7 block path; emit `som_block_reason: hrtf_missing` |
| SOM rule complexity grows with new devices | Low | SOM evaluator is a pure function; table-driven extension is safe |

---

## 11. Agent Mega-Prompts

### Slice A — Skill-Aware Prompt

```
Skills: $spatial-audio-engineering $skill_impl

Context:
- You are implementing BL-028 Slice A in the LocusQ JUCE spatial audio plugin.
- BL-026 and BL-027 are merged. Profile selector and RENDERER panel are live.
- SpatialRenderer.h currently contains spatial profile enums near line 60.
- PluginProcessor.cpp contains the APVTS parameter change listener.
- RT invariant: processBlock() must never allocate, lock, or block.
- ADR-0012: only one RendererDomain is active at a time.

Task:
1. Read Source/SpatialRenderer.h lines 50-90 for existing enum layout.
2. Add the RendererDomain enum and SomResult struct to SpatialRenderer.h:
   enum class RendererDomain { InternalBinaural, Multichannel, ExternalSpatial };
   struct SomResult { bool allowed; const char* block_reason; };
3. Implement evaluateSom(domain, busChannelCount, headTrackingActive, hrtfLoaded)
   as a pure function in SpatialRenderer.h (or a .cpp translation unit).
   Implement all 7 SOM rules from the rule table in this runbook section 4.
4. In PluginProcessor.cpp, locate the APVTS parameter change listener.
   Call evaluateSom on every domain/bus/tracking parameter change.
   On BLOCK: restore previous parameter values using APVTS.setValue on message thread.
   Emit som_block_reason into the bridge state cache (not processBlock).
5. Write a unit test (or use the existing test harness) that feeds all 7 rule
   combinations and asserts ALLOW/BLOCK correctness.
6. Run UI-P2-028A validation lane.
7. Update status.json: BL-028 slice_a = "done".
8. List changed files, test results, validation result.

Constraints:
- evaluateSom is a pure function with no side effects.
- Do not call evaluateSom from processBlock().
- Do not allocate in processBlock().
```

### Slice A — Standalone Fallback Prompt

```
Context (no skills loaded):
- Repository: LocusQ — JUCE VST3/AU/CLAP spatial audio plugin with WebView UI.
- SpatialRenderer.h defines renderer types near line 60.
- PluginProcessor.cpp is the JUCE AudioProcessor subclass. It has an APVTS and
  a parameter change listener callback.
- RT invariant: processBlock() must not allocate, lock, or block. Never violated.

Task:
1. Open Source/SpatialRenderer.h. Add after the existing enums:
   enum class RendererDomain { InternalBinaural, Multichannel, ExternalSpatial };
   struct SomResult { bool allowed; const char* block_reason; };
   SomResult evaluateSom(RendererDomain domain, int busChannelCount,
                         bool headTrackingActive, bool hrtfLoaded);

2. Create Source/SomEvaluator.cpp (or add to SpatialRenderer.cpp if it exists).
   Implement evaluateSom with the 7 rules from this document's section 4.
   Rules are pure logic: no I/O, no allocation, no side effects.

3. In PluginProcessor.cpp, find the parameter listener (parameterChanged or
   AudioProcessorValueTreeState::Listener::parameterChanged).
   After parameter update, call evaluateSom with current state.
   On !allowed: call apvts.getParameter(id)->setValue(previousValue) for each
   changed parameter. Write block_reason to a std::atomic<const char*> bridge
   cache field.

4. Confirm processBlock() is not modified.
5. List all files changed with a one-sentence description per file.
```

### Slice B — Agent Prompt

```
Skills: $spatial-audio-engineering $skill_impl

Context:
- BL-028 Slice A is merged. RendererDomain enum and SOM evaluator are live.
- Device profiles define domain, channel count, HT capability, and SOFA path.
- Selecting a device profile must set domain automatically and re-evaluate SOM.

Task:
1. Read Source/SpatialRenderer.h for RendererDomain enum.
2. Define a DeviceProfile struct in SpatialRenderer.h:
   struct DeviceProfile {
       const char* id;
       const char* label;
       RendererDomain default_domain;
       int output_channels;
       bool ht_capable;
       const char* sofa_path; // "none" or file path
   };
3. Register the four archetypes (generic, airpods_pro_2, sony_wh1000xm5,
   custom_sofa) in PluginProcessor.cpp as a static constexpr array.
4. Wire profile selection to: set domain = profile.default_domain, then call
   evaluateSom, then update APVTS. On BLOCK (e.g., SOM-7 for airpods_pro_2
   when HRTF missing), revert profile selection.
5. Run UI-P2-028B lane.
6. List changed files and validation result.
```

### Slice C — Agent Prompt

```
Skills: $steam-audio-capi $spatial-audio-engineering

Context:
- BL-028 Slice B is merged. Device profiles are live.
- HeadTrackingState must be readable from processBlock() without locking.
  Use an atomic snapshot pattern: HeadTrackingState is written by the HT source
  thread and snapshotted atomically (or via SPSC) for processBlock reads.

Task:
1. Add HeadTrackingSource enum and HeadTrackingState struct to SpatialRenderer.h
   per the definitions in this runbook section 6 (Slice C).
2. In PluginProcessor.cpp, add a HeadTrackingState atomic snapshot member.
   Implement an updateHeadTracking() method called from the HT source (Steam
   Audio callback or CoreMotion delegate, not from processBlock).
   processBlock reads via std::atomic<HeadTrackingState>::load(relaxed) or
   equivalent lock-free pattern.
3. Wire headTrackingActive in evaluateSom calls to use the current snapshot.
4. Run UI-P2-028C lane.
5. List changed files, confirm no lock in processBlock, validation result.
```

### Slice D — Agent Prompt

```
Skills: $juce-webview-runtime $skill_impl

Context:
- BL-028 Slice C is merged. HeadTrackingState is live.
- The bridge scene-state payload is serialized in PluginProcessor.cpp on the
  message thread. processBlock() does not touch the bridge.

Task:
1. Read Source/PluginProcessor.cpp for the bridge payload serialization path.
2. Add the following fields to the renderer object in the JSON payload:
   - domain: string (RendererDomain enum name)
   - device: { id: string, label: string }
   - tracking: { source: string, confidence: float, latency_ms: float }
   - som_block_reason: string or null
3. Clear som_block_reason to null after each successful poll that consumed it.
4. Ensure serialization allocates only on the message thread (juce::MessageManager
   thread or JUCE timer callback).
5. Run UI-P2-028D lane.
6. List changed files and validation result.
```

### Slice E — Agent Prompt

```
Skills: $juce-webview-runtime $threejs $skill_design

Context:
- BL-028 Slices A-D are merged. Bridge payload includes domain, tracking, device,
  som_block_reason.
- index.js must reflect these fields in CALIBRATE and RENDERER panels.
- RENDERER domain selector must disable illegal options per SOM rules at render time.

Task:
1. Read Source/ui/public/js/index.js for the RENDERER domain selector and
   CALIBRATE panel tracking badge (if any exist from BL-027).
2. In the RENDERER panel domain selector:
   - Compute allowed domains for current device profile and tracking state using
     the JS equivalent of the SOM rule table (implement as a pure JS function).
   - Disable (visually grey + aria-disabled) options that would be blocked.
3. In the CALIBRATE panel, add a tracking confidence badge showing:
   source label, confidence as a percentage bar, latency_ms.
4. When som_block_reason is non-null, show an inline non-modal warning in the
   RENDERER panel that auto-dismisses after 4 seconds.
5. Run UI-P2-028E lane and docs freshness gate.
6. Update status.json: BL-028 status = "done".
7. List changed files, test results, freshness gate result.
```

---

## 12. Closeout Criteria

- [ ] All five slices merged and validation lanes passed.
- [ ] `status.json` updated: BL-028 status = "done", evidence references to TestEvidence entries.
- [ ] `TestEvidence/validation-trend.md` updated with BL-028 row.
- [ ] `TestEvidence/build-summary.md` updated with BL-028 build summary.
- [ ] Annex spec (`Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-24.md`) updated to reflect implementation decisions (especially any SOM rule refinements).
- [ ] No new RT-safety violations in build summary.
- [ ] ADR-0012 reviewed; if any relaxation was made, a new ADR is filed.
- [ ] Docs freshness gate passes: `./scripts/validate-docs-freshness.sh` exits 0.
- [ ] BL-029 entry criteria can be satisfied (domain and tracking fields present in bridge payload).
