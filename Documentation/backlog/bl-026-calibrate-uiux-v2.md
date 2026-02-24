Title: BL-026 CALIBRATE UI/UX V2 Multi-Topology
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-026: CALIBRATE UI/UX V2 Multi-Topology

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | In Planning |
| Owner Track | Track C — UX Authoring |
| Depends On | BL-025 (Done), BL-009 (Done), BL-018 |
| Blocks | BL-027, BL-028, BL-029 |
| Annex Spec | `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Topology profile selector + alias dictionary |
| B | Med | M | Dynamic speaker row rendering per topology |
| C | Med | M | Profile library save/recall |
| D | Med | M | Validation diagnostic cards |
| E | Med | M | Host integration + resize regression |

## Objective

Redesign the CALIBRATE panel to support multi-topology monitoring profiles (mono, stereo, quad, surround, binaural, ambisonic, downmix) with a calibration profile library, while preserving deterministic calibration state machine contracts and WebView host reliability. Success: operators can calibrate and store multiple audio configuration profiles from one coherent workflow.

## Scope & Non-Scope

**In scope:**
- Topology profile selector with alias mapping for all supported output configurations
- Dynamic speaker row rendering that adapts to selected topology channel count
- Profile library for saving/recalling calibration results per topology
- Validation diagnostic cards with explicit pass/fail blocks for channel map, polarity, phase
- Host integration and resize behavior regression testing

**Out of scope:**
- New calibration algorithms (reuse existing CalibrationEngine)
- Changes to processBlock calibration signal flow
- Renderer panel changes (that's BL-027)
- Output matrix enforcement (that's BL-028)
- Head tracking integration (that's BL-017)

## Architecture Context

- Calibration state machine: `Source/CalibrationEngine.h:57` — Idle/Playing/Recording/Analyzing/Complete/Error
- Native bridge lifecycle: `Source/PluginEditor.cpp:122` (start), `Source/PluginEditor.cpp:572` (status)
- JS calibration runtime: `Source/ui/public/js/index.js:5058`
- Auto-detected routing: `Source/PluginProcessor.cpp:1835`
- Current topology model limited to 4x Mono / 2x Stereo: `Source/ui/public/index.html:879`
- Renderer diagnostics (requested/active/stage): `Documentation/scene-state-contract.md:145`
- Invariants: Audio Thread (RT safety), Device Compatibility (canonical scene intent), State/Traceability
- ADRs: ADR-0006 (device profiles), ADR-0008 (viewport scope), ADR-0012 (renderer domain exclusivity)

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Topology profile selector + alias dictionary | `Source/PluginProcessor.cpp`, `Source/ui/public/index.html`, `Source/ui/public/js/index.js` | BL-025 stable, BL-018 diagnostics stable | Profile selector renders, alias mapping works |
| B | Dynamic speaker row rendering | `Source/ui/public/index.html`, `Source/ui/public/js/index.js` | Slice A done | Speaker rows adapt to topology channel count |
| C | Profile library save/recall | `Source/PluginProcessor.cpp`, `Source/ui/public/js/index.js` | Slice B done | Profiles save/load per topology |
| D | Validation diagnostic cards | `Source/ui/public/index.html`, `Source/ui/public/js/index.js` | Slice C done | Pass/fail blocks render for channel map, polarity, phase |
| E | Host integration + resize | `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js` | Slice D done | Host resize works, no control clipping |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl BL-026 Slice A: Topology profile selector and alias dictionary
Load: $skill_design, $juce-webview-runtime, $threejs, $skill_docs

Objective: Add a topology profile selector to the CALIBRATE panel that lets operators
choose from: mono, stereo, quadraphonic, 5.1, 7.1, 7.1.2, 7.4.2, binaural, ambisonic_1st,
ambisonic_3rd, and downmix_stereo. Create a shared alias dictionary mapping profile IDs
to human-readable labels and channel counts.

Files to modify:
- Source/PluginProcessor.cpp — add cal_topology_profile APVTS parameter
- Source/ui/public/index.html — add topology selector dropdown in CALIBRATE section (near line 879)
- Source/ui/public/js/index.js — add topology change handler, alias dictionary object

Constraints:
- Preserve existing CalibrationEngine state machine (Idle/Playing/Recording/Analyzing/Complete/Error)
- Topology selection must not trigger calibration — it configures the target only
- Alias dictionary must be shared between native and JS (define in JS, validate in native bridge)
- RT safety: no allocation in processBlock for topology changes
- New parameter must be added to Documentation/implementation-traceability.md

Validation:
- Production self-test passes with topology selector visible
- Topology dropdown renders all profiles
- Selecting a topology updates diagnostics without starting calibration

Evidence:
- TestEvidence/bl026_calibrate_v2_<timestamp>/slice_a_selftest.json
- Update TestEvidence/validation-trend.md
```

### Slice A — Standalone Fallback Prompt

