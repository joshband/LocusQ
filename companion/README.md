---
Title: BL-017 Companion MVP
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-26
---

# BL-017 Companion MVP

This directory contains the BL-017 head-tracking companion:

- macOS Swift executable (`locusq-headtrack-companion`)
- UDP pose packet sender matching the plugin bridge v1 wire contract
- two source modes:
  - `synthetic` deterministic replay signal
  - `live` real `CMHeadphoneMotionManager` motion stream
- optional monitor window (`--ui`) with raw and derived telemetry

Short operator guide:
- `companion/MANUAL.md`

## Packet Contract (v1)

Destination:
- Host: `127.0.0.1` by default
- Port: `19765` by default

Layout (40 bytes total):

| Offset | Type | Field |
|---|---|---|
| 0 | `uint32` | `magic = 0x4C515054` (`LQPT`) |
| 4 | `uint32` | `version = 1` |
| 8 | `float[4]` | quaternion `x,y,z,w` |
| 24 | `uint64` | `timestamp_ms` (epoch ms) |
| 32 | `uint32` | `seq` (monotonic) |
| 36 | `uint32` | reserved (`0`) |

## Build

```bash
cd companion
swift build -c release
```

Binary path:
- `.build/release/locusq-headtrack-companion`

## Run

Deterministic synthetic replay:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode synthetic --seconds 5 --hz 60 --verbose
```

Live AirPods motion stream:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --seconds 0 --hz 60
```

Live stream with monitor window:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --seconds 0 --hz 60 --ui
```

Custom destination:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --host 127.0.0.1 --port 19765 --seconds 30 --hz 60
```

## CLI Options (Summary)

Core options:
- `--mode synthetic|live`
- `--host`, `--port`, `--hz`
- `--plugin-ack-port` (default `19766`, plugin-ingest ack listener)
- `--seconds` (`0` means run until signal)
- `--ui`
- `--verbose`
- `--sched-profile eco|balanced|performance` (default `balanced`)
- `--monitor-hz <5..120>` (default `30`)

Live stabilization options:
- `--no-recenter`
- `--stabilize-alpha <0..1>`
- `--deadband-deg <float>`
- `--velocity-damping <0..1>`

Synthetic options:
- `--yaw-amplitude`
- `--pitch-amplitude`
- `--roll-amplitude`
- `--yaw-frequency`

See full option help:

```bash
cd companion
.build/release/locusq-headtrack-companion --help
```

## Telemetry Monitor Window

When `--ui` is enabled, the companion opens a separate monitor window showing:

- transport: mode, destination, packet sequence, age, send errors
- stream health: effective send rate, interval/jitter, sequence-gap
- plugin ingest: active plugin sources/consumers, endpoint, pose/ack age, invalid/decode counters
- output device identity: name/model, transport type, sample rate, connected state
- raw pose: quaternion + derived yaw/pitch/roll
- motion vectors: rotation rate, gravity, user acceleration
- derived vectors: velocity estimate and displacement estimate
- normalized scores: `motionNorm` and `stabilityNorm`
- stabilization settings currently applied

Important:
- velocity/displacement are **derived estimates** from acceleration and drift over time.
- they are diagnostic helpers, not absolute world-space translation.

## Diagnostic-Grade P0 + P1 (Current)

The companion + bridge now include the following reliability-oriented upgrades:

- Process-shared plugin bridge receiver:
  - one UDP ingest thread/socket per host process
  - multiple LocusQ instances share the same receiver state
  - reduces per-instance overhead and startup contention
- Plugin ingest ack channel (`127.0.0.1:19766` by default):
  - companion receives lightweight plugin-side heartbeat packets
  - exposes plugin consumer count, latest seq, pose age, and invalid packet count
  - enables explicit send-to-ingest confidence in multi-instance sessions
- Scene-state additive field:
  - `rendererHeadTrackingConsumers` is published by plugin diagnostics
  - renderer diagnostics card can surface shared-consumer count
- Output-device visibility:
  - companion polls default output device metadata (name/model/transport/sample-rate/alive)
  - helps confirm real AirPods endpoint vs fallback endpoint
- Stream quality metrics:
  - effective send rate, interval/jitter estimates, and seq-gap
  - provides quick detection of bursty/stale/non-deterministic transport behavior

## Resource and Core Isolation Notes

- Companion process:
  - runs independently from DAW/plugin process, so its CPU work is already isolated at process level.
  - monitor redraw rate is bounded (`--monitor-hz`) and can be tuned for lower overhead.
  - scheduling profile is tunable (`--sched-profile`) to bias responsiveness vs CPU impact.
- Plugin side:
  - head-tracking ingest does not run in `processBlock()`.
  - audio thread only reads atomically published snapshots (no locks/allocations/IO in RT path).
  - bridge ingest thread applies utility QoS hint on macOS.
- Core pinning:
  - hard pinning specific cores from inside a plugin/host is not generally reliable or recommended.
  - practical approach is process-level isolation + low overhead + RT-safe design, which is what this implementation provides.

## Recommended Real Session Launch

Baseline command for multi-instance DAW sessions:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --ui --hz 60 --sched-profile eco --monitor-hz 30
```

If telemetry freshness is not sufficient, increase responsiveness:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --ui --hz 60 --sched-profile performance --monitor-hz 60
```

