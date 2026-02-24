---
Title: BL-027 Renderer UX v2
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# BL-027 — Renderer UX v2

## 1. Summary

Redesign the RENDERER panel to be profile-authoritative with explicit requested/active/stage diagnostics, dynamic output/speaker presentation, structured Steam Audio and Ambisonic diagnostic cards, and full cross-panel coherence with CALIBRATE v2 (BL-026).

| Field | Value |
|---|---|
| ID | BL-027 |
| Status | In Planning |
| Priority | P2 |
| Track | C — UX Authoring |
| Effort | High / XL total; Med / M per slice |
| Depends | BL-026 |
| Blocks | BL-028, BL-029 |
| Annex | `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md` |

---

## 2. Objective

The current RENDERER panel presents static output configuration without reflecting live profile state or diagnostic context. This backlog item delivers:

1. A profile selector that consumes the shared alias dictionary established by BL-026, keeping chip labels consistent across panels.
2. A dynamic output and speaker map that adapts to the active profile's channel count at runtime.
3. A Steam Audio diagnostic card surfacing HRTF status, binaural quality tier, and fallback state drawn from scene-state-contract diagnostics fields.
4. An Ambisonic diagnostic card surfacing ambisonics order, channel count, and decoder state.
5. Host integration and cross-panel coherence with CALIBRATE v2 so that profile selection in either panel propagates correctly without race conditions.

Success criteria: RENDERER panel reflects active profile within one render cycle of a profile switch; all five diagnostic fields are visible and accurate under normal, degraded, and fallback runtime conditions; cross-panel chip labels are identical to CALIBRATE v2 chips.

---

## 3. Normative References

- `Source/SpatialRenderer.h` — spatial profile enums (~line 60), renderer domain types
- `Source/PluginProcessor.cpp` — renderer parameter update path, processBlock RT invariant
- `Source/ui/public/index.html` — WebView host page, panel mount points
- `Source/ui/public/js/index.js` — Three.js scene, UI event handlers, bridge calls
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md` — shared alias dictionary contract
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md` — full slice spec
- `Documentation/invariants.md` — RT invariant: no alloc/lock/blocking in processBlock()
- `Documentation/adr/ADR-0006.md` — device profile authority
- `Documentation/adr/ADR-0011.md` — standalone renderer audition source
- `Documentation/adr/ADR-0012.md` — renderer domain exclusivity and matrix gating
- `Documentation/scene-state-contract.md` — diagnostics fields: requested, active, stage

---

## 4. Entry Criteria

### Global Entry
- BL-026 Slice A (profile selector shared alias dictionary) is complete and merged.
- `status.json` reflects BL-026 in a closed or verified state.
- No RT-safety violations are open in `TestEvidence/build-summary.md`.

### Per-Slice Entry
| Slice | Entry Gate |
|---|---|
| A | BL-026 Slice A done; alias dictionary exported from PluginProcessor parameter group |
| B | Slice A merged; profile channel count exposed on scene-state bridge |
| C | Slice B merged; Steam Audio diagnostic fields present in scene-state payload |
| D | Slice C merged; Ambisonic decoder state field present in scene-state payload |
| E | Slice D merged; both diagnostic cards pass display validation |

---

## 5. Slices

### Slice A — Profile Selector with Shared Alias Dictionary

**Goal:** Render a profile chip/selector in the RENDERER panel that reads from the same alias dictionary BL-026 established. Profile selection propagates through PluginProcessor's renderer parameter update path. No duplicate alias definitions.

**Files:** `Source/PluginProcessor.cpp`, `Source/ui/public/index.html`, `Source/ui/public/js/index.js`

**Acceptance:**
- Profile chip labels match CALIBRATE v2 chip labels exactly (string equality).
- Selecting a profile in RENDERER triggers PluginProcessor parameter change identical in effect to CALIBRATE panel selection.
- No additional heap allocation occurs on the audio thread during selection propagation.
- UI-P2-027A lane passes.

---

### Slice B — Dynamic Output/Speaker Presentation

**Goal:** The speaker map in the RENDERER panel adapts at runtime to the profile's channel count. Mono, stereo, quad, 5.1, 7.1, and binaural (2ch head-tracked) layouts are supported. Layout transitions are animated and do not flicker.

**Files:** `Source/ui/public/js/index.js`, `Source/SpatialRenderer.h`

