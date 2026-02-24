---
Title: HX-05 Payload Budget and Throttle Contract
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# HX-05: Payload Budget and Throttle Contract

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Open |
| Owner Track | Track F — Hardening |
| Depends On | BL-016 (Done), BL-025 (Done) |
| Blocks | — |
| Annex Spec | (inline — references scene-state-contract and transport docs) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Low | S | Measure current payload sizes |
| B | Med | M | Define budget thresholds |
| C | Med | L | Implement throttle/drop policy |
| D | Med | M | Stress validation |

## Objective

Define and enforce a scene payload budget and throttling contract to maintain UI responsiveness under high emitter counts. Prevent unbounded payload growth from degrading WebView rendering performance.

## Scope & Non-Scope

**In scope:**
- Measuring current scene snapshot payload sizes (bytes per emitter, total per snapshot)
- Defining budget thresholds (max bytes per snapshot, max emitters before throttling)
- Implementing throttle/drop policy when budget is exceeded
- Stress testing with 8+ simultaneous emitters

**Out of scope:**
- Redesigning the scene snapshot format
- Changing the SceneGraph lock-free architecture
- WebView rendering optimization (that's independent)

## Architecture Context

- Scene snapshots: published from processBlock via SceneGraph double-buffer, consumed by UI timer
- Snapshot payload: JSON serialized in PluginEditor.cpp native bridge, sent to WebView JS
- Current payload fields: per-emitter position, direction, energy, physics state, overlays
- Scene-state contract: `Documentation/scene-state-contract.md` defines all payload fields
- Transport contract (BL-016): cadence and sequence safety guarantees
- Invariants: Scene Graph (lock-free, finite fields), Audio Thread (RT safety)

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Measure payload sizes | `Source/PluginEditor.cpp`, `Source/SceneGraph.h` | BL-016, BL-025 done | Payload size report per emitter count |
| B | Define budget thresholds | `Documentation/scene-state-contract.md` | Slice A data | Thresholds documented |
| C | Implement throttle/drop | `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js` | Slice B done | Throttle active when budget exceeded |
| D | Stress validation | `tests/`, `TestEvidence/` | Slice C done | 8+ emitters stable |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl HX-05 Slice A: Measure current scene payload sizes
Load: $juce-webview-runtime, $skill_testing

Objective: Instrument the scene snapshot bridge path to measure payload sizes
(bytes per snapshot) at various emitter counts (1, 2, 4, 8, 16).

Files to modify:
- Source/PluginEditor.cpp — add temporary payload size logging in bridge send path

Constraints:
- Logging must be on message thread only (no logging in processBlock)
- Measure JSON serialized size before sending to WebView
- Capture: emitter_count, payload_bytes, fields_per_emitter

Validation:
- Launch standalone, add emitters, record payload sizes
- Compile report: payload_bytes = f(emitter_count)

Evidence:
- TestEvidence/hx05_payload_budget_<timestamp>/payload_measurements.tsv
```

### Slice A — Standalone Fallback Prompt

```
You are implementing HX-05 Slice A for LocusQ.

PROJECT CONTEXT:
- Scene snapshots are serialized as JSON in Source/PluginEditor.cpp native bridge
- SceneGraph (Source/SceneGraph.h) has EmitterSlot array with per-emitter fields
- Snapshot payload includes: position (x,y,z), direction (az,el), energy (rms),
  physics state, overlay flags, plus room/listener/speaker telemetry

TASK:
1. In Source/PluginEditor.cpp, find the bridge function that sends scene state to WebView
2. Add temporary size measurement: log JSON string length before sending
3. Test with 1, 2, 4, 8 emitters active
4. Record payload sizes in TSV format: emitter_count | payload_bytes | timestamp
5. Calculate per-emitter overhead and base overhead
6. Remove temporary logging after measurement
7. Write report

EVIDENCE:
- TestEvidence/hx05_payload_budget_<timestamp>/payload_measurements.tsv
```

### Slice B — Skill-Aware Prompt

```
/plan HX-05 Slice B: Define payload budget thresholds
Load: $skill_docs, $juce-webview-runtime

Objective: Based on Slice A measurements, define budget thresholds in scene-state-contract.md.

Decisions needed:
- Max payload per snapshot (suggest: 32KB soft limit, 64KB hard limit)
- Max emitters before throttling (based on per-emitter size from Slice A)
- Throttle behavior: reduce update frequency vs drop optional fields vs both
- Warning threshold (% of budget that triggers diagnostics)

Evidence:
- Updated Documentation/scene-state-contract.md with budget section
```

### Slice C — Skill-Aware Prompt

```
/impl HX-05 Slice C: Implement throttle/drop policy
Load: $skill_impl, $juce-webview-runtime

Objective: When scene snapshot exceeds soft budget, apply throttle policy.

Behavior:
- Below soft limit: full payload at normal cadence
- Between soft and hard limit: reduce cadence (skip every other snapshot)
- Above hard limit: drop optional fields (overlays, physics detail) first, then throttle

Files to modify:
- Source/PluginEditor.cpp — budget check before bridge send
- Source/ui/public/js/index.js — handle reduced cadence gracefully (interpolate)

Constraints:
- Budget check must be on message thread (not processBlock)
- Throttle must not cause visual tearing
- Must publish throttle state in diagnostics

Evidence:
- TestEvidence/hx05_payload_budget_<timestamp>/throttle_implementation.log
```

### Slice D — Skill-Aware Prompt

```
/test HX-05 Slice D: High-emitter stress validation
Load: $skill_test, $skill_testing

Objective: Validate UI remains responsive with 8+ simultaneous emitters under throttle policy.

Test scenarios:
1. 8 emitters, full motion: verify smooth UI, payload within budget
2. 16 emitters: verify throttle activates, UI remains responsive
3. Rapid emitter add/remove (1->16->1): verify no crash or stale state
4. Throttle diagnostics visible in UI

Evidence:
- TestEvidence/hx05_payload_budget_<timestamp>/stress_results.tsv
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| HX-05-measure | Manual | Payload size measurement | Report generated |
| HX-05-throttle | Automated | High-emitter stress | UI responsive at 8+ emitters |
| HX-05-diagnostics | Automated | Throttle state check | Diagnostics publish correctly |
| HX-05-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Throttle causes visual stuttering | Med | Med | Interpolation on JS side smooths gaps |
| Budget thresholds too aggressive | Med | Med | Start conservative, tune based on testing |
| Payload measurement overhead | Low | Low | Temporary instrumentation, remove after |

## Failure & Rollback Paths

- If throttle causes worse UX than no throttle: disable throttle, increase soft limit
- If stress test crashes: check SceneGraph slot bounds, verify emitter registration
- If payload grows unexpectedly: audit new fields added since last measurement

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Payload measurements | `TestEvidence/hx05_payload_budget_<timestamp>/payload_measurements.tsv` | emitter_count, payload_bytes |
| Stress results | `TestEvidence/hx05_payload_budget_<timestamp>/stress_results.tsv` | scenario, fps, payload_bytes, throttle_active |
| Status TSV | `TestEvidence/hx05_payload_budget_<timestamp>/status.tsv` | lane, result, timestamp |

## Closeout Checklist

- [ ] Payload sizes measured and documented
- [ ] Budget thresholds defined in scene-state-contract.md
- [ ] Throttle policy implemented and tested
- [ ] 8+ emitter stress test passes
- [ ] Evidence captured at designated paths
- [ ] status.json updated
- [ ] Documentation/backlog/index.md row updated
- [ ] TestEvidence surfaces updated
- [ ] ./scripts/validate-docs-freshness.sh passes