Operator confidence checks:
- `Plugin Ingest` is `active`
- `Plugin Sources / Consumers` is non-zero while LocusQ instances are open
- `Plugin Ack Age` is low/stable (typically well below 200ms)
- output device matches expected AirPods endpoint

## Head-Tracking Data Model (Measured vs Estimated)

### Measured from CoreMotion (authoritative)

The companion currently streams and/or displays these fields from `CMHeadphoneMotionManager` + `CMDeviceMotion`:

- connection/auth:
  - motion authorization state
  - device motion availability/active state
  - headphone connect/disconnect callbacks
- pose/orientation:
  - quaternion `x,y,z,w`
  - derived yaw/pitch/roll (from quaternion)
  - motion timestamp
- motion:
  - rotation rate (deg/s)
  - gravity vector (g)
  - user acceleration vector (g)
  - heading (when available/valid)
  - sensor location (`default`, `headphone_left`, `headphone_right`)
- transport/health:
  - packet sequence
  - send errors / invalid samples
  - packet age

### Estimated in Companion (diagnostic, not absolute)

The companion computes these values from measured inputs:

- smoothed quaternion and smoothed vectors
- velocity estimate (integrated from user acceleration + damping)
- displacement estimate (integrated velocity + clamp)
- `motionNorm` and `stabilityNorm`

These are useful for stability/troubleshooting, but are not true absolute world position.

## AirPods Identity and Additional API Coverage

### What we can add reliably next

- explicit headphone status card:
  - connected / disconnected / unavailable / permission denied
  - stream active / stale
- richer activity status (newer OS targets):
  - `CMHeadphoneActivityManager` (macOS 15+)
  - connection status + motion activity classification/confidence

### What is limited or not guaranteed from public motion APIs

- exact consumer model identity (for example "AirPods Pro 2") is not guaranteed by CoreMotion motion APIs.
- per-bud/case battery and ANC/transparency mode are not part of the current CoreMotion motion contract.
- absolute 6DoF world translation is not provided by headphone motion API.

### Practical model display strategy

To display a user-friendly device label in the companion:

- primary: currently selected output device name from CoreAudio/AVAudioSession equivalent path.
- optional enrichment: Bluetooth metadata where available.
- fallback: `"Compatible Headphones (motion)"` when model-level metadata is unavailable.

## Recommended Companion UI/Visualization Additions

Priority order for product value:

1. Connection + identity strip
   - connection state
   - output device name/model label
   - stream active/stale badge
2. Stream quality HUD
   - target Hz vs effective Hz
   - jitter, sequence gaps/drop rate
   - send-to-plugin consume lag (if plugin ack channel is enabled)
3. Dual-frame orientation visualization
   - world axes + head-local axes simultaneously
   - forward cone + short orientation trail
4. Confidence overlays
   - low-confidence state when jitter/spikes exceed threshold
   - uncertainty tint on vectors/pose glyph
5. Activity card (OS-gated)
   - headphone activity classification + confidence (when API available)

## Extrapolated Metrics We Can Add Safely

Additive derived metrics that can improve diagnostics:

- angular acceleration (from rotation-rate delta)
- jerk (from acceleration delta)
- drift score (integration error tendency over window)
- settle time after recenter
- tremor index (high-frequency motion energy)
- pose-to-render latency and stream-to-plugin latency (when ack path exists)

All derived metrics should be explicitly labeled `estimated` and clamped/sanitized for UI stability.

## Reliability Notes (Slice D)

- Bounded transient UDP send retries are enabled to prevent unbounded resend loops under temporary socket pressure.
- Deterministic signal handling is enabled for `SIGINT`/`SIGTERM`.
  - `Ctrl+C` triggers graceful shutdown with explicit `companion_shutdown` and final `companion_done` logs.
  - Companion exits without hanging in send/sleep loops once stop is requested.
- Existing CLI flags are backward-compatible; no new required arguments.

## Soak Run Example

Use a longer soak pass for bridge/runtime reliability checks:

```bash
cd companion
.build/release/locusq-headtrack-companion --seconds 600 --hz 30 --host 127.0.0.1 --port 19765
```

During soak execution, send `SIGINT` (`Ctrl+C`) to verify deterministic shutdown behavior.

## Promotion Replay Reference (Slice E)

Promotion packet replay command used for BL-017 Slice E:

```bash
./companion/.build/arm64-apple-macosx/release/locusq-headtrack-companion --seconds 30 --hz 30 --host 127.0.0.1 --port 19765
```

## Pairing Steps With LocusQ

1. Launch LocusQ standalone or host session where BL-017 Slice A/B bridge path is enabled.
2. Confirm plugin bridge is listening on UDP port `19765` (or set matching port if changed).
3. Start the companion sender from this directory.
4. Verify plugin-side head-tracking diagnostics advance sequence and update age/state.
5. (Optional) Enable `--ui` and compare companion telemetry against the LocusQ renderer diagnostics card.

## Notes

- This MVP sends deterministic synthetic orientation trajectories as proof-of-contract.
- Live `CMHeadphoneMotionManager` mode keeps packet format unchanged (quaternion + timestamp + seq).
