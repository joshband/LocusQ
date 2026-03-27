Title: Companion Room Calibration Execution Packet
Document Type: Plan
Author: APC Codex
Created Date: 2026-03-26
Last Modified Date: 2026-03-26

# Companion Room Calibration Execution Packet

## Purpose

Turn the room-estimation idea into one concrete LocusQ companion architecture and one canonical backlog lane.

This packet is planning authority only.
It does not claim that room calibration is implemented or validated.

Validation status: `not tested`

## Skills Used

Phase order:
- `plan`

Specialist bundle:
- `apple-spatial-companion-platform`
- `headtracking-companion-runtime`
- `spatial-audio-engineering`
- `skill_docs`

## Source Inputs

- `status.json`
- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/plan.md`
- `Documentation/backlog/index.md`
- `Documentation/backlog/runbook-authoring-guide.md`
- `Documentation/backlog/done/bl-058-companion-profile-acquisition.md`
- `Documentation/backlog/done/bl-096-companion-executable-core-protocol-reunification.md`
- `Documentation/backlog/done/bl-101-calibrate-discovery-provenance-and-truthfulness.md`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`
- `companion/Sources/LocusQHeadTrackerCore/MotionService.swift`

Platform references:
- Apple RoomPlan overview: `https://developer.apple.com/augmented-reality/roomplan/`
- Apple RoomPlan multi-room scan docs: `https://developer.apple.com/documentation/roomplan/scanning-the-rooms-of-a-single-structure`
- Apple audio routing QA1799: `https://developer.apple.com/library/archive/qa/qa1799/_index.html`
- Apple audio session guide: `https://developer.apple.com/library/archive/documentation/Audio/Conceptual/AudioSessionProgrammingGuide/AudioSessionBasics/AudioSessionBasics.html`
- Apple Continuity Camera session: `https://developer.apple.com/videos/play/wwdc2022/10018/`
- Apple WWDC25 recording updates: `https://developer.apple.com/videos/play/wwdc2025/251/`

## Current Reality

What already exists:
- the Mac companion already owns Apple-specific runtime work for head tracking
- live mode already uses `CMHeadphoneMotionManager`
- the companion already has a calibration-oriented operator shell and profile acquisition posture
- the plugin already has CALIBRATE-facing provenance and truthfulness surfaces

What does not exist yet:
- no room calibration or room estimation mode in the companion
- no acoustic probe engine in the Mac or iPhone path
- no geometry capture path for room structure
- no compact room-profile contract between companion and plugin
- no truthful proof lane for room-estimation claims

Important constraint:
- the existing AirPods companion path provides orientation, not room-space position
- walking to multiple points is useful, but only if we pair it with explicit waypoint confirmation or a true position source such as iPhone ARKit/RoomPlan

## Product Contract

The feature should preserve these rules:

1. Room calibration is companion-owned.
2. The plugin consumes only a compact room profile, not raw audio or raw camera scans.
3. Mac-only capture may provide a coarse estimate, not authoritative room geometry.
4. iPhone Pro capture is the preferred full-fidelity path for room geometry and ear-near acoustic capture.
5. AirPods microphone capture is optional and experimental until proven truthful and stable.
6. AirPods head tracking may assist orientation tagging and capture guidance, but not automatic room-position truth claims.
7. Privacy defaults stay local-first: no cloud upload, no silent persistence of raw audio, no silent persistence of room scans.

## Operator Modes

| Mode | Primary device | Primary sensors | Goal | Truth posture |
|---|---|---|---|---|
| Quick Scan | Mac companion | Mac speaker, Mac mic, Mac camera | coarse room-size and liveness estimate | approximate only |
| Guided Multi-Point Scan | Mac companion + AirPods | Mac speaker, Mac mic, Mac camera, AirPods orientation | stronger estimate from multiple named points | approximate room profile, no geometry truth |
| Full Scan | iPhone 16 Pro Max + Mac companion | iPhone LiDAR, iPhone camera, iPhone mics, optional AirPods orientation | room geometry + acoustic profile | authoritative v1 path |
| Experimental AirPods Capture | Mac or iPhone companion | AirPods mic route if available | compare ear-route capture against baseline | advisory only |

## User Flow

