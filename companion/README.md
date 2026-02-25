---
Title: BL-017 Companion MVP
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-25
---

# BL-017 Companion MVP

This directory contains the BL-017 Slice C companion prototype:

- macOS Swift CLI sender (`locusq-headtrack-companion`)
- UDP pose packet serializer matching the plugin bridge v1 wire contract
- deterministic synthetic pose generation for local smoke validation

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

## Run (Local Smoke)

Default short run:

```bash
cd companion
.build/release/locusq-headtrack-companion --seconds 2 --hz 30
```

Custom destination:

```bash
cd companion
.build/release/locusq-headtrack-companion --host 127.0.0.1 --port 19765 --seconds 5 --hz 60
```

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

## Notes

- This MVP sends deterministic synthetic orientation trajectories as proof-of-contract.
- `CMHeadphoneMotionManager` capture can be layered on top of this sender contract in a follow-up slice without changing packet format.