**Acceptance:**
- Speaker map re-renders within one animation frame of a profile switch.
- Channel labels are accurate for each layout.
- Binaural layout shows head model, not speaker icons.
- UI-P2-027B lane passes.

---

### Slice C — Steam Audio Diagnostic Card

**Goal:** A dedicated card in the RENDERER panel surfaces five Steam Audio runtime fields from scene-state-contract diagnostics: HRTF load status (ok/missing/fallback), binaural quality tier (full/degraded/off), reflection engine state (active/disabled), convolution method (true-conv/hybrid/parametric), and last error string if present.

**Files:** `Source/ui/public/js/index.js`, `Source/SpatialRenderer.h`

**Acceptance:**
- All five fields update within one bridge poll cycle of a runtime state change.
- Fallback state is visually distinguished (amber/red indicator).
- Card is collapsible and collapsed by default.
- UI-P2-027C lane passes.

---

### Slice D — Ambisonic Diagnostic Card

**Goal:** A dedicated card surfaces Ambisonic runtime fields: order (1/2/3), channel count (4/9/16), decoder state (active/bypassed/error), and active decoder type (AllRAD/basic/none).

**Files:** `Source/ui/public/js/index.js`, `Source/SpatialRenderer.h`

**Acceptance:**
- Order and channel count are consistent (order N → (N+1)^2 channels).
- Decoder state reflects live renderer state, not parameter defaults.
- Card is collapsible and collapsed by default.
- UI-P2-027D lane passes.

---

### Slice E — Host Integration and Cross-Panel Coherence

**Goal:** Profile selection in either RENDERER or CALIBRATE propagates to both panels simultaneously with no stale chip state. Bridge message ordering is deterministic. Profile switch latency from user gesture to both panels reflecting new state is under 50 ms on reference hardware.

**Files:** `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js`

**Acceptance:**
- Profile switch stress test (10 rapid switches) produces no stale-label observation.
- No additional IPC round-trips beyond the existing bridge poll cycle.
- RENDERER and CALIBRATE chip labels are identical at rest and after each switch.
- UI-P2-027E lane passes.
- Docs freshness gate (`./scripts/validate-docs-freshness.sh`) passes.

---

## 6. ADR Obligations

| ADR | Obligation |
|---|---|
| ADR-0006 | Device profile alias authority resides in PluginProcessor; UI is read-only consumer |
| ADR-0011 | Standalone audition source selection is RENDERER-panel-authoritative; do not duplicate in CALIBRATE |
| ADR-0012 | Renderer domain exclusivity: only one domain active at a time; selector must enforce this |

If Slice A or E requires a change that conflicts with ADR-0011 or ADR-0012, record a new ADR before closing the slice.

---

## 7. RT-Safety Checklist

For every code change touching `Source/PluginProcessor.cpp` or `Source/SpatialRenderer.h`:

- [ ] No `new` / `delete` / `malloc` / `free` on the audio thread
- [ ] No `std::mutex`, `std::lock_guard`, or any blocking primitive in `processBlock()`
- [ ] No file I/O or system calls in `processBlock()`
- [ ] SPSC queues or atomic flags used for bridge-to-audio-thread communication
- [ ] Profile switch parameter updates routed through JUCE `AudioProcessorValueTreeState` change callback, not called directly from audio thread

---

## 8. Validation Lanes

| Lane | Trigger | Pass Criteria |
|---|---|---|
| UI-P2-027A | Slice A merge | Profile chip labels match BL-026 alias dictionary |
| UI-P2-027B | Slice B merge | Speaker map renders correct layout for each profile channel count |
| UI-P2-027C | Slice C merge | Steam Audio diagnostic card shows all five fields accurately |
| UI-P2-027D | Slice D merge | Ambisonic card shows order, channel count, decoder state accurately |
| UI-P2-027E | Slice E merge | Cross-panel coherence under 10-switch stress test |
| FRESHNESS | Each slice merge | `./scripts/validate-docs-freshness.sh` exits 0 |

---

## 9. Risks and Mitigations