### Quick Scan

1. Open the companion and enter `Room Calibration`.
2. Select `Quick Scan`.
3. The Mac companion checks output route, input route, room noise floor, and camera visibility.
4. The companion emits a short probe sweep and records the response.
5. The companion returns a coarse room profile:
   - size class
   - liveness
   - brightness/damping
   - confidence

### Guided Multi-Point Scan

1. Open `Room Calibration` and pair AirPods if head orientation tagging is desired.
2. The companion asks the user to stand at named points such as `desk`, `left wall`, `rear center`, and `listening spot`.
3. The Mac camera verifies framing and the companion records the selected waypoint.
4. The companion emits probe sweeps and records responses at each waypoint.
5. The result adds asymmetry and seat-to-seat variation metrics.

### Full Scan

1. The Mac companion launches or pairs with an iPhone capture session.
2. The iPhone 16 Pro Max uses RoomPlan to capture room structure.
3. The iPhone runs guided acoustic captures at one or more listening positions.
4. The iPhone sends a compact geometry + acoustic packet back to the Mac companion.
5. The Mac companion stores the resulting room profile and exposes a truthful confidence summary to LocusQ.

## Architecture

### Component Map

| Component | Lives in | Responsibility |
|---|---|---|
| `RoomCalibrationController` | companion executable | owns mode selection, sequencing, and operator state |
| `ProbeSignalEngine` | companion core | generates sweep/MLS probes and playback timing |
| `CaptureRouteManager` | companion core | chooses Mac/iPhone/AirPods input-output route and records route provenance |
| `VisionGuidanceController` | Mac companion | uses the Mac camera for framing, waypoint confirmation, and orientation guidance |
| `RoomGeometryProvider` | iPhone companion | runs RoomPlan and exports compact structural geometry |
| `RoomProfileEstimator` | shared companion core or paired modules | deconvolves capture data and derives room metrics |
| `CalibrationProfileStore` | companion + plugin bridge boundary | persists compact room profile and provenance |
| `RoomCalibrationBridge` | companion-to-plugin bridge | exposes room profile readiness, confidence, and profile selection |

### Data Flow

1. Operator selects calibration mode.
2. Capture route is locked and logged.
3. Probe signal is emitted and timestamped.
4. Recorded response is windowed and deconvolved.
5. Geometry, if available, is fused with acoustic estimates.
6. A compact room profile is derived.
7. The companion writes only the compact profile plus provenance into the CALIBRATE-facing path.

## Apple API Boundary

| Capability | Preferred API / platform | Why |
|---|---|---|
| AirPods head orientation | `CMHeadphoneMotionManager` on Mac companion | already matches the current LocusQ live-mode boundary |
| Mac quick-scan playback/recording | `AVAudioEngine` / `AVFoundation` on macOS | deterministic probe playback and capture without pushing work into plugin DSP |
| Mac camera guidance | `AVFoundation` + `Vision` on macOS | enough for framing and waypoint UX, not enough for authoritative room geometry |
| Full room geometry | `RoomPlan` + `ARKit` on iPhone Pro / iPad Pro | Apple-supported room-structure path |
| High-quality Bluetooth recording | supported Apple routing options where exposed | optional improvement, not the core contract |
| Mac <-> iPhone session handoff | local peer transport | implementation detail; keep sync local-only and explicit |

### Explicit Non-Goals

- no plugin-side room probing in `processBlock()`
- no claim of exact metric room dimensions from Mac-only camera capture
- no dependency on undocumented AirPods microphone control
- no assumption that AirPods input and laptop output can be held in a stable, calibration-grade route on every setup

## Signal-Processing Contract

The room profile should be compact and truthful.
It should prefer stable features over visually impressive but weakly supported claims.

### Probe Strategy

- use short swept-sine or MLS bursts
- capture a bounded response window
- repeat if noise-floor or clipping checks fail
- log route, device, gain, and ambient-noise metadata

### Derived Metrics