```
You are implementing BL-026 Slice A for LocusQ, a JUCE spatial audio plugin with WebView UI.

PROJECT CONTEXT:
- LocusQ has three modes: EMITTER, RENDERER, CALIBRATE
- CALIBRATE panel currently has fixed 4-speaker (SPK1-SPK4) topology at Source/ui/public/index.html:879
- CalibrationEngine state machine at Source/CalibrationEngine.h:57 manages calibration lifecycle
- Native bridge at Source/PluginEditor.cpp:122 handles start/abort/status
- PluginProcessor.cpp manages APVTS parameters and all native bridge handlers
- Scene-state contract at Documentation/scene-state-contract.md defines diagnostics fields
- Existing spatial profiles in Source/SpatialRenderer.h define available renderer configurations
- RT safety invariant: No heap allocation, locks, or blocking I/O in processBlock()

TASK:
1. Add new APVTS parameter `cal_topology_profile` to Source/PluginProcessor.cpp
   - Type: Choice parameter
   - Values: mono, stereo, quad, surround_51, surround_71, surround_712, surround_742,
     binaural, ambisonic_1st, ambisonic_3rd, downmix_stereo
   - Default: stereo
2. Add topology alias dictionary in Source/ui/public/js/index.js:
   const TOPOLOGY_ALIASES = {
     mono: { label: "Mono", channels: 1 },
     stereo: { label: "Stereo", channels: 2 },
     quad: { label: "Quadraphonic", channels: 4 },
     surround_51: { label: "5.1 Surround", channels: 6 },
     surround_71: { label: "7.1 Surround", channels: 8 },
     surround_712: { label: "7.1.2 Atmos", channels: 10 },
     surround_742: { label: "7.4.2 Immersive", channels: 13 },
     binaural: { label: "Binaural/Headphone", channels: 2 },
     ambisonic_1st: { label: "Ambisonics 1st Order", channels: 4 },
     ambisonic_3rd: { label: "Ambisonics 3rd Order", channels: 16 },
     downmix_stereo: { label: "Downmix Validation", channels: 2 }
   };
3. Add topology selector dropdown in Source/ui/public/index.html in the CALIBRATE section
4. Wire dropdown change event to update APVTS parameter via native bridge
5. Topology selection must NOT trigger calibration start — it only configures the target

CONSTRAINTS:
- Preserve CalibrationEngine state machine — no modifications
- No heap allocation in processBlock for topology changes
- New parameter must follow existing APVTS patterns in PluginProcessor.cpp
- Add new parameter to Documentation/implementation-traceability.md

VALIDATION:
- Build: cmake --build build --target all
- Launch standalone, switch to CALIBRATE mode
- Verify topology dropdown appears and all options are selectable
- Verify selecting topology does not start calibration
- Run production self-test

EVIDENCE:
- TestEvidence/bl026_calibrate_v2_<timestamp>/slice_a_selftest.json
```

### Slice B — Skill-Aware Prompt

```
/impl BL-026 Slice B: Dynamic speaker row rendering per topology
Load: $skill_design, $juce-webview-runtime, $threejs

Objective: Replace fixed SPK1-SPK4 rows with dynamic speaker rows that adapt to the
selected topology's channel count using the alias dictionary from Slice A.

Files to modify:
- Source/ui/public/index.html — replace static speaker rows with dynamic container
- Source/ui/public/js/index.js — add renderSpeakerRows(topologyId) function

Constraints:
- Speaker labels must match topology convention (e.g., L/R for stereo, FL/FR/RL/RR for quad)
- Rows must be scrollable for high channel counts (7.4.2 = 13 channels)
- Preserve existing calibration per-speaker status indicators

Validation:
- Switch between topologies, verify correct number of speaker rows
- Verify labels are topology-appropriate
- Verify scrolling works for 7.4.2 (13 rows)

Evidence:
- TestEvidence/bl026_calibrate_v2_<timestamp>/slice_b_selftest.json
```

### Slice C — Skill-Aware Prompt

```
/impl BL-026 Slice C: Profile library save/recall
Load: $skill_impl, $juce-webview-runtime, $skill_docs

Objective: Implement calibration profile library — save calibration results per
topology+monitoring path combination, recall previously saved profiles.

Files to modify:
- Source/PluginProcessor.cpp — profile save/load handlers in native bridge
- Source/ui/public/js/index.js — profile library UI (save button, profile list, recall)

Constraints:
- Profile storage must use JSON format compatible with existing preset system
- Profile naming: auto-generate from topology + date, allow rename
- Profile recall must restore all calibration fields without re-running calibration
- Must not interfere with host preset save/load (separate storage path)

Validation:
- Save a calibration profile for stereo and quad topologies
- Recall each profile, verify all fields restore correctly
- Verify profiles persist across session restart

Evidence:
- TestEvidence/bl026_calibrate_v2_<timestamp>/slice_c_profile_test.json
```

### Slice D — Skill-Aware Prompt