| Risk | Severity | Mitigation |
|---|---|---|
| Profile chip naming unsettled across panels | High | BL-026 Slice A must freeze alias dictionary before BL-027 Slice A begins |
| Compact-layout thresholds for speaker map TBD | Medium | Define breakpoints in annex spec before Slice B; hard-code initial thresholds |
| Cross-panel state sync complexity (Slice E) | High | Use single source of truth in PluginProcessor APVTS; both panels read, neither writes independently |
| Steam Audio HRTF load timing races with diagnostic card init | Medium | Card reads cached scene-state field; initialize to "unknown" not "ok" |
| Slice H (BL-029 dependency) may pull design changes back | Low | Treat BL-027 as self-contained; BL-029 consumes but does not modify RENDERER panel layout |

---

## 10. Effort and Sequencing

| Slice | Effort | Parallelizable |
|---|---|---|
| A | M | No — gate for all others |
| B | M | No — requires Slice A |
| C | M | No — requires Slice B |
| D | S | No — requires Slice C |
| E | M | No — requires Slice D |

Total: ~XL. Recommended sprint allocation: one slice per sprint day, starting only after BL-026 Slice A is verified.

---

## 11. Agent Mega-Prompts

### Slice A — Skill-Aware Prompt

```
Skills: $skill_design $juce-webview-runtime

Context:
- You are implementing BL-027 Slice A in the LocusQ JUCE spatial audio plugin.
- BL-026 Slice A is complete. The shared profile alias dictionary lives in
  PluginProcessor.cpp (look for the parameter group or alias registration block).
- SpatialRenderer.h contains spatial profile enums near line 60.
- The WebView bridge between PluginProcessor and the JS UI uses a JSON scene-state
  payload polled by index.js.
- RT invariant: processBlock() must never allocate, lock, or block.

Task:
1. Read Source/SpatialRenderer.h lines 50-80 to identify the profile enum names.
2. Read Source/PluginProcessor.cpp and locate the renderer parameter update path and
   any existing alias dictionary or chip label registration from BL-026.
3. Read Source/ui/public/js/index.js and locate the RENDERER panel mount point and
   any existing profile selector code.
4. Implement or extend the profile selector in the RENDERER panel so it:
   a. Reads profile chip labels from the same alias dictionary BL-026 registered.
   b. Sends a profile selection bridge message that routes through
      PluginProcessor's renderer parameter update path.
   c. Does not allocate on the audio thread.
5. Verify chip label strings are identical to CALIBRATE v2 chip labels (string equality).
6. Run the UI-P2-027A validation lane and report result.
7. Update status.json: set BL-027 slice_a to "in_progress" at start, "done" at end.

Constraints:
- Do not define a second alias dictionary. Import or reference BL-026's registration.
- Do not touch processBlock() directly.
- Output: list of changed files, diff summary, validation result.
```

### Slice A — Standalone Fallback Prompt

```
Context (no skills loaded):
- Repository: LocusQ — JUCE VST3/AU/CLAP spatial audio plugin with WebView UI.
- WebView UI is in Source/ui/public/js/index.js (Three.js scene + bridge calls).
- Audio host is Source/PluginProcessor.cpp (JUCE AudioProcessor subclass).
- Profile enums are in Source/SpatialRenderer.h near line 60.
- BL-026 added a shared profile alias dictionary. Find it in PluginProcessor.cpp.
- RT invariant: processBlock() has no alloc, lock, or blocking call. Do not violate this.

Task:
1. Locate the profile alias dictionary added by BL-026 in PluginProcessor.cpp.
2. Locate the RENDERER panel section in index.js.
3. Add a profile selector chip row in the RENDERER panel that reads alias labels
   from the bridge scene-state payload (do not hard-code strings in JS).
4. Wire the chip click to send a bridge message that updates the renderer profile
   parameter through PluginProcessor's existing parameter update path.
5. Confirm no new heap allocation is introduced on the audio thread.
6. List all files changed and describe changes in one paragraph each.
```

### Slice B — Agent Prompt

