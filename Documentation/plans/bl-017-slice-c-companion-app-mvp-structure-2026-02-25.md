---
Title: BL-017 Slice C Companion App MVP Structure
Document Type: Design Note
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25
---

# BL-017 Slice C Companion App MVP Structure

## 1. Scope and Constraints

- Target: macOS-only companion app (separate from plugin project).
- Sensor source: `CMHeadphoneMotionManager` (AirPods/compatible headphones required).
- Transport: UDP pose packets to `HeadTrackingBridge` on `127.0.0.1:19765`.
- BL-017 state: Slices A and B are merged and validated; this note defines Slice C MVP structure.

## 2. Proposed Directory Tree

Two viable MVP options are included. Start with **Option A (Swift CLI)** for fastest bring-up and deterministic local testing.

### Option A — Swift CLI (recommended MVP)

```text
companion/
  Package.swift
  README.md
  Sources/
    LocusQHeadTracker/
      main.swift
      MotionManager.swift
      UdpSender.swift
      PosePacket.swift
      Logging.swift
  Tests/
    LocusQHeadTrackerTests/
      PosePacketTests.swift
```

Why this option first:
- Minimal lifecycle complexity.
- Fast local iteration (`swift run`).
- Simple to run headless during validation captures.

### Option B — SwiftUI menu-bar minimal app

```text
companion-app/
  LocusQCompanion.xcodeproj
  LocusQCompanion/
    App/
      LocusQCompanionApp.swift
      AppState.swift
    Motion/
      MotionService.swift
    Network/
      UdpSender.swift
      PosePacket.swift
    UI/
      StatusMenuView.swift
    README.md
  LocusQCompanionTests/
    PosePacketTests.swift
```

Use this only if MVP needs user-visible status/start-stop controls in Slice C.

## 3. CMHeadphoneMotionManager Setup Snippet

```swift
import CoreMotion
import Foundation

final class MotionManager {
    private let motionManager = CMHeadphoneMotionManager()
    private let queue = OperationQueue()

    var onQuaternion: ((_ qx: Float, _ qy: Float, _ qz: Float, _ qw: Float, _ timestampMs: UInt64) -> Void)?

    func start() {
        guard CMHeadphoneMotionManager.authorizationStatus() == .authorized ||
              CMHeadphoneMotionManager.authorizationStatus() == .notDetermined else {
            print("Headphone motion access denied/restricted")
            return
        }

        guard motionManager.isDeviceMotionAvailable else {
            print("No compatible headphone motion device available")
            return
        }

        // First call prompts permission when status is .notDetermined.
        motionManager.startDeviceMotionUpdates(to: queue) { [weak self] motion, error in
            if let error {
                print("Device motion error: \(error)")
                return
            }
            guard let attitude = motion?.attitude else { return }

            let q = attitude.quaternion // x/y/z/w are Double
            let nowMs = UInt64(Date().timeIntervalSince1970 * 1000.0)
            self?.onQuaternion?(Float(q.x), Float(q.y), Float(q.z), Float(q.w), nowMs)
        }
    }

    func stop() {
        motionManager.stopDeviceMotionUpdates()
    }
}
```

Notes:
- Keep callback work minimal; forward to packetizer/sender immediately.
- If strict monotonic time is required later, switch `timestampMs` to monotonic clock conversion.

## 4. UDP Sender Snippet (Network.framework)

```swift
import Foundation
import Network

final class UdpSender {
    private let connection: NWConnection

    init(host: String = "127.0.0.1", port: UInt16 = 19765) {
        connection = NWConnection(host: NWEndpoint.Host(host),
                                  port: NWEndpoint.Port(rawValue: port)!,
                                  using: .udp)
        connection.start(queue: .global(qos: .userInitiated))
    }

    func send(_ payload: Data) {
        connection.send(content: payload, completion: .contentProcessed { error in
            if let error {
                print("UDP send error: \(error)")
            }
        })
    }

    func stop() {
        connection.cancel()
    }
}
```

Fallback alternative: POSIX `sendto` is acceptable for ultra-minimal environments, but Network.framework keeps API ownership cleaner for macOS companion code.

## 5. Packet Serialization Contract (BL-017 v1)

Expected packet format (little-endian), matching plugin `HeadTrackingBridge` contract:

| Offset | Type | Field |
|---:|---|---|
| 0 | `uint32` | `magic = 0x4C515054` (`"LQPT"`) |
| 4 | `uint32` | `version = 1` |
| 8 | `float32` | `qx` |
| 12 | `float32` | `qy` |
| 16 | `float32` | `qz` |
| 20 | `float32` | `qw` |
| 24 | `uint64` | `timestamp_ms` |
| 32 | `uint32` | `seq` |

Total payload: **36 bytes**.

```swift
import Foundation

struct PosePacket {
    static let magic: UInt32 = 0x4C515054
    static let version: UInt32 = 1

    let qx: Float
    let qy: Float
    let qz: Float
    let qw: Float
    let timestampMs: UInt64
    let seq: UInt32

    func serialize() -> Data {
        var data = Data(capacity: 36)

        func appendLE<T: FixedWidthInteger>(_ value: T) {
            var v = value.littleEndian
            withUnsafeBytes(of: &v) { data.append(contentsOf: $0) }
        }

        func appendFloatLE(_ value: Float) {
            appendLE(value.bitPattern)
        }

        appendLE(Self.magic)
        appendLE(Self.version)
        appendFloatLE(qx)
        appendFloatLE(qy)
        appendFloatLE(qz)
        appendFloatLE(qw)
        appendLE(timestampMs)
        appendLE(seq)

        return data
    }
}
```

Send loop integration example:

```swift
var seq: UInt32 = 0
let motion = MotionManager()
let sender = UdpSender()

motion.onQuaternion = { qx, qy, qz, qw, timestampMs in
    seq &+= 1
    let packet = PosePacket(qx: qx, qy: qy, qz: qz, qw: qw, timestampMs: timestampMs, seq: seq)
    sender.send(packet.serialize())
}

motion.start()
RunLoop.main.run()
```

## 6. README Pairing Flow Outline

Proposed `companion/README.md` sections:

1. **Purpose**
   - Streams headphone orientation from macOS companion app to LocusQ plugin bridge.
2. **Requirements**
   - macOS 14+ (or project minimum), Xcode/Swift toolchain, AirPods Pro/Max or compatible device.
3. **Build and Run**
   - `swift run LocusQHeadTracker` (CLI) or Xcode run steps (SwiftUI option).
4. **Pairing / Device Preconditions**
   - Connect AirPods to Mac.
   - Confirm `CMHeadphoneMotionManager.isDeviceMotionAvailable == true`.
   - Grant motion permission when prompted.
5. **Plugin Setup**
   - Launch plugin host with LocusQ.
   - Ensure BL-017 bridge path is enabled and listening on UDP `19765`.
6. **Connectivity Check**
   - Companion logs incrementing `seq` and send rate.
   - Plugin diagnostics show `rendererHeadTrackingState=ok`, source `companion_udp`, age near realtime.
7. **Troubleshooting**
   - No motion device found, permission denied, stale packets, wrong port.
8. **Validation Capture**
   - Record command used, logs, and plugin diagnostic snapshot in test evidence entry.

## 7. Suggested Next Step

Implement Option A (Swift CLI) first, add one packet-size/unit test (`36 bytes`, header fields exact), then run manual AirPods validation with BL-017 diagnostics visible.