```
/impl BL-026 Slice D: Validation diagnostic cards
Load: $skill_design, $juce-webview-runtime

Objective: Add explicit pass/fail diagnostic cards for channel map verification,
polarity/phase check, and profile activation status in the CALIBRATE panel.

Files to modify:
- Source/ui/public/index.html — diagnostic card containers
- Source/ui/public/js/index.js — diagnostic rendering logic, pass/fail state management

Constraints:
- Cards must show: channel map (expected vs actual), polarity (in-phase/reversed per channel),
  profile activation (active/inactive with topology match indicator)
- Visual: green check for pass, red X for fail, grey dash for not-yet-tested
- Cards must update in real-time during calibration sequence

Validation:
- Run calibration, verify diagnostic cards update through each state
- Verify pass/fail indicators are correct for known-good calibration
- Verify grey state appears for untested channels

Evidence:
- TestEvidence/bl026_calibrate_v2_<timestamp>/slice_d_diagnostics.json
```

### Slice E — Skill-Aware Prompt

```
/test BL-026 Slice E: Host integration and resize regression
Load: $juce-webview-runtime, $skill_testing

Objective: Validate CALIBRATE v2 changes work in host environments and resize behavior
does not regress from BL-025 baseline.

Files to check:
- Source/PluginEditor.cpp — WebView resize handling
- Source/ui/public/js/index.js — responsive layout

Constraints:
- Test in standalone + REAPER (if available)
- Resize must not clip topology selector or diagnostic cards
- Minimum window size must accommodate smallest topology (mono)

Validation:
- Host resize matrix: test at 800x600, 1200x800, 1920x1080
- Verify no control clipping at any size
- Run production self-test in standalone mode
- Run REAPER headless render smoke if available

Evidence:
- TestEvidence/bl026_calibrate_v2_<timestamp>/slice_e_resize.json
- TestEvidence/bl026_calibrate_v2_<timestamp>/host_smoke.log
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| UI-P1-026A | Automated | Production self-test + topology selector assertions | Topology renders, selection works |
| UI-P1-026B | Automated | Speaker row dynamic rendering assertions | Correct rows per topology |
| UI-P1-026C | Mixed | Profile save/recall + persistence check | Profiles save, recall, persist |
| UI-P1-026D | Automated | Diagnostic card state assertions | Cards update through calibration |
| UI-P1-026E | Mixed | Host resize matrix + REAPER smoke | No clipping, host smoke passes |
| BL-026-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| 4-channel routing ceiling in current PluginProcessor | High | High | Document limitation, plan migration in BL-028 |
| Cross-panel coherence with BL-027 renderer | Med | Med | Shared alias dictionary ensures consistency |
| Profile library conflicts with host preset system | Med | Low | Separate storage path, namespace profiles |
| Resize regression from new UI elements | Med | Med | Slice E specifically guards this |

## Failure & Rollback Paths

- If topology selector breaks calibration state machine: revert Slice A, audit CalibrationEngine.h
- If speaker rows overflow: add CSS scroll container, test at minimum window size
- If profile save fails: check JSON serialization, verify storage path permissions
- If host resize clips: adjust CSS flex/grid layout, add min-width guards
- General rollback: git revert per-slice commits, re-validate BL-025 baseline

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Slice A self-test | `TestEvidence/bl026_calibrate_v2_<timestamp>/slice_a_selftest.json` | timestamp, assertions, pass_count |
| Slice B self-test | `TestEvidence/bl026_calibrate_v2_<timestamp>/slice_b_selftest.json` | topology_tested, row_counts |
| Slice C profile test | `TestEvidence/bl026_calibrate_v2_<timestamp>/slice_c_profile_test.json` | profiles_saved, recall_verified |
| Slice D diagnostics | `TestEvidence/bl026_calibrate_v2_<timestamp>/slice_d_diagnostics.json` | card_states, pass_fail_counts |
| Slice E resize/host | `TestEvidence/bl026_calibrate_v2_<timestamp>/slice_e_resize.json` | resolutions_tested, clip_detected |
| Host smoke log | `TestEvidence/bl026_calibrate_v2_<timestamp>/host_smoke.log` | host, result |
| Validation trend | `TestEvidence/validation-trend.md` | date, lane, result per slice |

## Closeout Checklist

- [ ] Slice A: Topology profile selector renders and selects all profiles
- [ ] Slice B: Dynamic speaker rows adapt to topology channel count
- [ ] Slice C: Profile library saves, recalls, and persists across sessions
- [ ] Slice D: Diagnostic cards update through calibration lifecycle
- [ ] Slice E: Host resize works, no clipping, REAPER smoke passes
- [ ] All UI-P1-026A..E validation lanes pass
- [ ] Evidence captured at designated paths
- [ ] status.json updated
- [ ] Documentation/backlog/index.md row updated
- [ ] TestEvidence surfaces updated
- [ ] implementation-traceability.md updated with new parameters
- [ ] ./scripts/validate-docs-freshness.sh passes