```
Skills: $juce-webview-runtime $threejs

Context:
- BL-027 Slice A is merged. Profile selector is live in RENDERER panel.
- SpatialRenderer.h exposes channel count per profile.
- The scene-state bridge payload must include the active profile's channel count.
- index.js renders a speaker map; it must re-render on profile switch.

Task:
1. Read Source/SpatialRenderer.h to find channel count per profile enum value.
2. Confirm scene-state bridge payload includes channel_count field; if absent, add it
   in PluginProcessor.cpp (bridge serialization path, not processBlock).
3. In index.js, implement a speaker map component that renders:
   - 1ch: center speaker icon
   - 2ch stereo: L/R icons
   - 2ch binaural: head model, no speaker icons
   - 4ch quad: L/R/Ls/Rs icons
   - 6ch 5.1: L/C/R/Ls/Rs/LFE icons
   - 8ch 7.1: L/C/R/Ls/Rs/Lss/Rss/LFE icons
4. Trigger re-render on profile switch with a CSS transition (no flicker).
5. Validate with UI-P2-027B lane.
6. List changed files and validation result.
```

### Slice C — Agent Prompt

```
Skills: $steam-audio-capi $juce-webview-runtime

Context:
- BL-027 Slice B is merged. Speaker map is live.
- Steam Audio C API runtime state is tracked in SpatialRenderer.h / PluginProcessor.cpp.
- scene-state-contract.md defines diagnostics fields: requested, active, stage.
- The diagnostic card must not allocate on any thread; it reads from cached bridge state.

Task:
1. Read Documentation/scene-state-contract.md (if present) for the diagnostics schema.
2. Read Source/SpatialRenderer.h for Steam Audio state fields (HRTF load status,
   binaural quality, fallback state, reflection engine, convolution method).
3. Ensure these five fields are serialized into the bridge scene-state payload in
   PluginProcessor.cpp (bridge serialization path only, not processBlock).
4. In index.js, add a collapsible Steam Audio diagnostic card in the RENDERER panel
   with the five fields. Card defaults to collapsed. Fallback state uses amber/red
   indicator class.
5. Validate with UI-P2-027C lane.
6. List changed files and validation result.
```

### Slice D — Agent Prompt

```
Skills: $spatial-audio-engineering $juce-webview-runtime

Context:
- BL-027 Slice C is merged. Steam Audio diagnostic card is live.
- SpatialRenderer.h tracks Ambisonic decoder state (order, channel count, decoder type).
- Ambisonic order N must satisfy channel_count == (N+1)^2.

Task:
1. Read Source/SpatialRenderer.h for Ambisonic state fields.
2. Ensure order, channel_count (ambisonics), decoder_state, and decoder_type are
   serialized into the bridge payload in PluginProcessor.cpp.
3. In index.js, add a collapsible Ambisonic diagnostic card in the RENDERER panel.
   Validate that rendered channel count equals (order+1)^2; log a console warning
   if inconsistent.
4. Validate with UI-P2-027D lane.
5. List changed files and validation result.
```

### Slice E — Agent Prompt

```
Skills: $juce-webview-runtime $skill_design

Context:
- BL-027 Slices A-D are merged. Both diagnostic cards live.
- Cross-panel coherence means: profile selection in CALIBRATE or RENDERER must
  propagate to both panels within one bridge poll cycle.
- Single source of truth: PluginProcessor APVTS. Both panels read; neither writes
  an independent profile state.

Task:
1. Read Source/PluginEditor.cpp for the bridge poll/push mechanism.
2. Read Source/PluginProcessor.cpp for APVTS parameter change listener registration.
3. Verify that profile parameter changes from either panel flow:
   UI gesture -> bridge message -> PluginProcessor APVTS -> value change callback
   -> bridge scene-state update -> both panels re-render.
4. Stress test: simulate 10 rapid profile switches in the JS test harness or
   browser console. Confirm both panels show identical chip labels after each switch.
5. Run UI-P2-027E lane and docs freshness gate.
6. Update status.json: set BL-027 to "done" with evidence note.
7. List changed files, test results, and freshness gate result.
```

---

## 12. Closeout Criteria

- [ ] All five slices merged and validation lanes passed.
- [ ] `status.json` updated: BL-027 status = "done", evidence references to TestEvidence entries.
- [ ] `TestEvidence/validation-trend.md` updated with BL-027 row.
- [ ] `TestEvidence/build-summary.md` updated with BL-027 build summary.
- [ ] Annex spec (`Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md`) updated to reflect any design changes made during implementation.
- [ ] No new RT-safety violations in build summary.
- [ ] Docs freshness gate passes: `./scripts/validate-docs-freshness.sh` exits 0.
- [ ] BL-028 entry criteria can be satisfied (RENDERER domain selector visible and connected to PluginProcessor).
