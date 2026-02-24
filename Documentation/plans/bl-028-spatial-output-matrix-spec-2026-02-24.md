Title: BL-028 Spatial Output Matrix v1 Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# BL-028 Spatial Output Matrix v1 Spec

## Purpose
Define one authoritative spatial-output contract for LocusQ across headphone binaural, head-tracked monitoring, multichannel speaker/AVR rendering, and external spatial pipelines, while preventing double-spatialization and hidden fallback behavior.

## Backlog Link
- Proposed Backlog ID: `BL-028`
- Canonical backlog file: `Documentation/backlog-post-v1-agentic-sprints.md`

## Companion Contracts
- `Documentation/plans/bl-017-head-tracked-monitoring-companion-bridge-plan-2026-02-22.md`
- `Documentation/plans/bl-026-calibrate-uiux-v2-spec-2026-02-23.md`
- `Documentation/plans/bl-027-renderer-uiux-v2-spec-2026-02-23.md`
- `Documentation/scene-state-contract.md`
- `Documentation/spatial-audio-profiles-usage.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Problem Statement
Current contracts expose spatial profile, headphone mode/profile, and diagnostics, but output-domain authority is fragmented across CALIBRATE and RENDERER surfaces. Operators can still encounter ambiguous combinations (for example, headphone-binaural intent with multichannel speaker routing), and the system lacks one explicit matrix that blocks invalid combinations before runtime ambiguity appears.

## Normative Boundaries
1. Plugin-level Apple Personalized Spatial Audio internals are out of scope for DAW plugin runtime control.
2. Head tracking in plugin scope is bridge-based (`BL-017` companion path), not direct OS personalization control.
3. Device profile selection is not renderer-domain authority; it is compensation metadata layered onto the selected renderer domain.
4. Internal binaural and multichannel speaker rendering are mutually exclusive runtime domains.

## Renderer Domain Authority Model

```cpp
enum class RendererDomain
{
    InternalBinaural, // plugin performs binauralization
    Multichannel,     // plugin outputs speaker channels directly
    ExternalSpatial   // plugin outputs bed for external/OS spatialization
};
```

Rules:
1. Exactly one `RendererDomain` is active.
2. Domain selection is explicit and operator-visible.
3. Invalid domain + bus combinations are blocked, not silently auto-corrected.

## Spatial Output Matrix

| ID | Renderer Domain | Bus Layout | Head Tracking Source | Binaural Stage | Target | Contract |
|---|---|---|---|---|---|---|
| SOM-01 | `InternalBinaural` | Stereo (2ch) | `none` | enabled | Generic/Sony headphone monitoring | allowed |
| SOM-02 | `InternalBinaural` | Stereo (2ch) | `bridge_udp` | enabled | AirPods DAW monitoring with bridge pose | allowed |
| SOM-03 | `InternalBinaural` | Multichannel (>2ch) | any | enabled | Speaker/AVR path | blocked (double-spatialization risk) |
| SOM-04 | `Multichannel` | 5.1 / 7.1 / 7.4.2 | `none` | disabled | Denon AVR / discrete speaker systems | allowed |
| SOM-05 | `Multichannel` | Stereo (2ch) | any | disabled | Collapsed speaker topology | blocked |
| SOM-06 | `ExternalSpatial` | Multichannel bed | `os_managed` | disabled | Apple standalone or external renderer path | allowed |
| SOM-07 | `ExternalSpatial` | Stereo (2ch) | `os_managed` | disabled | External spatial intent with collapsed bus | blocked |

## Device Profile Contract
Supported profile IDs:
- `generic`
- `airpods_pro_2`
- `sony_wh1000xm5`
- `custom_sofa`

Rules:
1. Device profile does not auto-switch renderer domain.
2. Profile selection updates compensation path and diagnostics labels only.
3. Unsupported profile resources (for example SOFA asset missing) must publish explicit fallback state.

## Head Tracking Contract
Head-tracking state model (bridge-oriented):

```cpp
enum class HeadTrackingSource { none, bridge_udp, bridge_ws, os_managed };