| Metric | Meaning | Target use |
|---|---|---|
| `room_size_class` | `small`, `medium`, `large`, or `unknown` | broad tuning and UI messaging |
| `first_reflection_ms` | strongest early reflection delay proxy | early-boundary feel |
| `decay_mid_proxy` | midband decay / liveness proxy | wetness and damping choices |
| `spectral_damping` | high-vs-low decay balance | brightness / softness compensation |
| `asymmetry_score` | left-right / front-back inconsistency | warning and guidance |
| `capture_confidence` | measurement confidence | truth-language gate |
| `geometry_confidence` | RoomPlan or structural confidence | geometry-based feature gating |

### Room Profile Contract

The plugin should receive:
- compact scalar metrics
- capture-device provenance
- geometry availability flags
- confidence and fallback reason

The plugin should not receive:
- raw impulse responses
- raw microphone recordings
- full camera frames
- raw RoomPlan scene dumps unless explicitly exported by the user

## Proposed UX Language

- `Quick Scan`: "Fast estimate from this Mac. Good for a first pass."
- `Full Scan`: "Uses your iPhone Pro for room structure and stronger calibration."
- `AirPods Assist`: "Uses head orientation and, when available, ear-route recording. Experimental."

Truth language:
- `Estimated room profile`
- `Geometry-assisted estimate`
- `Experimental ear-route capture`
- never `exact room dimensions` unless the iPhone RoomPlan lane is actually present and the UI distinguishes geometry from acoustic inference

## Backlog Mapping

| ID | Scope | Depends On | Blocks |
|---|---|---|---|
| BL-121 | companion room calibration, device-assisted room estimation, route/provenance contract, and proof plan | BL-058, BL-096, BL-101 | future room-aware calibration/rendering follow-ons |

## Implementation Slices

| Slice | Goal | Primary write set | Exit signal |
|---|---|---|---|
| A | Freeze product boundary and room-profile contract | companion docs, CALIBRATE contract docs, bridge schema | one truthful room-profile contract exists |
| B | Mac `Quick Scan` path | `companion/Sources/...` | Mac-only coarse scan runs end to end |
| C | Guided multi-point Mac flow with camera guidance and AirPods orientation tags | companion UI/runtime + camera guidance code | named waypoint capture flow works |
| D | iPhone Pro full scan path with RoomPlan and acoustic fusion | new iPhone-side capture module/app + sync boundary | geometry-assisted scan works end to end |
| E | Plugin ingest, profile storage, and truthful UI surfaces | plugin bridge/state/UI surfaces | profile selection and confidence render truthfully |
| F | Validation and evidence | scripts, runbooks, `TestEvidence/bl121_*` | deterministic docs/runtime/manual proof exists |

## Validation Strategy

| Layer | Minimum planned proof |
|---|---|
| Docs + backlog authority | `./scripts/export-backlog-summaries.py --check`, `./scripts/validate-backlog-plain-language.sh`, `./scripts/validate-backlog-redundancy.py`, `./scripts/validate-docs-freshness.sh` |
| Companion core | `cd companion && swift test` plus targeted route/probe unit coverage |
| Mac quick scan | repeated local captures with stable route provenance and bounded variance |
| Guided multi-point flow | manual waypoint walkthrough with camera-guidance evidence |
| iPhone full scan | RoomPlan structural capture + acoustic replay packet |
| Plugin ingest | profile round-trip and truthful fallback rendering |

## Risks

| Risk | Why it matters | Mitigation |
|---|---|---|
| AirPods input route is unstable or strongly processed | it could produce impressive but misleading room estimates | keep AirPods capture advisory-only until objective proof exists |
| Mac camera overclaims geometry fidelity | a single webcam is not RoomPlan | use Mac camera only for guidance and waypoint checks |
| iPhone full scan adds cross-device complexity | pairing and local sync can become brittle | keep Mac quick scan as an independent first slice |
| Raw recordings or scans persist unexpectedly | privacy trust would drop fast | store only compact profile output by default |
| Orientation is confused for position | walking flow could overclaim mapping power | require manual waypoints or iPhone AR geometry for spatial claims |

## Exit Signal

This packet is complete when:

1. BL-121 exists as a canonical open backlog item.
2. `Documentation/backlog/index.md` reflects the new lane.
3. The architecture names one truthful Mac path and one truthful iPhone Pro path.
4. AirPods capture is explicitly marked optional and experimental.
5. No implementation or validation claims are made without later evidence.
