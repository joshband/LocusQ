---
Title: LocusQ Head Tracker Companion (BL-017 Slice C MVP)
Document Type: Companion App README
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25
---

# LocusQ Head Tracker Companion (BL-017 Slice C MVP)

This macOS companion app streams headphone orientation from `CMHeadphoneMotionManager` to the LocusQ plugin `HeadTrackingBridge` over UDP.

## Requirements

- macOS 14+
- Xcode 16+ or Swift 6 toolchain
- AirPods Pro/Max (or compatible device exposing headphone motion)
- LocusQ plugin build with BL-017 bridge path enabled

## Build and Run

```bash
cd companion
swift run LocusQHeadTracker
```

Optional destination override:

```bash
swift run LocusQHeadTracker --host 127.0.0.1 --port 19765
```

## Pairing / Setup Flow

1. Connect AirPods to the Mac.
2. Launch the plugin host and load LocusQ.
3. Ensure head-tracking bridge ingress is enabled and listening on UDP `19765`.
4. Start companion app (`swift run LocusQHeadTracker`).
5. Grant motion permissions if prompted.
6. Confirm plugin diagnostics:
   - source = `companion_udp`
   - state = `ok`
   - pose sequence increments

## Packet Contract (v1)

Little-endian payload, 36 bytes total:

- `magic` (`uint32`) = `0x4C515054` (`LQPT`)
- `version` (`uint32`) = `1`
- `qx`, `qy`, `qz`, `qw` (`float32`)
- `timestamp_ms` (`uint64`)
- `seq` (`uint32`)

## Troubleshooting

- `No compatible headphone motion device available`: reconnect supported headphones and verify active output.
- `authorization denied/restricted`: allow motion access in macOS privacy settings.
- plugin remains stale: verify host/port, plugin bridge enable flag, and local firewall rules.