struct HeadTrackingState
{
    bool enabled;
    HeadTrackingSource source;
    float yawDeg;
    float pitchDeg;
    float rollDeg;
    int ageMs;
    bool stale;
};
```

Rules:
1. Head tracking applies only in `InternalBinaural` domain for v1.
2. Stale timeout disables pose influence deterministically while keeping audio active.
3. Audio thread consumes lock-free snapshot only (no socket/IPC work on audio thread).

## Bus and Topology Authority
1. Runtime detects negotiated host bus layout each block-cycle boundary.
2. `Multichannel` domain requires topology-compatible bus layout or explicit block state.
3. `InternalBinaural` domain requires stereo output and headphone path diagnostics visibility.
4. `ExternalSpatial` domain requires multichannel bed and explicit inactive internal binaural stage.

## CALIBRATE and RENDERER UX Contracts
1. Both panels must show `requested -> active -> stage` for spatial domain, profile, and monitoring path.
2. CALIBRATE must validate matrix legality before start (`UI-P1-026C` preflight extension).
3. RENDERER must expose domain authority cluster and stage chips (`BL-027` slice alignment).
4. Any blocked matrix combination must show a clear reason and remediation text.

## Scene-State Telemetry Contract Additions (Planned)
Planned additions to snapshot payload:
- `rendererDomainRequested`
- `rendererDomainActive`
- `rendererDomainStage`
- `rendererMatrixRuleId`
- `rendererMatrixRuleState` (`allowed`, `blocked`)
- `rendererMatrixRuleReason`

BL-017 bridge fields remain authoritative for pose telemetry:
- `rendererHeadTrackingEnabled`
- `rendererHeadTrackingSource`
- `rendererHeadTrackingSeq`
- `rendererHeadTrackingAgeMs`
- `rendererHeadTrackingState`
- `rendererHeadTrackingYawDeg`
- `rendererHeadTrackingPitchDeg`
- `rendererHeadTrackingRollDeg`

## Validation Strategy

### Objective Lanes
1. `./scripts/qa-bl009-headphone-contract-mac.sh`
2. `./scripts/qa-bl009-headphone-profile-contract-mac.sh`
3. `./scripts/qa-bl018-ambisonic-contract-mac.sh`
4. `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
5. `./scripts/standalone-ui-selftest-production-p0-mac.sh`

### Planned Matrix-Specific Lanes
1. `UI-P2-028A`: matrix legality checks for blocked combinations.
2. `UI-P2-028B`: requested/active/stage chip parity across CALIBRATE and RENDERER.
3. `UI-P2-028C`: bridge stale-timeout behavior in internal binaural domain.
4. `UI-P2-028D`: multichannel speaker routing integrity in AVR-target topology.
5. `UI-P2-028E`: external spatial domain enforcement with internal binaural disabled.

### Manual Lanes
1. AirPods Pro 2 DAW monitoring:
- Validate stereo endpoint, domain legality, and bridge-state visibility.
2. Sony WH-1000XM5 DAW monitoring:
- Validate stereo/binaural endpoint behavior without claiming OS-managed personalization.
3. Denon AVR multichannel monitoring:
- Validate channel-ID sweep and speaker-map integrity with internal binaural disabled.

## Implementation Plan
1. Slice A: Contract freeze and shared alias dictionary in CALIBRATE/RENDERER.
2. Slice B: Renderer domain selector and matrix validator surface wiring.
3. Slice C: Scene-state publication of domain + matrix rule diagnostics.
4. Slice D: BL-017 bridge integration to matrix gating (`InternalBinaural` only).
5. Slice E: Self-test lane extensions and evidence closeout synchronization.

## Exit Criteria
1. All matrix rules are enforced with deterministic allow/block behavior.
2. CALIBRATE and RENDERER show consistent requested/active/stage semantics.
3. No hidden fallback remains for invalid domain + layout combinations.
4. Matrix lanes pass with fresh evidence in `TestEvidence`.
5. Backlog/status/evidence docs are synchronized in one closeout change set.

## Non-Goals
1. Dolby bitstream encoding implementation in plugin.
2. Extraction or direct control of private Apple personalized HRTF data.
3. Full codec export/import pipeline changes (ADM/IAMF remain separate scope unless explicitly promoted).

