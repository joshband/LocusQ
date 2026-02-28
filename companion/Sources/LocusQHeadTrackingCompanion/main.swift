import Darwin
import Foundation

#if canImport(CoreMotion)
import CoreMotion
#endif

#if canImport(CoreAudio)
import CoreAudio
#endif

#if canImport(AppKit)
import AppKit
#if canImport(WebKit)
import WebKit
#endif
#endif

private enum CompanionError: Error, CustomStringConvertible {
    case invalidArgument(String)
    case socketCreateFailed(Int32)
    case invalidIPv4Address(String)
    case sendFailed(Int32)
    case sendRetryLimitExceeded(Int32, Int)
    case interrupted(String)
    case liveModeUnavailable(String)

    var description: String {
        switch self {
        case .invalidArgument(let message):
            return "invalid argument: \(message)"
        case .socketCreateFailed(let code):
            return "failed to create UDP socket (errno=\(code))"
        case .invalidIPv4Address(let host):
            return "invalid IPv4 destination host: \(host)"
        case .sendFailed(let code):
            return "UDP send failed (errno=\(code))"
        case .sendRetryLimitExceeded(let code, let attempts):
            return "UDP send retry limit exceeded (errno=\(code), attempts=\(attempts))"
        case .interrupted(let reason):
            return "companion interrupted: \(reason)"
        case .liveModeUnavailable(let reason):
            return "live mode unavailable: \(reason)"
        }
    }
}

private enum ReliabilityConfig {
    static let maxSendRetries = 3
    static let retryBackoffMicros: [useconds_t] = [1_000, 2_000, 5_000]
}

private struct Vector3: Equatable {
    var x: Double
    var y: Double
    var z: Double

    static let zero = Vector3(x: 0.0, y: 0.0, z: 0.0)

    func magnitude() -> Double {
        sqrt((x * x) + (y * y) + (z * z))
    }

    func scaled(by scalar: Double) -> Vector3 {
        Vector3(x: x * scalar, y: y * scalar, z: z * scalar)
    }

    func clamped(min minValue: Double, max maxValue: Double) -> Vector3 {
        Vector3(
            x: max(minValue, min(maxValue, x)),
            y: max(minValue, min(maxValue, y)),
            z: max(minValue, min(maxValue, z))
        )
    }

    static func + (lhs: Vector3, rhs: Vector3) -> Vector3 {
        Vector3(x: lhs.x + rhs.x, y: lhs.y + rhs.y, z: lhs.z + rhs.z)
    }

    static func * (lhs: Vector3, rhs: Double) -> Vector3 {
        lhs.scaled(by: rhs)
    }
}

private struct Quaternion {
    var x: Float
    var y: Float
    var z: Float
    var w: Float

    static let identity = Quaternion(x: 0, y: 0, z: 0, w: 1)

    static func fromAxisAngle(axisX: Float, axisY: Float, axisZ: Float, radians: Float) -> Quaternion {
        let axisLength = sqrtf((axisX * axisX) + (axisY * axisY) + (axisZ * axisZ))
        guard axisLength > 0 else { return .identity }
        let invLength = 1.0 / axisLength
        let half = radians * 0.5
        let sinHalf = sinf(half)
        return Quaternion(
            x: axisX * invLength * sinHalf,
            y: axisY * invLength * sinHalf,
            z: axisZ * invLength * sinHalf,
            w: cosf(half)
        ).normalized()
    }

    func normalized() -> Quaternion {
        let length = sqrtf((x * x) + (y * y) + (z * z) + (w * w))
        guard length > 0 else { return .identity }
        return Quaternion(x: x / length, y: y / length, z: z / length, w: w / length)
    }

    func conjugate() -> Quaternion {
        Quaternion(x: -x, y: -y, z: -z, w: w)
    }

    func dot(_ other: Quaternion) -> Float {
        (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w)
    }

    func multiplied(by other: Quaternion) -> Quaternion {
        Quaternion(
            x: (w * other.x) + (x * other.w) + (y * other.z) - (z * other.y),
            y: (w * other.y) - (x * other.z) + (y * other.w) + (z * other.x),
            z: (w * other.z) + (x * other.y) - (y * other.x) + (z * other.w),
            w: (w * other.w) - (x * other.x) - (y * other.y) - (z * other.z)
        )
    }

    func nlerp(to target: Quaternion, alpha: Float) -> Quaternion {
        let clampedAlpha = max(0.0, min(1.0, alpha))
        var destination = target
        if dot(target) < 0 {
            destination = Quaternion(x: -target.x, y: -target.y, z: -target.z, w: -target.w)
        }
        let blended = Quaternion(
            x: x + (destination.x - x) * clampedAlpha,
            y: y + (destination.y - y) * clampedAlpha,
            z: z + (destination.z - z) * clampedAlpha,
            w: w + (destination.w - w) * clampedAlpha
        )
        return blended.normalized()
    }

    func angularDistanceDeg(to other: Quaternion) -> Float {
        let d = abs(dot(other))
        let clamped = max(-1.0 as Float, min(1.0 as Float, d))
        let angle = 2.0 * acosf(clamped)
        return angle * 57.2957795
    }

    func toEulerDegrees() -> (yaw: Float, pitch: Float, roll: Float) {
        // Intrinsic Tait-Bryan ZYX extraction (matches plugin diagnostics).
        let sinrCosp = 2.0 * ((w * x) + (y * z))
        let cosrCosp = 1.0 - 2.0 * ((x * x) + (y * y))
        let roll = atan2f(sinrCosp, cosrCosp)

        let sinp = 2.0 * ((w * y) - (z * x))
        let pitch: Float
        if abs(sinp) >= 1.0 {
            pitch = copysignf(Float.pi * 0.5, sinp)
        } else {
            pitch = asinf(sinp)
        }

        let sinyCosp = 2.0 * ((w * z) + (x * y))
        let cosyCosp = 1.0 - 2.0 * ((y * y) + (z * z))
        let yaw = atan2f(sinyCosp, cosyCosp)

        let radToDeg: Float = 57.2957795
        return (yaw * radToDeg, pitch * radToDeg, roll * radToDeg)
    }
}

// CMHeadphoneMotionManager frame harmonization:
// CoreMotion headphone attitude uses a frame where +Z is typically aligned with
// "up" for yaw semantics, while LocusQ runtime diagnostics/rendering contract is
// +X right, +Y up, -Z ahead (Steam canonical).
//
// Apply basis remap by conjugation:
//   qSteam = R * qCoreMotion * inverse(R),
// where R is +90deg about +X (maps Y->Z and Z->-Y).
private let coreMotionToSteamBasis = Quaternion.fromAxisAngle(
    axisX: 1.0,
    axisY: 0.0,
    axisZ: 0.0,
    radians: Float.pi * 0.5
)
private let steamToCoreMotionBasis = coreMotionToSteamBasis.conjugate().normalized()

private func remapCoreMotionQuaternionToSteamBasis(_ quaternion: Quaternion) -> Quaternion {
    coreMotionToSteamBasis
        .multiplied(by: quaternion)
        .multiplied(by: steamToCoreMotionBasis)
        .normalized()
}

private func remapCoreMotionVectorToSteamBasis(_ vector: Vector3) -> Vector3 {
    // +90deg rotation around +X axis:
    // x' = x
    // y' = z
    // z' = -y
    Vector3(x: vector.x, y: vector.z, z: -vector.y)
}

private func applyOrientationSignCorrections(_ quaternion: Quaternion) -> Quaternion {
    // Preserve yaw/pitch handedness from the current remap while correcting
    // shoulder-tilt roll sign for companion + plugin parity.
    let euler = quaternion.toEulerDegrees()
    return quaternionFromYawPitchRoll(
        yawDeg: euler.yaw,
        pitchDeg: euler.pitch,
        rollDeg: -euler.roll
    )
}

private struct PosePacketV1 {
    static let magic: UInt32 = 0x4C515054
    static let version: UInt32 = 1

    var quaternion: Quaternion
    var timestampMs: UInt64
    var sequence: UInt32

    func encodedData() -> Data {
        var payload = Data(capacity: 40)
        payload.appendLittleEndian(PosePacketV1.magic)
        payload.appendLittleEndian(PosePacketV1.version)
        payload.appendLittleEndian(quaternion.x)
        payload.appendLittleEndian(quaternion.y)
        payload.appendLittleEndian(quaternion.z)
        payload.appendLittleEndian(quaternion.w)
        payload.appendLittleEndian(timestampMs)
        payload.appendLittleEndian(sequence)
        payload.appendLittleEndian(UInt32(0))
        return payload
    }
}

private extension Data {
    mutating func appendLittleEndian<T: FixedWidthInteger>(_ value: T) {
        var little = value.littleEndian
        Swift.withUnsafeBytes(of: &little) { bytes in
            append(contentsOf: bytes)
        }
    }

    mutating func appendLittleEndian(_ value: Float) {
        appendLittleEndian(value.bitPattern)
    }
}

private enum CompanionMode: String {
    case synthetic
    case live
}

private enum SchedulingProfile: String {
    case eco
    case balanced
    case performance

    var operationQoS: QualityOfService {
        switch self {
        case .eco: return .utility
        case .balanced: return .userInitiated
        case .performance: return .userInteractive
        }
    }

    var ackThreadQoS: QualityOfService {
        switch self {
        case .eco: return .background
        case .balanced: return .utility
        case .performance: return .userInitiated
        }
    }

    var dispatchQoS: DispatchQoS.QoSClass {
        switch self {
        case .eco: return .utility
        case .balanced: return .userInitiated
        case .performance: return .userInteractive
        }
    }
}

private struct CompanionArguments {
    var host: String = "127.0.0.1"
    var port: UInt16 = 19765
    var pluginAckPort: UInt16 = 19766
    var hz: Int = 60
    var seconds: Int = 0
    var mode: CompanionMode = .synthetic
    var ui: Bool = false
    var verbose: Bool = false
    var schedulingProfile: SchedulingProfile = .balanced
    var monitorHz: Int = 30

    // Stabilization and frame controls (live mode)
    var recenterOnStart: Bool = true
    var requireSyncToStart: Bool = false
    var stabilizationAlpha: Float = 0.20
    var deadbandDeg: Float = 0.6
    var velocityDamping: Double = 0.92

    // Synthetic generator controls
    var yawAmplitudeDeg: Float = 35.0
    var pitchAmplitudeDeg: Float = 10.0
    var rollAmplitudeDeg: Float = 5.0
    var yawFrequencyHz: Float = 0.25

    static func usage() -> String {
        """
        LocusQ Head-Tracking Companion

        Usage:
          locusq-headtrack-companion [options]

        Core Options:
          --mode <synthetic|live>   Source mode (default: synthetic)
          --host <ipv4>             Destination host (default: 127.0.0.1)
          --port <uint16>           Destination UDP port (default: 19765)
          --plugin-ack-port <uint16> Plugin ingest-ack UDP port (default: 19766)
          --hz <int>                Target send rate in Hz (default: 60)
          --seconds <int>           Duration in seconds; 0 = run until signal (default: 0)
          --ui                       Show monitor window with raw + derived telemetry
          --verbose                  Print per-packet logs
          --sched-profile <eco|balanced|performance> Scheduler profile (default: balanced)
          --monitor-hz <int>        UI monitor redraw rate [5..120] (default: 30)

        Live Mode Controls:
          --no-recenter             Disable startup recenter transform
          --require-sync            Gate UDP pose output until "Center/Sync" is pressed in UI after ready
          --auto-sync               Disable sync gate (default behavior)
          --stabilize-alpha <float> Quaternion low-pass alpha [0..1] (default: 0.20)
          --deadband-deg <float>    Ignore tiny orientation deltas below threshold (default: 0.6)
          --velocity-damping <float> Velocity damping [0..1] for derived translation estimate (default: 0.92)

        Synthetic Mode Controls:
          --yaw-amplitude <float>   Yaw amplitude in degrees (default: 35)
          --pitch-amplitude <float> Pitch amplitude in degrees (default: 10)
          --roll-amplitude <float>  Roll amplitude in degrees (default: 5)
          --yaw-frequency <float>   Yaw oscillation frequency in Hz (default: 0.25)

        Notes:
          - Live mode uses CMHeadphoneMotionManager and requires supported headphones.
          - UDP payload remains v1 contract (40 bytes): quaternion + timestamp + sequence.
          - Monitor translation/velocity vectors are derived estimates, not absolute tracking.
        """
    }

    static func parse(_ raw: [String]) throws -> CompanionArguments? {
        var args = CompanionArguments()
        var index = 0
        while index < raw.count {
            let key = raw[index]
            switch key {
            case "--mode":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = CompanionMode(rawValue: value.lowercased()) else {
                    throw CompanionError.invalidArgument("--mode must be synthetic or live")
                }
                args.mode = parsed
            case "--host":
                args.host = try takeValue(raw, index: &index, option: key)
            case "--port":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = UInt16(value) else {
                    throw CompanionError.invalidArgument("\(key) requires uint16")
                }
                args.port = parsed
            case "--plugin-ack-port":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = UInt16(value) else {
                    throw CompanionError.invalidArgument("\(key) requires uint16")
                }
                args.pluginAckPort = parsed
            case "--hz":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Int(value), parsed > 0 else {
                    throw CompanionError.invalidArgument("\(key) requires positive integer")
                }
                args.hz = parsed
            case "--seconds":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Int(value), parsed >= 0 else {
                    throw CompanionError.invalidArgument("\(key) requires integer >= 0")
                }
                args.seconds = parsed
            case "--ui":
                args.ui = true
            case "--verbose":
                args.verbose = true
            case "--sched-profile":
                let value = try takeValue(raw, index: &index, option: key).lowercased()
                guard let parsed = SchedulingProfile(rawValue: value) else {
                    throw CompanionError.invalidArgument("\(key) must be eco, balanced, or performance")
                }
                args.schedulingProfile = parsed
            case "--monitor-hz":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Int(value), parsed >= 5, parsed <= 120 else {
                    throw CompanionError.invalidArgument("\(key) requires integer in [5,120]")
                }
                args.monitorHz = parsed
            case "--no-recenter":
                args.recenterOnStart = false
            case "--require-sync":
                args.requireSyncToStart = true
            case "--auto-sync":
                args.requireSyncToStart = false
            case "--stabilize-alpha":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Float(value), parsed >= 0.0, parsed <= 1.0 else {
                    throw CompanionError.invalidArgument("\(key) requires float in [0,1]")
                }
                args.stabilizationAlpha = parsed
            case "--deadband-deg":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Float(value), parsed >= 0 else {
                    throw CompanionError.invalidArgument("\(key) requires float >= 0")
                }
                args.deadbandDeg = parsed
            case "--velocity-damping":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Double(value), parsed >= 0.0, parsed <= 1.0 else {
                    throw CompanionError.invalidArgument("\(key) requires float in [0,1]")
                }
                args.velocityDamping = parsed
            case "--yaw-amplitude":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Float(value) else {
                    throw CompanionError.invalidArgument("\(key) requires float")
                }
                args.yawAmplitudeDeg = parsed
            case "--pitch-amplitude":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Float(value) else {
                    throw CompanionError.invalidArgument("\(key) requires float")
                }
                args.pitchAmplitudeDeg = parsed
            case "--roll-amplitude":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Float(value) else {
                    throw CompanionError.invalidArgument("\(key) requires float")
                }
                args.rollAmplitudeDeg = parsed
            case "--yaw-frequency":
                let value = try takeValue(raw, index: &index, option: key)
                guard let parsed = Float(value), parsed > 0 else {
                    throw CompanionError.invalidArgument("\(key) requires positive float")
                }
                args.yawFrequencyHz = parsed
            case "--help", "-h":
                print(CompanionArguments.usage())
                return nil
            default:
                throw CompanionError.invalidArgument("unknown option \(key)")
            }
            index += 1
        }

        return args
    }

    private static func takeValue(_ raw: [String], index: inout Int, option: String) throws -> String {
        let valueIndex = index + 1
        guard valueIndex < raw.count else {
            throw CompanionError.invalidArgument("\(option) requires a value")
        }
        index = valueIndex
        return raw[valueIndex]
    }
}

nonisolated(unsafe) private var gStopRequested: sig_atomic_t = 0
nonisolated(unsafe) private var gSyncRequested = false
private let gSyncRequestLock = NSLock()

private func markStopRequested() {
    gStopRequested = 1
}

private func clearStopRequested() {
    gStopRequested = 0
}

private func stopRequested() -> Bool {
    gStopRequested != 0
}

private func requestSyncFromUI() {
    gSyncRequestLock.lock()
    gSyncRequested = true
    gSyncRequestLock.unlock()
}

private func consumeSyncRequest() -> Bool {
    gSyncRequestLock.lock()
    defer { gSyncRequestLock.unlock() }
    if gSyncRequested {
        gSyncRequested = false
        return true
    }
    return false
}

private func handleTerminationSignal(_ signal: Int32) -> Void {
    _ = signal
    markStopRequested()
}

private func installTerminationHandlers() {
    _ = Darwin.signal(SIGINT, handleTerminationSignal)
    _ = Darwin.signal(SIGTERM, handleTerminationSignal)
}

private func restoreTerminationHandlers() {
    _ = Darwin.signal(SIGINT, SIG_DFL)
    _ = Darwin.signal(SIGTERM, SIG_DFL)
}

private final class UDPSender {
    private let socketFD: Int32
    private var destination: sockaddr_in
    private var closed = false

    init(host: String, port: UInt16) throws {
        socketFD = Darwin.socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
        guard socketFD >= 0 else {
            throw CompanionError.socketCreateFailed(errno)
        }

        destination = sockaddr_in()
        destination.sin_len = UInt8(MemoryLayout<sockaddr_in>.size)
        destination.sin_family = sa_family_t(AF_INET)
        destination.sin_port = CFSwapInt16HostToBig(port)

        let converted = host.withCString { cHost in
            inet_pton(AF_INET, cHost, &destination.sin_addr)
        }
        guard converted == 1 else {
            Darwin.close(socketFD)
            throw CompanionError.invalidIPv4Address(host)
        }

        var timeout = timeval(tv_sec: 1, tv_usec: 0)
        _ = withUnsafePointer(to: &timeout) { ptr in
            Darwin.setsockopt(
                socketFD,
                SOL_SOCKET,
                SO_SNDTIMEO,
                ptr,
                socklen_t(MemoryLayout<timeval>.size)
            )
        }
    }

    deinit {
        close()
    }

    func close() {
        if closed {
            return
        }
        closed = true
        Darwin.close(socketFD)
    }

    func send(_ payload: Data) throws {
        var attempt = 0
        while true {
            if stopRequested() {
                throw CompanionError.interrupted("signal_stop_before_send")
            }

            var sendErr: Int32 = 0
            let sent = payload.withUnsafeBytes { bytes -> Int in
                var target = destination
                return withUnsafePointer(to: &target) { ptr in
                    ptr.withMemoryRebound(to: sockaddr.self, capacity: 1) { sockaddrPtr in
                        let rc = Darwin.sendto(
                            socketFD,
                            bytes.baseAddress,
                            payload.count,
                            0,
                            sockaddrPtr,
                            socklen_t(MemoryLayout<sockaddr_in>.size)
                        )
                        if rc < 0 {
                            sendErr = errno
                        }
                        return rc
                    }
                }
            }

            if sent == payload.count {
                return
            }

            let errCode: Int32 = (sent < 0 ? sendErr : EIO)
            let transientErr = (errCode == EINTR || errCode == EAGAIN || errCode == EWOULDBLOCK || errCode == ENOBUFS)
            if transientErr && attempt < ReliabilityConfig.maxSendRetries && !stopRequested() {
                let backoff = ReliabilityConfig.retryBackoffMicros[min(attempt, ReliabilityConfig.retryBackoffMicros.count - 1)]
                usleep(backoff)
                attempt += 1
                continue
            }

            if transientErr && attempt >= ReliabilityConfig.maxSendRetries {
                throw CompanionError.sendRetryLimitExceeded(errCode, attempt + 1)
            }

            throw CompanionError.sendFailed(errCode)
        }
    }
}

private struct StreamHealthSnapshot {
    var effectiveRateHz: Double = 0.0
    var intervalMs: Double = 0.0
    var jitterMs: Double = 0.0
    var seqGap: Int = 0
}

private struct PluginIngestSnapshot {
    var state: String = "unavailable"
    var sourceCount: Int = 0
    var consumerCount: Int = 0
    var endpoint: String = "n/a"
    var sequence: UInt32 = 0
    var poseAgeMs: Double = 0.0
    var ackAgeMs: Double = 0.0
    var invalidPackets: UInt32 = 0
    var ackPackets: Int = 0
    var decodeErrors: Int = 0
}

private struct OutputDeviceSnapshot {
    var name: String = "n/a"
    var model: String = "n/a"
    var transport: String = "n/a"
    var sampleRateHz: Double = 0.0
    var channels: Int = 0
    var connected: Bool = false
}

private struct RuntimeSnapshot {
    var mode: CompanionMode = .synthetic
    var source: String = "synthetic_generator"
    var connection: String = "n/a"
    var schedulingProfile: String = SchedulingProfile.balanced.rawValue
    var monitorHz: Int = 30
    var sequence: UInt32 = 0
    var timestampMs: UInt64 = 0
    var ageMs: Double = 0.0

    var qx: Float = 0.0
    var qy: Float = 0.0
    var qz: Float = 0.0
    var qw: Float = 1.0

    var yawDeg: Float = 0.0
    var pitchDeg: Float = 0.0
    var rollDeg: Float = 0.0
    var frameMapping: String = "steam_basis_identity"
    var baselineState: String = "n/a"
    var readinessState: String = "disabled_disconnected"
    var sendGateOpen: Bool = false
    var syncRequired: Bool = false

    var rotRateXDeg: Double = 0.0
    var rotRateYDeg: Double = 0.0
    var rotRateZDeg: Double = 0.0
    var angularSpeedDegPerSec: Double = 0.0

    var gravity = Vector3.zero
    var userAccelerationG = Vector3.zero

    var velocityEstimateMps = Vector3.zero
    var displacementEstimateM = Vector3.zero

    var dtSec: Double = 0.0
    var headingDeg: Double? = nil
    var sensorLocation: String = "default"

    var motionNorm: Double = 0.0
    var stabilityNorm: Double = 1.0

    var packetCount: Int = 0
    var sendErrors: Int = 0
    var invalidSamples: Int = 0
    var lastError: String = ""
    var streamHealth = StreamHealthSnapshot()
    var pluginIngest = PluginIngestSnapshot()
    var outputDevice = OutputDeviceSnapshot()
}

private final class SnapshotStore: @unchecked Sendable {
    private let lock = NSLock()
    private var snapshot = RuntimeSnapshot()

    func read() -> RuntimeSnapshot {
        lock.lock()
        defer { lock.unlock() }
        return snapshot
    }

    func update(_ mutate: (inout RuntimeSnapshot) -> Void) {
        lock.lock()
        defer { lock.unlock() }
        mutate(&snapshot)
    }
}

private final class PluginAckReceiver: @unchecked Sendable {
    private struct AckEntry {
        var sourceToken: UInt32
        var consumerCount: UInt32
        var sequence: UInt32
        var invalidPackets: UInt32
        var poseAgeMs: Double
        var flags: UInt32
        var endpoint: String
        var receivedAtMs: UInt64
    }

    private let lock = NSLock()
    private let listenPort: UInt16
    private var entries: [UInt32: AckEntry] = [:]
    private var packetCount: Int = 0
    private var decodeErrors: Int = 0
    private var running = false
    private var socketFD: Int32 = -1
    private var thread: Thread?
    private let workerQoS: QualityOfService

    private static let packetMagic: UInt32 = 0x4C514143 // "LQAC"
    private static let packetVersion: UInt32 = 1
    private static let packetSize: Int = 48
    private static let staleWindowMs: UInt64 = 2_000
    private static let flagPoseStale: UInt32 = 1 << 1

    init(listenPort: UInt16 = 19766, workerQoS: QualityOfService = .utility) {
        self.listenPort = listenPort
        self.workerQoS = workerQoS
    }

    func start() {
        lock.lock()
        if running {
            lock.unlock()
            return
        }
        running = true
        lock.unlock()

        let worker = Thread { [weak self] in
            self?.runLoop()
        }
        worker.name = "LocusQHeadTracker.PluginAck"
        worker.qualityOfService = workerQoS
        lock.lock()
        thread = worker
        lock.unlock()
        worker.start()
    }

    func stop() {
        lock.lock()
        running = false
        let fd = socketFD
        socketFD = -1
        lock.unlock()

        if fd >= 0 {
            _ = Darwin.shutdown(fd, SHUT_RDWR)
            Darwin.close(fd)
        }

        if let worker = thread {
            while worker.isExecuting {
                usleep(10_000)
            }
        }

        lock.lock()
        thread = nil
        lock.unlock()
    }

    func snapshot(nowMs: UInt64) -> PluginIngestSnapshot {
        lock.lock()
        defer { lock.unlock() }

        var active: [AckEntry] = []
        active.reserveCapacity(entries.count)
        var staleKeys: [UInt32] = []
        staleKeys.reserveCapacity(entries.count)

        for (key, entry) in entries {
            if nowMs >= entry.receivedAtMs && (nowMs - entry.receivedAtMs) <= Self.staleWindowMs {
                active.append(entry)
            } else {
                staleKeys.append(key)
            }
        }

        for key in staleKeys {
            entries.removeValue(forKey: key)
        }

        guard !active.isEmpty else {
            return PluginIngestSnapshot(
                state: packetCount > 0 ? "stale" : "waiting",
                sourceCount: 0,
                consumerCount: 0,
                endpoint: "n/a",
                sequence: 0,
                poseAgeMs: 0.0,
                ackAgeMs: 0.0,
                invalidPackets: 0,
                ackPackets: packetCount,
                decodeErrors: decodeErrors
            )
        }

        let latest = active.max { lhs, rhs in
            lhs.receivedAtMs < rhs.receivedAtMs
        } ?? active[0]
        let totalConsumers = active.reduce(0) { partial, item in
            partial + Int(item.consumerCount)
        }
        let ackAgeMs = nowMs >= latest.receivedAtMs ? Double(nowMs - latest.receivedAtMs) : 0.0
        let poseStale = (latest.flags & Self.flagPoseStale) != 0
        var ingestState = ackAgeMs > 500.0 ? "stale" : "active"
        if ingestState == "active" && poseStale {
            ingestState = "pose_stale"
        }

        return PluginIngestSnapshot(
            state: ingestState,
            sourceCount: active.count,
            consumerCount: totalConsumers,
            endpoint: latest.endpoint,
            sequence: latest.sequence,
            poseAgeMs: latest.poseAgeMs,
            ackAgeMs: ackAgeMs,
            invalidPackets: latest.invalidPackets,
            ackPackets: packetCount,
            decodeErrors: decodeErrors
        )
    }

    private func runLoop() {
        let fd = Darwin.socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
        if fd < 0 {
            lock.lock()
            decodeErrors += 1
            running = false
            lock.unlock()
            return
        }

        var reuse: Int32 = 1
        _ = Darwin.setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, socklen_t(MemoryLayout<Int32>.size))
        _ = Darwin.setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, socklen_t(MemoryLayout<Int32>.size))

        var timeout = timeval(tv_sec: 0, tv_usec: 200_000)
        _ = withUnsafePointer(to: &timeout) { ptr in
            Darwin.setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, ptr, socklen_t(MemoryLayout<timeval>.size))
        }

        var address = sockaddr_in()
        address.sin_len = UInt8(MemoryLayout<sockaddr_in>.size)
        address.sin_family = sa_family_t(AF_INET)
        address.sin_port = CFSwapInt16HostToBig(listenPort)
        address.sin_addr = in_addr(s_addr: inet_addr("127.0.0.1"))

        let bindStatus: Int32 = withUnsafePointer(to: &address) { ptr in
            ptr.withMemoryRebound(to: sockaddr.self, capacity: 1) { saPtr in
                Darwin.bind(fd, saPtr, socklen_t(MemoryLayout<sockaddr_in>.size))
            }
        }

        if bindStatus != 0 {
            Darwin.close(fd)
            lock.lock()
            decodeErrors += 1
            running = false
            lock.unlock()
            return
        }

        lock.lock()
        socketFD = fd
        lock.unlock()

        var receiveBuffer = [UInt8](repeating: 0, count: 256)
        while true {
            lock.lock()
            let isRunning = running
            lock.unlock()
            if !isRunning || stopRequested() {
                break
            }

            var peer = sockaddr_in()
            var peerLen = socklen_t(MemoryLayout<sockaddr_in>.size)
            let received = withUnsafeMutablePointer(to: &peer) { peerPtr in
                peerPtr.withMemoryRebound(to: sockaddr.self, capacity: 1) { saPtr in
                    Darwin.recvfrom(
                        fd,
                        &receiveBuffer,
                        receiveBuffer.count,
                        0,
                        saPtr,
                        &peerLen
                    )
                }
            }

            if received < 0 {
                if errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR {
                    continue
                }
                lock.lock()
                decodeErrors += 1
                lock.unlock()
                continue
            }

            if received == 0 {
                continue
            }

            guard let entry = decodeAckPacket(bytes: receiveBuffer, count: Int(received), peer: peer) else {
                lock.lock()
                decodeErrors += 1
                lock.unlock()
                continue
            }

            lock.lock()
            entries[entry.sourceToken] = entry
            packetCount += 1
            lock.unlock()
        }

        Darwin.close(fd)
        lock.lock()
        socketFD = -1
        running = false
        lock.unlock()
    }

    private func decodeAckPacket(bytes: [UInt8], count: Int, peer: sockaddr_in) -> AckEntry? {
        guard count >= Self.packetSize else {
            return nil
        }

        let magic = readU32LE(bytes, 0)
        let version = readU32LE(bytes, 4)
        guard magic == Self.packetMagic, version == Self.packetVersion else {
            return nil
        }

        let sourceToken = readU32LE(bytes, 8)
        let consumerCount = readU32LE(bytes, 12)
        let sequence = readU32LE(bytes, 16)
        let invalidPackets = readU32LE(bytes, 20)
        let poseAgeMs = Double(readF32LE(bytes, 32))
        let flags = readU32LE(bytes, 36)
        let receivedAtMs = nowEpochMilliseconds()

        return AckEntry(
            sourceToken: sourceToken,
            consumerCount: consumerCount,
            sequence: sequence,
            invalidPackets: invalidPackets,
            poseAgeMs: poseAgeMs,
            flags: flags,
            endpoint: endpointString(peer: peer),
            receivedAtMs: receivedAtMs
        )
    }

    private func endpointString(peer: sockaddr_in) -> String {
        var address = peer.sin_addr
        var hostBuffer = [CChar](repeating: 0, count: Int(INET_ADDRSTRLEN))
        let host = withUnsafePointer(to: &address) { ptr -> String in
            let cString = Darwin.inet_ntop(AF_INET, ptr, &hostBuffer, socklen_t(INET_ADDRSTRLEN))
            if cString == nil {
                return "127.0.0.1"
            }
            let bytes = hostBuffer.prefix { $0 != 0 }.map { UInt8(bitPattern: $0) }
            return String(decoding: bytes, as: UTF8.self)
        }
        let port = Int(CFSwapInt16BigToHost(peer.sin_port))
        return "\(host):\(port)"
    }

    private func readU32LE(_ bytes: [UInt8], _ offset: Int) -> UInt32 {
        UInt32(bytes[offset + 0])
            | (UInt32(bytes[offset + 1]) << 8)
            | (UInt32(bytes[offset + 2]) << 16)
            | (UInt32(bytes[offset + 3]) << 24)
    }

    private func readF32LE(_ bytes: [UInt8], _ offset: Int) -> Float {
        let raw = readU32LE(bytes, offset)
        return Float(bitPattern: raw)
    }
}

#if canImport(CoreAudio)
private func mapTransportType(_ value: UInt32) -> String {
    switch value {
    case kAudioDeviceTransportTypeBuiltIn:
        return "built_in"
    case kAudioDeviceTransportTypeBluetooth:
        return "bluetooth"
    case kAudioDeviceTransportTypeUSB:
        return "usb"
    case kAudioDeviceTransportTypeHDMI:
        return "hdmi"
    case kAudioDeviceTransportTypeAirPlay:
        return "airplay"
    case kAudioDeviceTransportTypeVirtual:
        return "virtual"
    case kAudioDeviceTransportTypeAggregate:
        return "aggregate"
    default:
        return "unknown_\(value)"
    }
}

private func readDeviceString(_ deviceID: AudioDeviceID,
                              selector: AudioObjectPropertySelector,
                              scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal) -> String? {
    var address = AudioObjectPropertyAddress(
        mSelector: selector,
        mScope: scope,
        mElement: kAudioObjectPropertyElementMain
    )
    var value: CFTypeRef?
    var size = UInt32(MemoryLayout<CFTypeRef?>.size)
    let status = AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &value)
    guard status == noErr, let text = value as? String else {
        return nil
    }
    return text
}

private func readDeviceDouble(_ deviceID: AudioDeviceID,
                              selector: AudioObjectPropertySelector,
                              scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal) -> Double? {
    var address = AudioObjectPropertyAddress(
        mSelector: selector,
        mScope: scope,
        mElement: kAudioObjectPropertyElementMain
    )
    var value: Double = 0
    var size = UInt32(MemoryLayout<Double>.size)
    let status = AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &value)
    guard status == noErr else {
        return nil
    }
    return value
}

private func readDeviceUInt32(_ deviceID: AudioDeviceID,
                              selector: AudioObjectPropertySelector,
                              scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal) -> UInt32? {
    var address = AudioObjectPropertyAddress(
        mSelector: selector,
        mScope: scope,
        mElement: kAudioObjectPropertyElementMain
    )
    var value: UInt32 = 0
    var size = UInt32(MemoryLayout<UInt32>.size)
    let status = AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &value)
    guard status == noErr else {
        return nil
    }
    return value
}

private func readDefaultOutputDeviceSnapshot() -> OutputDeviceSnapshot {
    var address = AudioObjectPropertyAddress(
        mSelector: kAudioHardwarePropertyDefaultOutputDevice,
        mScope: kAudioObjectPropertyScopeGlobal,
        mElement: kAudioObjectPropertyElementMain
    )
    var deviceID: AudioDeviceID = 0
    var size = UInt32(MemoryLayout<AudioDeviceID>.size)
    let status = AudioObjectGetPropertyData(
        AudioObjectID(kAudioObjectSystemObject),
        &address,
        0,
        nil,
        &size,
        &deviceID
    )
    guard status == noErr, deviceID != 0 else {
        return OutputDeviceSnapshot()
    }

    let name = readDeviceString(deviceID, selector: kAudioObjectPropertyName) ?? "unknown"
    let model = readDeviceString(deviceID, selector: kAudioObjectPropertyModelName) ?? name
    let sampleRate = readDeviceDouble(deviceID, selector: kAudioDevicePropertyNominalSampleRate, scope: kAudioDevicePropertyScopeOutput) ?? 0.0
    let channels = Int(readDeviceUInt32(deviceID, selector: kAudioDevicePropertyPreferredChannelsForStereo, scope: kAudioDevicePropertyScopeOutput) != nil ? 2 : 0)
    let transportCode = readDeviceUInt32(deviceID, selector: kAudioDevicePropertyTransportType) ?? 0
    let transport = mapTransportType(transportCode)
    let connectedFlag = readDeviceUInt32(deviceID, selector: kAudioDevicePropertyDeviceIsAlive) ?? 0

    return OutputDeviceSnapshot(
        name: name,
        model: model,
        transport: transport,
        sampleRateHz: sampleRate,
        channels: channels,
        connected: connectedFlag != 0
    )
}
#else
private func readDefaultOutputDeviceSnapshot() -> OutputDeviceSnapshot {
    OutputDeviceSnapshot()
}
#endif

#if canImport(AppKit)
#if canImport(WebKit)
@MainActor
private final class CompanionMonitorWindow: NSObject, NSWindowDelegate, WKScriptMessageHandler {
    private let window: NSWindow
    private let webView: WKWebView
    private var pendingPayloadJSON: String?

    override init() {
        let frame = NSRect(x: 80, y: 80, width: 1380, height: 900)
        window = NSWindow(
            contentRect: frame,
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = "LocusQ Head-Tracking Companion"
        window.minSize = NSSize(width: 1100, height: 700)
        window.center()

        let configuration = WKWebViewConfiguration()
        configuration.defaultWebpagePreferences.allowsContentJavaScript = true
        let userContentController = WKUserContentController()
        configuration.userContentController = userContentController
        webView = WKWebView(frame: frame, configuration: configuration)
        webView.autoresizingMask = [.width, .height]

        super.init()
        userContentController.add(self, name: "locusqControl")
        window.delegate = self

        if let contentView = window.contentView {
            webView.frame = contentView.bounds
            contentView.addSubview(webView)
        }

        let (threeTag, baseURL) = Self.resolveThreeScriptTag()
        webView.loadHTMLString(Self.dashboardHTML(threeScriptTag: threeTag), baseURL: baseURL)
    }

    func show() {
        let app = NSApplication.shared
        app.setActivationPolicy(.regular)
        window.makeKeyAndOrderFront(nil)
        app.activate(ignoringOtherApps: true)
    }

    func windowWillClose(_ notification: Notification) {
        _ = notification
        markStopRequested()
    }

    func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        guard message.name == "locusqControl" else { return }
        guard let body = message.body as? [String: Any],
              let type = body["type"] as? String
        else {
            return
        }
        if type == "sync" {
            requestSyncFromUI()
        }
    }

    func render(snapshot: RuntimeSnapshot, args: CompanionArguments) {
        guard let payloadJSON = Self.serializePayload(snapshot: snapshot, args: args) else {
            return
        }
        pendingPayloadJSON = payloadJSON
        flushPendingPayload()
    }

    private func flushPendingPayload() {
        guard let payloadJSON = pendingPayloadJSON else {
            return
        }
        pendingPayloadJSON = nil
        let script = "window.__companionUpdate && window.__companionUpdate(\(payloadJSON));"
        webView.evaluateJavaScript(script) { _, error in
            if let error {
                self.pendingPayloadJSON = payloadJSON
                fputs("companion_monitor_js_error: \(error)\n", stderr)
            }
        }
    }

    private static func serializePayload(snapshot: RuntimeSnapshot, args: CompanionArguments) -> String? {
        let payload: [String: Any] = [
            "mode": snapshot.mode.rawValue,
            "source": snapshot.source,
            "connection": snapshot.connection,
            "destination": "\(args.host):\(args.port)",
            "schedulingProfile": snapshot.schedulingProfile,
            "monitorHz": snapshot.monitorHz,
            "rateHz": args.hz,
            "durationText": args.seconds == 0 ? "until signal" : "\(args.seconds)s",
            "sequence": snapshot.sequence,
            "timestampMs": snapshot.timestampMs,
            "ageMs": snapshot.ageMs,
            "packetCount": snapshot.packetCount,
            "sendErrors": snapshot.sendErrors,
            "invalidSamples": snapshot.invalidSamples,
            "lastError": snapshot.lastError,
            "quaternion": [
                "x": snapshot.qx,
                "y": snapshot.qy,
                "z": snapshot.qz,
                "w": snapshot.qw
            ],
            "yawPitchRollDeg": [
                "yaw": snapshot.yawDeg,
                "pitch": snapshot.pitchDeg,
                "roll": snapshot.rollDeg
            ],
            "frameMapping": snapshot.frameMapping,
            "baselineState": snapshot.baselineState,
            "readinessState": snapshot.readinessState,
            "sendGateOpen": snapshot.sendGateOpen,
            "syncRequired": snapshot.syncRequired,
            "rotationRateDegPerSec": [
                "x": snapshot.rotRateXDeg,
                "y": snapshot.rotRateYDeg,
                "z": snapshot.rotRateZDeg
            ],
            "angularSpeedDegPerSec": snapshot.angularSpeedDegPerSec,
            "gravityG": [
                "x": snapshot.gravity.x,
                "y": snapshot.gravity.y,
                "z": snapshot.gravity.z
            ],
            "userAccelerationG": [
                "x": snapshot.userAccelerationG.x,
                "y": snapshot.userAccelerationG.y,
                "z": snapshot.userAccelerationG.z
            ],
            "velocityEstimateMps": [
                "x": snapshot.velocityEstimateMps.x,
                "y": snapshot.velocityEstimateMps.y,
                "z": snapshot.velocityEstimateMps.z
            ],
            "displacementEstimateM": [
                "x": snapshot.displacementEstimateM.x,
                "y": snapshot.displacementEstimateM.y,
                "z": snapshot.displacementEstimateM.z
            ],
            "dtSec": snapshot.dtSec,
            "headingDeg": snapshot.headingDeg ?? NSNull(),
            "sensorLocation": snapshot.sensorLocation,
            "motionNorm": snapshot.motionNorm,
            "stabilityNorm": snapshot.stabilityNorm,
            "streamHealth": [
                "effectiveRateHz": snapshot.streamHealth.effectiveRateHz,
                "intervalMs": snapshot.streamHealth.intervalMs,
                "jitterMs": snapshot.streamHealth.jitterMs,
                "seqGap": snapshot.streamHealth.seqGap
            ],
            "pluginIngest": [
                "state": snapshot.pluginIngest.state,
                "sourceCount": snapshot.pluginIngest.sourceCount,
                "consumerCount": snapshot.pluginIngest.consumerCount,
                "endpoint": snapshot.pluginIngest.endpoint,
                "sequence": snapshot.pluginIngest.sequence,
                "poseAgeMs": snapshot.pluginIngest.poseAgeMs,
                "ackAgeMs": snapshot.pluginIngest.ackAgeMs,
                "invalidPackets": snapshot.pluginIngest.invalidPackets,
                "ackPackets": snapshot.pluginIngest.ackPackets,
                "decodeErrors": snapshot.pluginIngest.decodeErrors
            ],
            "outputDevice": [
                "name": snapshot.outputDevice.name,
                "model": snapshot.outputDevice.model,
                "transport": snapshot.outputDevice.transport,
                "sampleRateHz": snapshot.outputDevice.sampleRateHz,
                "channels": snapshot.outputDevice.channels,
                "connected": snapshot.outputDevice.connected
            ],
            "controls": [
                "recenterOnStart": args.recenterOnStart,
                "requireSyncToStart": args.requireSyncToStart,
                "stabilizationAlpha": args.stabilizationAlpha,
                "deadbandDeg": args.deadbandDeg,
                "velocityDamping": args.velocityDamping
            ]
        ]

        guard JSONSerialization.isValidJSONObject(payload),
              let jsonData = try? JSONSerialization.data(withJSONObject: payload, options: []),
              let jsonString = String(data: jsonData, encoding: .utf8)
        else {
            return nil
        }
        return jsonString
    }

    private static func resolveThreeScriptTag() -> (String, URL?) {
        let fileManager = FileManager.default
        var candidates: [URL] = []

        if let bundled = Bundle.main.url(forResource: "three.min", withExtension: "js") {
            candidates.append(bundled)
        }

        let cwd = URL(fileURLWithPath: fileManager.currentDirectoryPath, isDirectory: true)
        candidates.append(cwd.appendingPathComponent("Source/ui/public/js/three.min.js"))
        candidates.append(cwd.appendingPathComponent("companion/Resources/three.min.js"))

        let executableURL = URL(fileURLWithPath: CommandLine.arguments.first ?? "")
        var cursor = executableURL.deletingLastPathComponent()
        for _ in 0..<10 {
            candidates.append(cursor.appendingPathComponent("Source/ui/public/js/three.min.js"))
            candidates.append(cursor.appendingPathComponent("companion/Resources/three.min.js"))
            cursor.deleteLastPathComponent()
        }

        for url in candidates where fileManager.fileExists(atPath: url.path) {
            let directory = url.deletingLastPathComponent()
            return ("<script src=\"\(url.lastPathComponent)\"></script>", directory)
        }

        return ("<script src=\"https://unpkg.com/three@0.161.0/build/three.min.js\"></script>", nil)
    }

    private static func dashboardHTML(threeScriptTag: String) -> String {
        """
        <!doctype html>
        <html lang="en">
        <head>
          <meta charset="utf-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1" />
          <title>LocusQ Head-Tracking Companion</title>
          <style>
            :root {
              --bg: #05070c;
              --card: #0f151f;
              --card-border: #1f2f46;
              --muted: #8fa7c4;
              --text: #ddeaf9;
              --accent: #51d2ff;
              --accent-soft: rgba(81, 210, 255, 0.18);
              --accent-glow: rgba(75, 198, 255, 0.2);
              --ok: #46e089;
              --warn: #ffca66;
              --error: #ff6c6c;
              --font-xs: clamp(8px, 0.58vw, 10px);
              --font-sm: clamp(9px, 0.68vw, 11px);
              --row-pad-y: clamp(2px, 0.24vh, 4px);
              --row-pad-x: clamp(7px, 0.55vw, 9px);
            }
            * { box-sizing: border-box; }
            html, body {
              margin: 0;
              width: 100%;
              height: 100%;
              overflow: hidden;
              background: radial-gradient(circle at 15% 10%, #0f1a2b 0%, #05070c 60%);
              color: var(--text);
              font-family: "SF Mono", "Menlo", "Monaco", monospace;
              letter-spacing: 0.01em;
            }
            #root {
              display: grid;
              grid-template-columns: minmax(480px, 1.04fr) minmax(420px, 0.96fr);
              gap: clamp(8px, 1vw, 12px);
              width: 100%;
              height: 100%;
              padding: clamp(8px, 1vw, 12px);
              background:
                radial-gradient(1200px 700px at 8% 12%, rgba(24, 58, 92, 0.22), transparent 60%),
                radial-gradient(900px 500px at 88% 92%, rgba(19, 37, 64, 0.28), transparent 66%);
            }
            @media (max-width: 1240px) {
              #root {
                grid-template-columns: 1fr;
                grid-template-rows: 54% 46%;
              }
            }
            @media (max-height: 900px) {
              #root {
                padding: 8px;
                gap: 8px;
              }
            }
            .card {
              border: 1px solid var(--card-border);
              border-radius: 14px;
              background:
                linear-gradient(180deg, rgba(13, 20, 31, 0.97), rgba(8, 12, 19, 0.985)),
                radial-gradient(180% 130% at 10% 2%, rgba(31, 79, 122, 0.14), transparent 65%);
              box-shadow: 0 18px 50px rgba(0, 0, 0, 0.45);
              overflow: hidden;
              min-height: 0;
            }
            #vizCard {
              display: grid;
              grid-template-rows: auto 1fr;
            }
            .cardHeader {
              display: flex;
              justify-content: space-between;
              align-items: center;
              gap: 12px;
              padding: clamp(8px, 0.95vh, 10px) clamp(12px, 0.95vw, 14px);
              border-bottom: 1px solid rgba(84, 115, 160, 0.28);
              background:
                linear-gradient(90deg, rgba(18, 32, 50, 0.58), rgba(11, 24, 38, 0.34)),
                rgba(16, 28, 44, 0.36);
            }
            .headerTitle {
              font-size: var(--font-sm);
              letter-spacing: 0.18em;
              text-transform: uppercase;
              color: #89b8de;
            }
            #statusPill {
              font-size: var(--font-sm);
              font-weight: 700;
              padding: clamp(4px, 0.5vh, 5px) clamp(9px, 0.7vw, 11px);
              border-radius: 999px;
              border: 1px solid rgba(93, 149, 214, 0.38);
              color: #86d6ff;
              background: rgba(43, 86, 124, 0.28);
            }
            #statusPill.active {
              color: #74edaa;
              border-color: rgba(102, 209, 143, 0.5);
              background: rgba(44, 113, 78, 0.25);
            }
            #statusPill.warn {
              color: #ffd17d;
              border-color: rgba(255, 184, 83, 0.45);
              background: rgba(122, 80, 21, 0.25);
            }
            #statusPill.error {
              color: #ff9393;
              border-color: rgba(255, 113, 113, 0.5);
              background: rgba(123, 38, 38, 0.32);
            }
            #vizWrap {
              position: relative;
              width: 100%;
              height: 100%;
              min-height: 320px;
              overflow: hidden;
            }
            #vizWrap::before {
              content: "";
              position: absolute;
              inset: 0;
              pointer-events: none;
              background:
                radial-gradient(circle at center, rgba(38, 103, 155, 0.10), transparent 42%),
                radial-gradient(circle at center, rgba(12, 34, 59, 0.20) 0%, transparent 70%);
              z-index: 0;
            }
            #viz {
              position: absolute;
              inset: 0;
              z-index: 1;
            }
            #centerReticle {
              position: absolute;
              left: 50%;
              top: 50%;
              width: 16px;
              height: 16px;
              transform: translate(-50%, -50%);
              pointer-events: none;
              opacity: 0.58;
              z-index: 2;
            }
            #centerReticle::before,
            #centerReticle::after {
              content: "";
              position: absolute;
              background: rgba(93, 188, 255, 0.5);
              box-shadow: 0 0 10px rgba(93, 188, 255, 0.2);
            }
            #centerReticle::before {
              left: 7px;
              top: 0;
              width: 2px;
              height: 16px;
            }
            #centerReticle::after {
              left: 0;
              top: 7px;
              width: 16px;
              height: 2px;
            }
            #orientationOverlay {
              position: absolute;
              top: 12px;
              left: 12px;
              display: flex;
              flex-wrap: wrap;
              gap: 6px;
              max-width: min(360px, calc(100% - 24px));
              z-index: 4;
              pointer-events: none;
            }
            .orientationChip {
              font-size: var(--font-xs);
              letter-spacing: 0.08em;
              text-transform: uppercase;
              border-radius: 999px;
              border: 1px solid rgba(96, 138, 184, 0.44);
              padding: 3px 8px;
              background: rgba(8, 17, 28, 0.82);
              color: #b5d4ee;
            }
            .orientationChip.left {
              color: #a9dbff;
              border-color: rgba(84, 183, 255, 0.56);
              background: rgba(22, 65, 98, 0.5);
            }
            .orientationChip.right {
              color: #ffc1cf;
              border-color: rgba(255, 145, 170, 0.56);
              background: rgba(91, 35, 53, 0.5);
            }
            .orientationHint {
              width: 100%;
              font-size: var(--font-xs);
              color: #9ec3e6;
              letter-spacing: 0.05em;
              padding: 2px 0 0 1px;
              text-shadow: 0 0 10px rgba(38, 121, 188, 0.25);
            }
            #viewControls {
              position: absolute;
              top: 12px;
              right: 12px;
              display: inline-flex;
              gap: 6px;
              z-index: 5;
              pointer-events: auto;
            }
            .viewBtn {
              border: 1px solid rgba(74, 119, 167, 0.56);
              border-radius: 6px;
              background: rgba(8, 16, 26, 0.86);
              color: #a8c7e4;
              font-size: var(--font-xs);
              font-weight: 700;
              letter-spacing: 0.12em;
              min-width: 28px;
              height: 24px;
              line-height: 1;
              padding: 0 8px;
              cursor: pointer;
              transition: all 0.12s ease;
            }
            .viewBtn:hover {
              border-color: rgba(100, 185, 255, 0.72);
              color: #d5ecff;
              background: rgba(18, 37, 56, 0.9);
            }
            .viewBtn.active {
              border-color: rgba(102, 196, 255, 0.9);
              color: #f5fbff;
              background: linear-gradient(180deg, rgba(52, 124, 179, 0.64), rgba(31, 87, 135, 0.62));
              box-shadow: 0 0 10px rgba(78, 176, 248, 0.24);
            }
            #legend {
              position: absolute;
              left: 12px;
              bottom: 12px;
              display: grid;
              grid-template-columns: repeat(2, minmax(140px, 1fr));
              gap: 6px 12px;
              padding: clamp(8px, 0.8vh, 10px) clamp(10px, 0.8vw, 12px);
              border-radius: 10px;
              border: 1px solid rgba(76, 108, 149, 0.5);
              background: linear-gradient(180deg, rgba(8, 17, 28, 0.86), rgba(7, 15, 24, 0.76));
              font-size: var(--font-xs);
              color: #a9c3df;
              backdrop-filter: blur(3px);
              box-shadow: 0 8px 20px rgba(0, 0, 0, 0.35), 0 0 20px rgba(65, 155, 233, 0.08);
              z-index: 3;
            }
            .legendDot {
              display: inline-block;
              width: 9px;
              height: 9px;
              border-radius: 999px;
              margin-right: 6px;
            }
            #diagCard {
              display: grid;
              grid-template-rows: auto auto minmax(0, 1fr);
              min-height: 0;
            }
            #smoothingControls {
              display: grid;
              grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
              gap: 8px 10px;
              padding: clamp(8px, 0.9vh, 10px) clamp(12px, 0.9vw, 14px);
              border-bottom: 1px solid rgba(84, 115, 160, 0.28);
              background: rgba(10, 18, 30, 0.45);
              font-size: var(--font-xs);
            }
            .controlBlock {
              display: grid;
              gap: 5px;
            }
            .controlBlock label {
              color: var(--muted);
              text-transform: uppercase;
              letter-spacing: 0.12em;
              font-size: var(--font-xs);
            }
            .controlBlock input[type="range"] {
              width: 100%;
              accent-color: var(--accent);
            }
            .controlValue {
              color: #c6e6ff;
              font-size: var(--font-sm);
            }
            .syncButton {
              border: 1px solid rgba(86, 144, 199, 0.66);
              border-radius: 8px;
              background: rgba(14, 30, 46, 0.88);
              color: #d4ecff;
              font-size: var(--font-xs);
              font-weight: 700;
              letter-spacing: 0.12em;
              text-transform: uppercase;
              min-height: 30px;
              cursor: pointer;
              transition: all 0.12s ease;
            }
            .syncButton:hover {
              border-color: rgba(112, 195, 255, 0.84);
              background: rgba(26, 53, 77, 0.92);
            }
            .syncButton:disabled {
              cursor: default;
              opacity: 0.55;
              border-color: rgba(75, 95, 118, 0.46);
              background: rgba(12, 19, 28, 0.82);
              color: #9fb4c9;
            }
            #metrics {
              overflow: hidden;
              padding: 6px 8px 8px;
              display: grid;
              grid-template-columns: repeat(2, minmax(0, 1fr));
              grid-template-rows: repeat(2, minmax(0, 1fr));
              gap: 6px;
              align-content: stretch;
              min-height: 0;
            }
            .section {
              border: 1px solid rgba(74, 102, 140, 0.32);
              border-radius: 10px;
              margin: 0;
              overflow: hidden;
              background: rgba(7, 12, 20, 0.72);
              display: grid;
              grid-template-rows: auto minmax(0, 1fr);
              min-height: 0;
            }
            .section h3 {
              margin: 0;
              padding: 4px 8px;
              font-size: var(--font-xs);
              letter-spacing: 0.16em;
              text-transform: uppercase;
              color: #8cb8dc;
              border-bottom: 1px solid rgba(74, 102, 140, 0.25);
              background: rgba(12, 21, 34, 0.48);
            }
            .rows {
              display: grid;
              grid-template-columns: 1.06fr 0.94fr;
              min-height: 0;
            }
            .rowLabel, .rowValue {
              font-size: var(--font-sm);
              padding: var(--row-pad-y) var(--row-pad-x);
              border-bottom: 1px solid rgba(67, 89, 119, 0.19);
              line-height: 1.18;
              min-width: 0;
            }
            .rowLabel { color: var(--muted); }
            .rowValue {
              color: #e4f2ff;
              text-align: right;
              word-break: break-word;
              overflow-wrap: anywhere;
            }
            .rowValue.raw {
              color: #ffdca2;
            }
            #threeNotice {
              position: absolute;
              top: 14px;
              right: 14px;
              font-size: var(--font-xs);
              padding: 6px 9px;
              border-radius: 8px;
              color: #ffcf82;
              border: 1px solid rgba(255, 181, 83, 0.44);
              background: rgba(75, 47, 13, 0.35);
              display: none;
              z-index: 4;
            }
            @media (max-height: 820px) {
              #legend {
                grid-template-columns: 1fr;
                max-width: 210px;
                gap: 4px;
              }
              #smoothingControls {
                padding: 6px 10px;
                gap: 6px 8px;
              }
            }
          </style>
          \(threeScriptTag)
        </head>
        <body>
          <div id="root">
            <section id="vizCard" class="card">
              <div class="cardHeader">
                <div class="headerTitle">Head Pose Visualization</div>
                <div id="statusPill">INITIALIZING</div>
              </div>
              <div id="vizWrap">
                <div id="threeNotice">Three.js unavailable, telemetry-only mode.</div>
                <div id="viz"></div>
                <div id="centerReticle" aria-hidden="true"></div>
                <div id="viewControls">
                  <button class="viewBtn active" data-view="perspective" title="Perspective View">P</button>
                  <button class="viewBtn" data-view="top" title="Top View">T</button>
                  <button class="viewBtn" data-view="front" title="Front View">F</button>
                  <button class="viewBtn" data-view="side" title="Side View">S</button>
                </div>
                <div id="orientationOverlay" aria-hidden="true">
                  <div class="orientationChip left">L Ear - Screen Left</div>
                  <div class="orientationChip right">R Ear - Screen Right</div>
                  <div class="orientationHint">Forward points into screen depth</div>
                </div>
                <div id="legend">
                  <div><span class="legendDot" style="background:#7ed7ff"></span>Forward Vector</div>
                  <div><span class="legendDot" style="background:#7af4ae"></span>Up Vector</div>
                  <div><span class="legendDot" style="background:#f4cf64"></span>Velocity Vector</div>
                  <div><span class="legendDot" style="background:#8fd4ff"></span>Acceleration Vector</div>
                  <div><span class="legendDot" style="background:#d08fff"></span>Angular Velocity</div>
                  <div><span class="legendDot" style="background:#ff7272"></span>World Front Cue</div>
                </div>
              </div>
            </section>

            <section id="diagCard" class="card">
              <div class="cardHeader">
                <div class="headerTitle">Telemetry</div>
                <div id="ageIndicator" style="font-size:12px;color:#8fb5d7;">age=0.0 ms</div>
              </div>
              <div id="smoothingControls">
                <div class="controlBlock">
                  <label for="smoothOrientation">Orientation Smoothing</label>
                  <input id="smoothOrientation" type="range" min="0.02" max="0.60" step="0.01" value="0.18" />
                  <div id="smoothOrientationValue" class="controlValue">0.18</div>
                </div>
                <div class="controlBlock">
                  <label for="smoothVectors">Vector/Telemetry Smoothing</label>
                  <input id="smoothVectors" type="range" min="0.02" max="0.65" step="0.01" value="0.22" />
                  <div id="smoothVectorsValue" class="controlValue">0.22</div>
                </div>
                <div class="controlBlock">
                  <label for="syncButton">Center / Sync</label>
                  <button id="syncButton" class="syncButton" type="button">Center / Sync</button>
                  <div id="syncHint" class="controlValue">Waiting for readiness…</div>
                </div>
              </div>
              <div id="metrics">
                <div class="section">
                  <h3>Transport</h3>
                  <div class="rows">
                    <div class="rowLabel">Mode</div><div id="mode" class="rowValue">n/a</div>
                    <div class="rowLabel">Source</div><div id="source" class="rowValue">n/a</div>
                    <div class="rowLabel">Connection</div><div id="connection" class="rowValue">n/a</div>
                    <div class="rowLabel">Destination</div><div id="destination" class="rowValue">n/a</div>
                    <div class="rowLabel">Scheduling Profile</div><div id="schedulingProfile" class="rowValue">n/a</div>
                    <div class="rowLabel">Monitor Refresh</div><div id="monitorHz" class="rowValue">n/a</div>
                    <div class="rowLabel">Rate</div><div id="rateHz" class="rowValue">n/a</div>
                    <div class="rowLabel">Duration</div><div id="durationText" class="rowValue">n/a</div>
                    <div class="rowLabel">Sequence</div><div id="sequence" class="rowValue">n/a</div>
                    <div class="rowLabel">Timestamp</div><div id="timestampMs" class="rowValue">n/a</div>
                    <div class="rowLabel">Packets</div><div id="packetCount" class="rowValue">n/a</div>
                    <div class="rowLabel">Send Errors</div><div id="sendErrors" class="rowValue">n/a</div>
                    <div class="rowLabel">Invalid Samples</div><div id="invalidSamples" class="rowValue">n/a</div>
                    <div class="rowLabel">Effective Rate</div><div id="streamEffectiveRateHz" class="rowValue">n/a</div>
                    <div class="rowLabel">Interval / Jitter</div><div id="streamIntervalJitter" class="rowValue">n/a</div>
                    <div class="rowLabel">Seq Gap</div><div id="streamSeqGap" class="rowValue">n/a</div>
                    <div class="rowLabel">Readiness</div><div id="readinessState" class="rowValue">n/a</div>
                    <div class="rowLabel">Send Gate</div><div id="sendGateOpen" class="rowValue">n/a</div>
                    <div class="rowLabel">Plugin Ingest</div><div id="pluginIngestState" class="rowValue">n/a</div>
                    <div class="rowLabel">Plugin Sources / Consumers</div><div id="pluginIngestCounts" class="rowValue">n/a</div>
                    <div class="rowLabel">Plugin Endpoint</div><div id="pluginIngestEndpoint" class="rowValue">n/a</div>
                    <div class="rowLabel">Plugin Seq / Pose Age</div><div id="pluginIngestSeqAge" class="rowValue">n/a</div>
                    <div class="rowLabel">Plugin Ack Age</div><div id="pluginIngestAckAge" class="rowValue">n/a</div>
                    <div class="rowLabel">Plugin Invalid / Decode</div><div id="pluginIngestInvalidDecode" class="rowValue">n/a</div>
                    <div class="rowLabel">Last Error</div><div id="lastError" class="rowValue">none</div>
                  </div>
                </div>
                <div class="section">
                  <h3>Orientation</h3>
                  <div class="rows">
                    <div class="rowLabel">Yaw/Pitch/Roll Raw</div><div id="yprRaw" class="rowValue raw">n/a</div>
                    <div class="rowLabel">Yaw/Pitch/Roll Smoothed</div><div id="yprSmooth" class="rowValue">n/a</div>
                    <div class="rowLabel">Quaternion Raw (x,y,z,w)</div><div id="quatRaw" class="rowValue raw">n/a</div>
                    <div class="rowLabel">Quaternion Smoothed</div><div id="quatSmooth" class="rowValue">n/a</div>
                    <div class="rowLabel">Frame Mapping</div><div id="frameMapping" class="rowValue">n/a</div>
                    <div class="rowLabel">Heading</div><div id="headingDeg" class="rowValue">n/a</div>
                    <div class="rowLabel">Sensor Location</div><div id="sensorLocation" class="rowValue">n/a</div>
                  </div>
                </div>
                <div class="section">
                  <h3>Motion</h3>
                  <div class="rows">
                    <div class="rowLabel">Rot Rate Raw (deg/s)</div><div id="rotRateRaw" class="rowValue raw">n/a</div>
                    <div class="rowLabel">Rot Rate Smoothed</div><div id="rotRateSmooth" class="rowValue">n/a</div>
                    <div class="rowLabel">Angular Speed</div><div id="angularSpeedDegPerSec" class="rowValue">n/a</div>
                    <div class="rowLabel">Gravity (g)</div><div id="gravityG" class="rowValue">n/a</div>
                    <div class="rowLabel">Accel Raw (g)</div><div id="accelRaw" class="rowValue raw">n/a</div>
                    <div class="rowLabel">Accel Smoothed</div><div id="accelSmooth" class="rowValue">n/a</div>
                    <div class="rowLabel">Velocity Raw (m/s)</div><div id="velocityRaw" class="rowValue raw">n/a</div>
                    <div class="rowLabel">Velocity Smoothed</div><div id="velocitySmooth" class="rowValue">n/a</div>
                    <div class="rowLabel">Displacement (m)</div><div id="displacementEstimateM" class="rowValue">n/a</div>
                    <div class="rowLabel">dt</div><div id="dtSec" class="rowValue">n/a</div>
                    <div class="rowLabel">Motion Norm</div><div id="motionNorm" class="rowValue">n/a</div>
                    <div class="rowLabel">Stability Norm</div><div id="stabilityNorm" class="rowValue">n/a</div>
                  </div>
                </div>
                <div class="section">
                  <h3>Stabilization Config</h3>
                  <div class="rows">
                    <div class="rowLabel">Output Device</div><div id="outputDeviceName" class="rowValue">n/a</div>
                    <div class="rowLabel">Model</div><div id="outputDeviceModel" class="rowValue">n/a</div>
                    <div class="rowLabel">Transport</div><div id="outputDeviceTransport" class="rowValue">n/a</div>
                    <div class="rowLabel">Sample Rate / Ch</div><div id="outputDeviceSampleRateCh" class="rowValue">n/a</div>
                    <div class="rowLabel">Connected</div><div id="outputDeviceConnected" class="rowValue">n/a</div>
                    <div class="rowLabel">Recenter on Start</div><div id="recenterOnStart" class="rowValue">n/a</div>
                    <div class="rowLabel">Baseline State</div><div id="baselineState" class="rowValue">n/a</div>
                    <div class="rowLabel">Alpha</div><div id="stabilizationAlpha" class="rowValue">n/a</div>
                    <div class="rowLabel">Deadband (deg)</div><div id="deadbandDeg" class="rowValue">n/a</div>
                    <div class="rowLabel">Velocity Damping</div><div id="velocityDamping" class="rowValue">n/a</div>
                  </div>
                </div>
              </div>
            </section>
          </div>
          <script>
            (() => {
              const state = {
                snapshot: null,
                smooth: { orientation: 0.18, vectors: 0.22 },
                threeReady: false,
                renderer: null,
                scene: null,
                camera: null,
                cameraTarget: null,
                viewMode: "perspective",
                root: null,
                arrows: {},
                resizeObserver: null,
                filter: {
                  init: false,
                  quaternion: null,
                  ypr: { yaw: 0, pitch: 0, roll: 0 },
                  rotRate: null,
                  accel: null,
                  velocity: null,
                  angularSpeed: 0
                }
              };

              const f = (n, digits = 2) => Number.isFinite(n) ? n.toFixed(digits) : "n/a";
              const fmtVec = (v, digits = 3) => "[" + f(v.x, digits) + ", " + f(v.y, digits) + ", " + f(v.z, digits) + "]";
              const num = (value, fallback = 0) => (typeof value === "number" && Number.isFinite(value)) ? value : fallback;
              const get = (id) => document.getElementById(id);
              const setText = (id, text) => { const el = get(id); if (el) el.textContent = text; };
              const readinessLabel = (value) => {
                if (value === "active_ready") return "active_ready";
                if (value === "active_not_ready") return "active_not_ready";
                return "disabled_disconnected";
              };

              function bindViewControls() {
                const buttons = document.querySelectorAll("#viewControls .viewBtn[data-view]");
                if (!buttons || buttons.length === 0) return;

                const syncButtons = () => {
                  buttons.forEach((button) => {
                    const isActive = button.dataset.view === state.viewMode;
                    button.classList.toggle("active", isActive);
                  });
                };

                buttons.forEach((button) => {
                  button.addEventListener("click", () => {
                    const nextMode = button.dataset.view;
                    if (!nextMode) return;
                    state.viewMode = nextMode;
                    if (typeof state.applyCameraView === "function") {
                      state.applyCameraView();
                    }
                    syncButtons();
                  });
                });

                syncButtons();
              }

              function bindSmoothingControls() {
                const orientation = get("smoothOrientation");
                const vectors = get("smoothVectors");
                const orientationValue = get("smoothOrientationValue");
                const vectorsValue = get("smoothVectorsValue");
                if (!orientation || !vectors) return;

                const update = () => {
                  state.smooth.orientation = num(parseFloat(orientation.value), 0.18);
                  state.smooth.vectors = num(parseFloat(vectors.value), 0.22);
                  if (orientationValue) orientationValue.textContent = f(state.smooth.orientation, 2);
                  if (vectorsValue) vectorsValue.textContent = f(state.smooth.vectors, 2);
                };
                orientation.addEventListener("input", update);
                vectors.addEventListener("input", update);
                update();
              }

              function bindSyncControls() {
                const button = get("syncButton");
                if (!button) return;
                button.addEventListener("click", () => {
                  if (window.webkit
                      && window.webkit.messageHandlers
                      && window.webkit.messageHandlers.locusqControl
                      && typeof window.webkit.messageHandlers.locusqControl.postMessage === "function") {
                    window.webkit.messageHandlers.locusqControl.postMessage({ type: "sync" });
                  }
                });
              }

              function initThreeScene() {
                if (!window.THREE) {
                  const notice = get("threeNotice");
                  if (notice) notice.style.display = "block";
                  return;
                }

                const THREE = window.THREE;
                const container = get("viz");
                if (!container) return;

                state.scene = new THREE.Scene();
                state.scene.fog = new THREE.Fog(0x05070c, 6.0, 14.0);
                state.camera = new THREE.PerspectiveCamera(44, 1, 0.01, 80);
                state.cameraTarget = new THREE.Vector3(0, 0, 0);
                state.camera.position.set(0.0, 1.08, 2.92);
                state.camera.lookAt(state.cameraTarget);

                state.applyCameraView = () => {
                  if (!state.camera) return;
                  const target = state.cameraTarget || new THREE.Vector3(0, 0, 0);
                  const containerWidth = Math.max(120, container.clientWidth || 120);
                  const containerHeight = Math.max(120, container.clientHeight || 120);
                  const aspect = Math.max(0.6, Math.min(2.6, containerWidth / containerHeight));
                  const halfFovRad = (state.camera.fov * Math.PI / 180) * 0.5;
                  const fitRadius = 1.52;
                  const fitDistance = (fitRadius / Math.tan(halfFovRad)) * (aspect < 1 ? (1 / aspect) : 1);
                  const distance = Math.max(2.25, Math.min(4.3, fitDistance * 0.84));

                  if (state.viewMode === "top") {
                    state.camera.up.set(0, 0, -1);
                    state.camera.position.set(0.0, distance, 0.001);
                  } else if (state.viewMode === "front") {
                    state.camera.up.set(0, 1, 0);
                    state.camera.position.set(0.0, 0.0, distance);
                  } else if (state.viewMode === "side") {
                    state.camera.up.set(0, 1, 0);
                    state.camera.position.set(distance, 0.0, 0.0);
                  } else {
                    state.camera.up.set(0, 1, 0);
                    const elevation = Math.max(0.85, Math.min(1.32, 1.00 + (1.0 / aspect - 1.0) * 0.18));
                    state.camera.position.set(0, elevation, distance);
                  }
                  state.camera.lookAt(target);
                };

                state.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
                state.renderer.setPixelRatio(window.devicePixelRatio || 1);
                state.renderer.setClearColor(0x000000, 0);
                state.renderer.domElement.style.display = "block";
                state.renderer.domElement.style.width = "100%";
                state.renderer.domElement.style.height = "100%";
                container.appendChild(state.renderer.domElement);

                const hemi = new THREE.HemisphereLight(0x98d3ff, 0x131c2a, 1.02);
                state.scene.add(hemi);
                const key = new THREE.DirectionalLight(0xb9e7ff, 1.0);
                key.position.set(1.8, 2.6, 2.5);
                state.scene.add(key);
                const fill = new THREE.DirectionalLight(0x6fa3ff, 0.38);
                fill.position.set(-1.4, 0.8, -1.5);
                state.scene.add(fill);

                const grid = new THREE.GridHelper(6.6, 44, 0x234058, 0x13283d);
                grid.position.y = -0.34;
                state.scene.add(grid);

                const centerRing = new THREE.Mesh(
                  new THREE.TorusGeometry(0.34, 0.009, 12, 48),
                  new THREE.MeshBasicMaterial({ color: 0x2f83c7, transparent: true, opacity: 0.52 })
                );
                centerRing.rotation.x = Math.PI * 0.5;
                centerRing.position.set(0, -0.31, 0);
                state.scene.add(centerRing);

                const axes = new THREE.AxesHelper(0.66);
                axes.position.set(0, -0.02, 0);
                state.scene.add(axes);

                const root = new THREE.Group();
                root.position.set(0, 0, 0);
                state.root = root;
                state.scene.add(root);

                const head = new THREE.Mesh(
                  new THREE.SphereGeometry(0.28, 42, 32),
                  new THREE.MeshStandardMaterial({
                    color: 0x95c7ff,
                    roughness: 0.34,
                    metalness: 0.16
                  })
                );
                root.add(head);

                const nose = new THREE.Mesh(
                  new THREE.ConeGeometry(0.042, 0.12, 20),
                  new THREE.MeshStandardMaterial({ color: 0x4dd9ff, emissive: 0x0d2b3c })
                );
                nose.position.set(0, 0.0, -0.30);
                nose.rotation.x = Math.PI * 0.5;
                root.add(nose);

                const leftPod = new THREE.Mesh(
                  new THREE.SphereGeometry(0.058, 24, 20),
                  new THREE.MeshStandardMaterial({ color: 0x4fc3ff, emissive: 0x184268, emissiveIntensity: 1.05 })
                );
                leftPod.position.set(-0.35, 0.01, 0.02);
                root.add(leftPod);

                const rightPod = new THREE.Mesh(
                  new THREE.SphereGeometry(0.058, 24, 20),
                  new THREE.MeshStandardMaterial({ color: 0xff8faa, emissive: 0x4f1f30, emissiveIntensity: 1.05 })
                );
                rightPod.position.set(0.35, 0.01, 0.02);
                root.add(rightPod);

                function createPodLabelSprite(text, colorHex, x) {
                  const canvas = document.createElement("canvas");
                  canvas.width = 128;
                  canvas.height = 64;
                  const ctx = canvas.getContext("2d");
                  if (!ctx) return null;
                  ctx.clearRect(0, 0, canvas.width, canvas.height);
                  ctx.fillStyle = "rgba(6, 14, 24, 0.78)";
                  ctx.strokeStyle = "rgba(90, 150, 210, 0.5)";
                  ctx.lineWidth = 2;
                  const rx = 8;
                  const ry = 12;
                  const rw = canvas.width - 16;
                  const rh = canvas.height - 24;
                  ctx.beginPath();
                  ctx.moveTo(rx + 8, ry);
                  ctx.lineTo(rx + rw - 8, ry);
                  ctx.quadraticCurveTo(rx + rw, ry, rx + rw, ry + 8);
                  ctx.lineTo(rx + rw, ry + rh - 8);
                  ctx.quadraticCurveTo(rx + rw, ry + rh, rx + rw - 8, ry + rh);
                  ctx.lineTo(rx + 8, ry + rh);
                  ctx.quadraticCurveTo(rx, ry + rh, rx, ry + rh - 8);
                  ctx.lineTo(rx, ry + 8);
                  ctx.quadraticCurveTo(rx, ry, rx + 8, ry);
                  ctx.closePath();
                  ctx.fill();
                  ctx.stroke();
                  ctx.fillStyle = colorHex;
                  ctx.font = "700 30px Menlo, Monaco, monospace";
                  ctx.textAlign = "center";
                  ctx.textBaseline = "middle";
                  ctx.fillText(text, canvas.width * 0.5, canvas.height * 0.52);
                  const texture = new THREE.CanvasTexture(canvas);
                  texture.needsUpdate = true;
                  texture.minFilter = THREE.LinearFilter;
                  const material = new THREE.SpriteMaterial({
                    map: texture,
                    transparent: true,
                    depthWrite: false,
                    depthTest: false
                  });
                  const sprite = new THREE.Sprite(material);
                  sprite.scale.set(0.24, 0.12, 1);
                  sprite.position.set(x, 0.16, 0.02);
                  return sprite;
                }

                const leftLabel = createPodLabelSprite("L", "#8ED8FF", -0.47);
                const rightLabel = createPodLabelSprite("R", "#FFB4C6", 0.47);
                if (leftLabel) root.add(leftLabel);
                if (rightLabel) root.add(rightLabel);

                const frontCue = new THREE.Mesh(
                  new THREE.ConeGeometry(0.07, 0.22, 18),
                  new THREE.MeshStandardMaterial({ color: 0xff7a7a, emissive: 0x471919 })
                );
                frontCue.position.set(0, 0.0, -1.22);
                frontCue.rotation.x = Math.PI * 0.5;
                state.scene.add(frontCue);

                const forward = new THREE.ArrowHelper(new THREE.Vector3(0, 0, -1), new THREE.Vector3(0, 0, 0), 0.95, 0x7ed7ff, 0.11, 0.07);
                const up = new THREE.ArrowHelper(new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0, 0), 0.72, 0x7af4ae, 0.1, 0.06);
                const right = new THREE.ArrowHelper(new THREE.Vector3(1, 0, 0), new THREE.Vector3(0, 0, 0), 0.72, 0xf8c66f, 0.1, 0.06);
                const angular = new THREE.ArrowHelper(new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0, 0), 0.1, 0xd08fff, 0.09, 0.05);
                const velocity = new THREE.ArrowHelper(new THREE.Vector3(0, 0, 1), new THREE.Vector3(0, 0, 0), 0.1, 0xf4cf64, 0.09, 0.05);
                const accel = new THREE.ArrowHelper(new THREE.Vector3(0, 1, 1), new THREE.Vector3(0, 0, 0), 0.1, 0x8fd4ff, 0.09, 0.05);
                state.scene.add(forward, up, right, angular, velocity, accel);

                state.arrows = { forward, up, right, angular, velocity, accel };
                state.filter.quaternion = new THREE.Quaternion(0, 0, 0, 1);
                state.filter.rotRate = new THREE.Vector3();
                state.filter.accel = new THREE.Vector3();
                state.filter.velocity = new THREE.Vector3();

                const resize = () => {
                  const width = Math.max(120, container.clientWidth);
                  const height = Math.max(120, container.clientHeight);
                  state.camera.aspect = width / height;
                  state.camera.updateProjectionMatrix();
                  state.renderer.setSize(width, height, true);
                  state.renderer.setViewport(0, 0, width, height);
                  if (typeof state.applyCameraView === "function") {
                    state.applyCameraView();
                  }
                };
                window.addEventListener("resize", resize);
                if (window.ResizeObserver) {
                  state.resizeObserver = new ResizeObserver(resize);
                  state.resizeObserver.observe(container);
                }
                resize();

                const animate = () => {
                  if (state.renderer && state.scene && state.camera) {
                    state.renderer.render(state.scene, state.camera);
                  }
                  window.requestAnimationFrame(animate);
                };
                window.requestAnimationFrame(animate);
                state.threeReady = true;
              }

              function setArrow(arrow, direction, length) {
                if (!arrow) return;
                const magnitude = direction.length();
                if (!Number.isFinite(magnitude) || magnitude < 0.0001 || length < 0.03) {
                  arrow.visible = false;
                  return;
                }
                arrow.visible = true;
                const dir = direction.clone().normalize();
                arrow.setDirection(dir);
                arrow.setLength(Math.min(1.35, Math.max(0.06, length)), 0.10, 0.06);
              }

              function updateStatus(snapshot) {
                const pill = get("statusPill");
                if (!pill) return;

                const ageMs = num(snapshot.ageMs, 0);
                const hasErrors = num(snapshot.sendErrors, 0) > 0 || num(snapshot.invalidSamples, 0) > 0;
                const readiness = readinessLabel(snapshot.readinessState);
                const sendGateOpen = !!snapshot.sendGateOpen;
                const syncRequired = !!snapshot.syncRequired;

                pill.className = "";
                if (hasErrors) {
                  pill.classList.add("error");
                  pill.textContent = "ERROR";
                } else if (readiness === "disabled_disconnected") {
                  pill.classList.add("warn");
                  pill.textContent = "DISCONNECTED";
                } else if (readiness !== "active_ready") {
                  pill.classList.add("warn");
                  pill.textContent = "NOT READY";
                } else if (syncRequired && !sendGateOpen) {
                  pill.classList.add("warn");
                  pill.textContent = "SYNC NEEDED";
                } else if (ageMs > 120.0) {
                  pill.classList.add("warn");
                  pill.textContent = "STALE";
                } else {
                  pill.classList.add("active");
                  pill.textContent = "ACTIVE";
                }
                setText("ageIndicator", "age=" + f(ageMs, 1) + " ms");
              }

              function updateMetrics(snapshot) {
                setText("mode", snapshot.mode || "n/a");
                setText("source", snapshot.source || "n/a");
                setText("connection", snapshot.connection || "n/a");
                setText("destination", snapshot.destination || "n/a");
                setText("schedulingProfile", snapshot.schedulingProfile || "n/a");
                setText("monitorHz", f(num(snapshot.monitorHz, 0), 0) + " Hz");
                setText("rateHz", f(num(snapshot.rateHz, 0), 0) + " Hz");
                setText("durationText", snapshot.durationText || "n/a");
                setText("sequence", String(snapshot.sequence ?? "n/a"));
                setText("timestampMs", String(snapshot.timestampMs ?? "n/a") + " ms");
                setText("packetCount", String(snapshot.packetCount ?? "0"));
                setText("sendErrors", String(snapshot.sendErrors ?? "0"));
                setText("invalidSamples", String(snapshot.invalidSamples ?? "0"));
                setText("lastError", (snapshot.lastError && String(snapshot.lastError).length > 0) ? snapshot.lastError : "none");

                const q = snapshot.quaternion || {};
                const ypr = snapshot.yawPitchRollDeg || {};
                const rot = snapshot.rotationRateDegPerSec || {};
                const grav = snapshot.gravityG || {};
                const acc = snapshot.userAccelerationG || {};
                const vel = snapshot.velocityEstimateMps || {};
                const disp = snapshot.displacementEstimateM || {};
                const stream = snapshot.streamHealth || {};
                const plugin = snapshot.pluginIngest || {};
                const outputDevice = snapshot.outputDevice || {};
                const controls = snapshot.controls || {};
                const readiness = readinessLabel(snapshot.readinessState);
                const sendGateOpen = !!snapshot.sendGateOpen;
                const syncRequired = !!snapshot.syncRequired;
                const syncButton = get("syncButton");
                const syncHint = get("syncHint");

                setText("quatRaw", "[" + f(num(q.x), 4) + ", " + f(num(q.y), 4) + ", " + f(num(q.z), 4) + ", " + f(num(q.w, 1), 4) + "]");
                setText("yprRaw", f(num(ypr.yaw), 2) + " / " + f(num(ypr.pitch), 2) + " / " + f(num(ypr.roll), 2) + " deg");
                setText("frameMapping", snapshot.frameMapping || "n/a");
                setText("rotRateRaw", f(num(rot.x), 2) + " / " + f(num(rot.y), 2) + " / " + f(num(rot.z), 2));
                setText("angularSpeedDegPerSec", f(num(snapshot.angularSpeedDegPerSec), 2) + " deg/s");
                setText("gravityG", "[" + f(num(grav.x), 3) + ", " + f(num(grav.y), 3) + ", " + f(num(grav.z), 3) + "]");
                setText("accelRaw", "[" + f(num(acc.x), 3) + ", " + f(num(acc.y), 3) + ", " + f(num(acc.z), 3) + "]");
                setText("velocityRaw", "[" + f(num(vel.x), 3) + ", " + f(num(vel.y), 3) + ", " + f(num(vel.z), 3) + "]");
                setText("displacementEstimateM", "[" + f(num(disp.x), 3) + ", " + f(num(disp.y), 3) + ", " + f(num(disp.z), 3) + "]");
                setText("headingDeg", (snapshot.headingDeg == null) ? "n/a" : f(num(snapshot.headingDeg), 2) + " deg");
                setText("sensorLocation", snapshot.sensorLocation || "n/a");
                setText("dtSec", f(num(snapshot.dtSec), 4) + " s");
                setText("motionNorm", f(num(snapshot.motionNorm), 3));
                setText("stabilityNorm", f(num(snapshot.stabilityNorm), 3));
                setText("streamEffectiveRateHz", f(num(stream.effectiveRateHz), 1) + " Hz");
                setText("streamIntervalJitter", f(num(stream.intervalMs), 1) + " ms / " + f(num(stream.jitterMs), 2) + " ms");
                setText("streamSeqGap", String(stream.seqGap ?? "0"));
                setText("readinessState", readiness);
                setText("sendGateOpen", sendGateOpen ? "open" : "closed");
                setText("pluginIngestState", plugin.state || "n/a");
                setText("pluginIngestCounts", String(plugin.sourceCount ?? "0") + " / " + String(plugin.consumerCount ?? "0"));
                setText("pluginIngestEndpoint", plugin.endpoint || "n/a");
                setText("pluginIngestSeqAge", String(plugin.sequence ?? "0") + " / " + f(num(plugin.poseAgeMs), 1) + " ms");
                setText("pluginIngestAckAge", f(num(plugin.ackAgeMs), 1) + " ms");
                setText("pluginIngestInvalidDecode", String(plugin.invalidPackets ?? "0") + " / " + String(plugin.decodeErrors ?? "0"));
                setText("outputDeviceName", outputDevice.name || "n/a");
                setText("outputDeviceModel", outputDevice.model || "n/a");
                setText("outputDeviceTransport", outputDevice.transport || "n/a");
                setText("outputDeviceSampleRateCh", f(num(outputDevice.sampleRateHz), 1) + " Hz / " + String(outputDevice.channels ?? "0"));
                setText("outputDeviceConnected", outputDevice.connected ? "true" : "false");
                setText("recenterOnStart", controls.recenterOnStart ? "true" : "false");
                setText("baselineState", snapshot.baselineState || "n/a");
                setText("stabilizationAlpha", f(num(controls.stabilizationAlpha), 3));
                setText("deadbandDeg", f(num(controls.deadbandDeg), 3));
                setText("velocityDamping", f(num(controls.velocityDamping), 3));

                if (syncButton) {
                  syncButton.disabled = readiness !== "active_ready";
                }
                if (syncHint) {
                  if (readiness !== "active_ready") {
                    syncHint.textContent = "Waiting for in-ear ready state";
                  } else if (syncRequired && !sendGateOpen) {
                    syncHint.textContent = "Ready. Press Center / Sync to open send gate";
                  } else if (sendGateOpen) {
                    syncHint.textContent = "Synced. Streaming pose to plugin";
                  } else {
                    syncHint.textContent = "Ready";
                  }
                }
              }

              function applySmoothing(snapshot) {
                if (!window.THREE) return;
                const THREE = window.THREE;
                const qRaw = snapshot.quaternion || {};
                const yprRaw = snapshot.yawPitchRollDeg || {};
                const rotRaw = snapshot.rotationRateDegPerSec || {};
                const accRaw = snapshot.userAccelerationG || {};
                const velRaw = snapshot.velocityEstimateMps || {};

                const targetQ = new THREE.Quaternion(num(qRaw.x), num(qRaw.y), num(qRaw.z), num(qRaw.w, 1)).normalize();
                const alphaQ = num(state.smooth.orientation, 0.18);
                const alphaV = num(state.smooth.vectors, 0.22);

                if (!state.filter.init) {
                  state.filter.quaternion.copy(targetQ);
                  state.filter.ypr.yaw = num(yprRaw.yaw);
                  state.filter.ypr.pitch = num(yprRaw.pitch);
                  state.filter.ypr.roll = num(yprRaw.roll);
                  state.filter.rotRate.set(num(rotRaw.x), num(rotRaw.y), num(rotRaw.z));
                  state.filter.accel.set(num(accRaw.x), num(accRaw.y), num(accRaw.z));
                  state.filter.velocity.set(num(velRaw.x), num(velRaw.y), num(velRaw.z));
                  state.filter.angularSpeed = num(snapshot.angularSpeedDegPerSec);
                  state.filter.init = true;
                } else {
                  state.filter.quaternion.slerp(targetQ, alphaQ);
                  state.filter.ypr.yaw += (num(yprRaw.yaw) - state.filter.ypr.yaw) * alphaQ;
                  state.filter.ypr.pitch += (num(yprRaw.pitch) - state.filter.ypr.pitch) * alphaQ;
                  state.filter.ypr.roll += (num(yprRaw.roll) - state.filter.ypr.roll) * alphaQ;
                  state.filter.rotRate.lerp(new THREE.Vector3(num(rotRaw.x), num(rotRaw.y), num(rotRaw.z)), alphaV);
                  state.filter.accel.lerp(new THREE.Vector3(num(accRaw.x), num(accRaw.y), num(accRaw.z)), alphaV);
                  state.filter.velocity.lerp(new THREE.Vector3(num(velRaw.x), num(velRaw.y), num(velRaw.z)), alphaV);
                  state.filter.angularSpeed += (num(snapshot.angularSpeedDegPerSec) - state.filter.angularSpeed) * alphaV;
                }

                setText("quatSmooth", "[" + f(state.filter.quaternion.x, 4) + ", " + f(state.filter.quaternion.y, 4) + ", " + f(state.filter.quaternion.z, 4) + ", " + f(state.filter.quaternion.w, 4) + "]");
                setText("yprSmooth", f(state.filter.ypr.yaw, 2) + " / " + f(state.filter.ypr.pitch, 2) + " / " + f(state.filter.ypr.roll, 2) + " deg");
                setText("rotRateSmooth", f(state.filter.rotRate.x, 2) + " / " + f(state.filter.rotRate.y, 2) + " / " + f(state.filter.rotRate.z, 2));
                setText("accelSmooth", fmtVec(state.filter.accel, 3));
                setText("velocitySmooth", fmtVec(state.filter.velocity, 3));
              }

              function updateThree(snapshot) {
                if (!state.threeReady || !window.THREE) return;
                const THREE = window.THREE;
                if (!state.filter.init || !state.root) return;

                // Screen-forward contract:
                // +X = screen right, +Y = screen up, -Z = into the screen depth.
                // Keep identity basis so L pod stays visually on screen-left and R on screen-right.
                const displayQ = state.filter.quaternion.clone();
                state.root.quaternion.copy(displayQ);

                const forward = new THREE.Vector3(0, 0, -1).applyQuaternion(displayQ);
                const up = new THREE.Vector3(0, 1, 0).applyQuaternion(displayQ);
                const right = new THREE.Vector3(1, 0, 0).applyQuaternion(displayQ);

                setArrow(state.arrows.forward, forward, 0.95);
                setArrow(state.arrows.up, up, 0.78);
                setArrow(state.arrows.right, right, 0.78);
                setArrow(state.arrows.angular, state.filter.rotRate.clone(), state.filter.rotRate.length() * 0.008);
                setArrow(state.arrows.velocity, state.filter.velocity.clone(), state.filter.velocity.length() * 0.45);
                setArrow(state.arrows.accel, state.filter.accel.clone(), state.filter.accel.length() * 0.7);
              }

              window.__companionUpdate = function(snapshot) {
                if (!snapshot || typeof snapshot !== "object") return;
                state.snapshot = snapshot;
                updateStatus(snapshot);
                updateMetrics(snapshot);
                applySmoothing(snapshot);
                updateThree(snapshot);
              };

              bindSmoothingControls();
              bindSyncControls();
              bindViewControls();
              initThreeScene();
            })();
          </script>
        </body>
        </html>
        """
    }
}
#else
@MainActor
private final class CompanionMonitorWindow: NSObject, NSWindowDelegate {
    private let window: NSWindow
    private let textView: NSTextView

    init() {
        let frame = NSRect(x: 100, y: 100, width: 640, height: 780)
        window = NSWindow(
            contentRect: frame,
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = "LocusQ Companion Monitor"
        window.minSize = NSSize(width: 560, height: 560)
        window.center()

        let scrollView = NSScrollView(frame: frame)
        scrollView.hasVerticalScroller = true
        scrollView.hasHorizontalScroller = false
        scrollView.autoresizingMask = [.width, .height]

        textView = NSTextView(frame: frame)
        textView.isEditable = false
        textView.isSelectable = true
        textView.usesAdaptiveColorMappingForDarkAppearance = true
        textView.font = NSFont.monospacedSystemFont(ofSize: 12.5, weight: .regular)
        scrollView.documentView = textView

        super.init()
        window.delegate = self

        if let contentView = window.contentView {
            scrollView.frame = contentView.bounds
            contentView.addSubview(scrollView)
        }

        textView.string = "WebKit unavailable. Running telemetry-only monitor fallback."
    }

    func show() {
        let app = NSApplication.shared
        app.setActivationPolicy(.regular)
        window.makeKeyAndOrderFront(nil)
        app.activate(ignoringOtherApps: true)
    }

    func windowWillClose(_ notification: Notification) {
        _ = notification
        markStopRequested()
    }

    func render(snapshot: RuntimeSnapshot, args: CompanionArguments) {
        let headingText = snapshot.headingDeg.map { String(format: "%.2f deg", $0) } ?? "n/a"
        textView.string = String(
            format:
            """
            LocusQ Head-Tracking Companion (Telemetry fallback)

            mode=%@
            source=%@
            connection=%@
            destination=%@:%u
            scheduling_profile=%@ monitor=%dHz
            rate=%dHz duration=%@
            seq=%u timestamp=%llu age=%.2fms
            q=[%.6f, %.6f, %.6f, %.6f]
            ypr=[%.2f, %.2f, %.2f] deg
            rot=[%.2f, %.2f, %.2f] deg/s angular=%.2f deg/s
            accel=[%.3f, %.3f, %.3f]g velocity=[%.3f, %.3f, %.3f]m/s
            displacement=[%.3f, %.3f, %.3f]m heading=%@
            packet_count=%d send_errors=%d invalid=%d
            stream=[rate=%.1fHz interval=%.2fms jitter=%.2fms gap=%d]
            plugin_ingest=[state=%@ sources=%d consumers=%d endpoint=%@ seq=%u pose_age=%.1fms ack_age=%.1fms invalid=%u decode=%d]
            output_device=[name=%@ model=%@ transport=%@ sample_rate=%.1fHz channels=%d connected=%@]
            """,
            snapshot.mode.rawValue,
            snapshot.source,
            snapshot.connection,
            args.host,
            args.port,
            snapshot.schedulingProfile,
            snapshot.monitorHz,
            args.hz,
            args.seconds == 0 ? "until signal" : "\(args.seconds)s",
            snapshot.sequence,
            snapshot.timestampMs,
            snapshot.ageMs,
            snapshot.qx,
            snapshot.qy,
            snapshot.qz,
            snapshot.qw,
            snapshot.yawDeg,
            snapshot.pitchDeg,
            snapshot.rollDeg,
            snapshot.rotRateXDeg,
            snapshot.rotRateYDeg,
            snapshot.rotRateZDeg,
            snapshot.angularSpeedDegPerSec,
            snapshot.userAccelerationG.x,
            snapshot.userAccelerationG.y,
            snapshot.userAccelerationG.z,
            snapshot.velocityEstimateMps.x,
            snapshot.velocityEstimateMps.y,
            snapshot.velocityEstimateMps.z,
            snapshot.displacementEstimateM.x,
            snapshot.displacementEstimateM.y,
            snapshot.displacementEstimateM.z,
            headingText,
            snapshot.packetCount,
            snapshot.sendErrors,
            snapshot.invalidSamples,
            snapshot.streamHealth.effectiveRateHz,
            snapshot.streamHealth.intervalMs,
            snapshot.streamHealth.jitterMs,
            snapshot.streamHealth.seqGap,
            snapshot.pluginIngest.state,
            snapshot.pluginIngest.sourceCount,
            snapshot.pluginIngest.consumerCount,
            snapshot.pluginIngest.endpoint,
            snapshot.pluginIngest.sequence,
            snapshot.pluginIngest.poseAgeMs,
            snapshot.pluginIngest.ackAgeMs,
            snapshot.pluginIngest.invalidPackets,
            snapshot.pluginIngest.decodeErrors,
            snapshot.outputDevice.name,
            snapshot.outputDevice.model,
            snapshot.outputDevice.transport,
            snapshot.outputDevice.sampleRateHz,
            snapshot.outputDevice.channels,
            snapshot.outputDevice.connected ? "true" : "false"
        )
    }
}
#endif
#else
private final class CompanionMonitorWindow {
    init() {}
    func show() {}
    func render(snapshot: RuntimeSnapshot, args: CompanionArguments) {
        _ = snapshot
        _ = args
    }
}
#endif

@inline(__always)
private func withMainActorSync(_ body: @MainActor () -> Void) {
    if Thread.isMainThread {
        MainActor.assumeIsolated {
            body()
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            body()
        }
    }
}

@inline(__always)
private func withMainActorSyncThrowing<T: Sendable>(_ body: @MainActor () throws -> T) throws -> T {
    if Thread.isMainThread {
        return try MainActor.assumeIsolated {
            try body()
        }
    }

    var result: Result<T, Error>?
    DispatchQueue.main.sync {
        result = Result {
            try MainActor.assumeIsolated {
                try body()
            }
        }
    }
    return try result!.get()
}

private func createMonitorWindowOnMain() -> CompanionMonitorWindow {
    if Thread.isMainThread {
        return MainActor.assumeIsolated { CompanionMonitorWindow() }
    }
    return DispatchQueue.main.sync {
        MainActor.assumeIsolated { CompanionMonitorWindow() }
    }
}

private func nowEpochMilliseconds() -> UInt64 {
    UInt64((Date().timeIntervalSince1970 * 1000.0).rounded())
}

private func seededSequenceStart() -> UInt32 {
    // Seed from epoch milliseconds so relaunches do not restart from 1 and get
    // rejected by monotonic-sequence receivers in already-running plugin hosts.
    var seed = UInt32(truncatingIfNeeded: nowEpochMilliseconds())
    if seed == 0 {
        seed = 1
    }
    return seed
}

private func mergePluginAndDeviceDiagnostics(store: SnapshotStore,
                                             pluginAckReceiver: PluginAckReceiver?,
                                             cachedOutputDevice: inout OutputDeviceSnapshot,
                                             nextOutputDevicePollAtMs: inout UInt64) {
    let nowMs = nowEpochMilliseconds()
    if nowMs >= nextOutputDevicePollAtMs {
        cachedOutputDevice = readDefaultOutputDeviceSnapshot()
        nextOutputDevicePollAtMs = nowMs + 1_000
    }

    let pluginIngest = pluginAckReceiver?.snapshot(nowMs: nowMs) ?? PluginIngestSnapshot(state: "disabled")
    store.update { snapshot in
        snapshot.pluginIngest = pluginIngest
        snapshot.outputDevice = cachedOutputDevice
    }
}

private func clamp01(_ value: Double) -> Double {
    max(0.0, min(1.0, value))
}

private func quaternionFromYawPitchRoll(yawDeg: Float, pitchDeg: Float, rollDeg: Float) -> Quaternion {
    let radians = Float.pi / 180.0
    let yaw = yawDeg * radians * 0.5
    let pitch = pitchDeg * radians * 0.5
    let roll = rollDeg * radians * 0.5

    let cy = cosf(yaw)
    let sy = sinf(yaw)
    let cp = cosf(pitch)
    let sp = sinf(pitch)
    let cr = cosf(roll)
    let sr = sinf(roll)

    let q = Quaternion(
        x: sr * cp * cy - cr * sp * sy,
        y: cr * sp * cy + sr * cp * sy,
        z: cr * cp * sy - sr * sp * cy,
        w: cr * cp * cy + sr * sp * sy
    )
    return q.normalized()
}

private func runSynthetic(arguments: CompanionArguments,
                          sender: UDPSender,
                          store: SnapshotStore,
                          monitor: CompanionMonitorWindow?,
                          pluginAckReceiver: PluginAckReceiver?) throws {
    let packetCount = (arguments.seconds == 0)
        ? Int.max
        : max(1, arguments.hz * arguments.seconds)
    let intervalSeconds = 1.0 / Double(arguments.hz)
    let microsPerTick = useconds_t(max(1.0, intervalSeconds * 1_000_000.0))

    print(
        "companion_start mode=synthetic host=\(arguments.host) port=\(arguments.port) hz=\(arguments.hz) seconds=\(arguments.seconds) retries=\(ReliabilityConfig.maxSendRetries)"
    )

    var seq: UInt32 = seededSequenceStart()
    var sentPackets = 0
    var stopReason = "duration_complete"
    var cachedOutputDevice = readDefaultOutputDeviceSnapshot()
    var nextOutputDevicePollAtMs = nowEpochMilliseconds()

    for i in 0..<packetCount {
        if stopRequested() {
            stopReason = "signal_stop"
            break
        }

        let t = Float(Double(i) * intervalSeconds)
        let yaw = arguments.yawAmplitudeDeg * sinf(2.0 * Float.pi * arguments.yawFrequencyHz * t)
        let pitch = arguments.pitchAmplitudeDeg * sinf(2.0 * Float.pi * (arguments.yawFrequencyHz * 0.5) * t)
        let roll = arguments.rollAmplitudeDeg * cosf(2.0 * Float.pi * (arguments.yawFrequencyHz * 0.25) * t)
        let quat = quaternionFromYawPitchRoll(yawDeg: yaw, pitchDeg: pitch, rollDeg: roll)
        let timestampMs = nowEpochMilliseconds()

        let packet = PosePacketV1(
            quaternion: quat,
            timestampMs: timestampMs,
            sequence: seq
        )

        let payload = packet.encodedData()
        precondition(payload.count == 40, "Pose packet v1 must be 40 bytes")
        try sender.send(payload)
        sentPackets += 1

        let motionNorm = clamp01(Double(abs(yaw)) / 90.0)
        let stability = clamp01(1.0 - motionNorm * 0.35)

        store.update { snapshot in
            snapshot.mode = .synthetic
            snapshot.source = "synthetic_generator"
            snapshot.connection = "simulated"
            snapshot.frameMapping = "synthetic_steam_basis_identity"
            snapshot.baselineState = arguments.recenterOnStart ? "synthetic_not_applicable" : "disabled"
            snapshot.readinessState = "active_ready"
            snapshot.sendGateOpen = true
            snapshot.syncRequired = arguments.requireSyncToStart
            snapshot.sequence = seq
            snapshot.timestampMs = timestampMs
            snapshot.ageMs = 0.0
            snapshot.qx = quat.x
            snapshot.qy = quat.y
            snapshot.qz = quat.z
            snapshot.qw = quat.w
            snapshot.yawDeg = yaw
            snapshot.pitchDeg = pitch
            snapshot.rollDeg = roll
            snapshot.rotRateXDeg = 0.0
            snapshot.rotRateYDeg = 0.0
            snapshot.rotRateZDeg = 0.0
            snapshot.angularSpeedDegPerSec = 0.0
            snapshot.gravity = Vector3(x: 0.0, y: 1.0, z: 0.0)
            snapshot.userAccelerationG = .zero
            snapshot.velocityEstimateMps = .zero
            snapshot.displacementEstimateM = .zero
            snapshot.dtSec = intervalSeconds
            snapshot.headingDeg = nil
            snapshot.sensorLocation = "synthetic"
            snapshot.motionNorm = motionNorm
            snapshot.stabilityNorm = stability
            snapshot.packetCount = sentPackets
            snapshot.sendErrors = 0
            snapshot.invalidSamples = 0
            snapshot.lastError = ""
            snapshot.streamHealth.effectiveRateHz = Double(arguments.hz)
            snapshot.streamHealth.intervalMs = intervalSeconds * 1000.0
            snapshot.streamHealth.jitterMs = 0.0
            snapshot.streamHealth.seqGap = 1
        }

        mergePluginAndDeviceDiagnostics(
            store: store,
            pluginAckReceiver: pluginAckReceiver,
            cachedOutputDevice: &cachedOutputDevice,
            nextOutputDevicePollAtMs: &nextOutputDevicePollAtMs
        )

        if arguments.verbose {
            print(
                String(
                    format: "packet seq=%u ts_ms=%llu q=[%.6f,%.6f,%.6f,%.6f] ypr=[%.2f,%.2f,%.2f] bytes=%d",
                    seq,
                    timestampMs,
                    quat.x,
                    quat.y,
                    quat.z,
                    quat.w,
                    yaw,
                    pitch,
                    roll,
                    payload.count
                )
            )
        }

        seq &+= 1

        if let monitor {
            let snapshot = store.read()
            withMainActorSync {
                monitor.render(snapshot: snapshot, args: arguments)
            }
        }

        usleep(microsPerTick)
    }

    if stopReason == "signal_stop" {
        print("companion_shutdown reason=signal packets_sent=\(sentPackets) requested_packets=\(packetCount)")
    }
    print("companion_done mode=synthetic packets_sent=\(sentPackets) requested_packets=\(packetCount) reason=\(stopReason)")
}

#if canImport(CoreMotion)
private final class LiveRuntimeProcessor: @unchecked Sendable {
    private let arguments: CompanionArguments
    private let sender: UDPSender
    private let store: SnapshotStore

    private var sequence: UInt32 = seededSequenceStart()
    private var lastSentMotionTimestamp: TimeInterval = -.greatestFiniteMagnitude
    private var baselineInverse: Quaternion?
    private var baselineCapturePending = true
    private var baselineStableSampleCount = 0
    private var baselineSensorLocation = "unknown"
    private var sendGateOpen = false
    private var smoothedQuaternion = Quaternion.identity
    private var hasSmoothedQuaternion = false

    private var previousMotionTimestamp: TimeInterval?
    private var velocityEstimateMps = Vector3.zero
    private var displacementEstimateM = Vector3.zero
    private var lastSentEpochMs: UInt64 = 0
    private var intervalMsEma: Double = 0.0
    private var jitterMsEma: Double = 0.0
    private var previousSentSeq: UInt32 = 0
    private var latestSeqGap: Int = 1

    private(set) var packetCount = 0
    private(set) var sendErrors = 0
    private(set) var invalidSamples = 0
    private var lastError = ""

    private var connectionState = "awaiting_device"

    init(arguments: CompanionArguments, sender: UDPSender, store: SnapshotStore) {
        self.arguments = arguments
        self.sender = sender
        self.store = store
        self.sendGateOpen = !arguments.requireSyncToStart
    }

    func setConnectionState(_ state: String) {
        connectionState = state
        if arguments.recenterOnStart, state == "connected" {
            baselineInverse = nil
            baselineCapturePending = true
            baselineStableSampleCount = 0
            hasSmoothedQuaternion = false
            sendGateOpen = !arguments.requireSyncToStart
        }
        store.update { snapshot in
            snapshot.connection = state
            if state == "connected" {
                snapshot.readinessState = "active_not_ready"
            } else {
                snapshot.readinessState = "disabled_disconnected"
                snapshot.sendGateOpen = false
            }
            snapshot.syncRequired = arguments.requireSyncToStart
        }
    }

    func registerError(_ message: String) {
        lastError = message
        store.update { snapshot in
            snapshot.lastError = message
            snapshot.sendErrors = sendErrors
            snapshot.invalidSamples = invalidSamples
            snapshot.connection = connectionState
        }
    }

    func handleMotion(_ motion: CMDeviceMotion) {
        let rawCoreMotionQuaternion = Quaternion(
            x: Float(motion.attitude.quaternion.x),
            y: Float(motion.attitude.quaternion.y),
            z: Float(motion.attitude.quaternion.z),
            w: Float(motion.attitude.quaternion.w)
        ).normalized()
        // CoreMotion attitude quaternion is reference->device.
        // LocusQ orientation path expects device->reference for listener basis.
        let rawQuaternion = applyOrientationSignCorrections(
            remapCoreMotionQuaternionToSteamBasis(rawCoreMotionQuaternion)
                .conjugate()
                .normalized()
        )

        let sensorLocation: String
        switch motion.sensorLocation {
        case .default:
            sensorLocation = "default"
        case .headphoneLeft:
            sensorLocation = "headphone_left"
        case .headphoneRight:
            sensorLocation = "headphone_right"
        @unknown default:
            sensorLocation = "unknown"
        }

        let gravityCoreMotion = Vector3(
            x: motion.gravity.x,
            y: motion.gravity.y,
            z: motion.gravity.z
        )
        let userAccelerationCoreMotionG = Vector3(
            x: motion.userAcceleration.x,
            y: motion.userAcceleration.y,
            z: motion.userAcceleration.z
        )

        if arguments.recenterOnStart {
            if sensorLocation != baselineSensorLocation {
                baselineSensorLocation = sensorLocation
                baselineInverse = nil
                baselineCapturePending = true
                baselineStableSampleCount = 0
                hasSmoothedQuaternion = false
            }

            if baselineCapturePending || baselineInverse == nil {
                let rawAngularSpeedDeg = sqrt(
                    (motion.rotationRate.x * motion.rotationRate.x)
                        + (motion.rotationRate.y * motion.rotationRate.y)
                        + (motion.rotationRate.z * motion.rotationRate.z)
                ) * 57.2957795
                let rawAccelMagG = userAccelerationCoreMotionG.magnitude()
                let sampleStable = rawAngularSpeedDeg < 12.0 && rawAccelMagG < 0.08
                baselineStableSampleCount = sampleStable ? (baselineStableSampleCount + 1) : 0
                if baselineStableSampleCount >= 10 {
                    baselineInverse = rawQuaternion.conjugate().normalized()
                    baselineCapturePending = false
                    baselineStableSampleCount = 0
                }
            }
        } else {
            baselineCapturePending = false
        }

        var recenteredQuaternion = rawQuaternion
        if arguments.recenterOnStart, let baselineInverse {
            recenteredQuaternion = baselineInverse.multiplied(by: rawQuaternion).normalized()
        }

        let filteredQuaternion: Quaternion
        if !hasSmoothedQuaternion {
            smoothedQuaternion = recenteredQuaternion
            hasSmoothedQuaternion = true
            filteredQuaternion = recenteredQuaternion
        } else {
            let deltaDeg = smoothedQuaternion.angularDistanceDeg(to: recenteredQuaternion)
            let candidate = deltaDeg < arguments.deadbandDeg ? smoothedQuaternion : recenteredQuaternion
            smoothedQuaternion = smoothedQuaternion.nlerp(to: candidate, alpha: arguments.stabilizationAlpha)
            filteredQuaternion = smoothedQuaternion
        }

        let eulerDeg = filteredQuaternion.toEulerDegrees()

        let motionTimestamp = motion.timestamp
        let dtSec: Double
        if let previous = previousMotionTimestamp {
            dtSec = max(0.0, min(0.100, motionTimestamp - previous))
        } else {
            dtSec = 0.0
        }
        previousMotionTimestamp = motionTimestamp

        let gravity = remapCoreMotionVectorToSteamBasis(gravityCoreMotion)
        let userAccelerationG = remapCoreMotionVectorToSteamBasis(userAccelerationCoreMotionG)

        let accelerationMps2 = userAccelerationG * 9.80665
        velocityEstimateMps = ((velocityEstimateMps + (accelerationMps2 * dtSec)) * arguments.velocityDamping)
            .clamped(min: -3.0, max: 3.0)
        displacementEstimateM = (displacementEstimateM + (velocityEstimateMps * dtSec)).clamped(min: -1.5, max: 1.5)

        let rotationRateCoreMotionDeg = remapCoreMotionVectorToSteamBasis(
            Vector3(
                x: motion.rotationRate.x * 57.2957795,
                y: motion.rotationRate.y * 57.2957795,
                z: motion.rotationRate.z * 57.2957795
            )
        )
        let rotationRateXDeg = rotationRateCoreMotionDeg.x
        let rotationRateYDeg = rotationRateCoreMotionDeg.y
        let rotationRateZDeg = rotationRateCoreMotionDeg.z
        let angularSpeed = sqrt(
            (rotationRateXDeg * rotationRateXDeg)
                + (rotationRateYDeg * rotationRateYDeg)
                + (rotationRateZDeg * rotationRateZDeg)
        )

        let motionNorm = clamp01((angularSpeed / 220.0) * 0.7 + (userAccelerationG.magnitude() / 1.8) * 0.3)
        let stabilityNorm = clamp01(1.0 - motionNorm)

        let hasInEarSensor = (sensorLocation == "headphone_left" || sensorLocation == "headphone_right")
        let baselineLocked = !arguments.recenterOnStart || (baselineInverse != nil && !baselineCapturePending)
        let readinessState: String
        if connectionState == "connected" {
            readinessState = (hasInEarSensor && baselineLocked) ? "active_ready" : "active_not_ready"
        } else {
            readinessState = "disabled_disconnected"
        }

        if readinessState != "active_ready" {
            sendGateOpen = false
        }

        if consumeSyncRequest() {
            if readinessState == "active_ready" {
                baselineInverse = rawQuaternion.conjugate().normalized()
                baselineCapturePending = false
                baselineStableSampleCount = 0
                hasSmoothedQuaternion = false
                sendGateOpen = true
            }
        } else if !arguments.requireSyncToStart && readinessState == "active_ready" {
            sendGateOpen = true
        }

        let nowMs = nowEpochMilliseconds()
        let shouldSend = (motionTimestamp - lastSentMotionTimestamp) >= (1.0 / Double(arguments.hz))
        var sentSeq = sequence == 0 ? 0 : sequence - 1
        var packetTimestampMs: UInt64 = nowMs
        let allowPoseSend = (readinessState == "active_ready") && sendGateOpen

        if shouldSend && allowPoseSend {
            let packet = PosePacketV1(
                quaternion: filteredQuaternion,
                timestampMs: nowMs,
                sequence: sequence
            )
            let payload = packet.encodedData()
            do {
                try sender.send(payload)
                packetCount += 1
                sentSeq = sequence
                packetTimestampMs = nowMs
                if lastSentEpochMs > 0 && nowMs >= lastSentEpochMs {
                    let intervalMs = Double(nowMs - lastSentEpochMs)
                    if intervalMsEma <= 0.0 {
                        intervalMsEma = intervalMs
                        jitterMsEma = 0.0
                    } else {
                        let delta = abs(intervalMs - intervalMsEma)
                        intervalMsEma = (intervalMsEma * 0.85) + (intervalMs * 0.15)
                        jitterMsEma = (jitterMsEma * 0.8) + (delta * 0.2)
                    }
                }
                if previousSentSeq > 0 {
                    latestSeqGap = max(1, Int(sentSeq) - Int(previousSentSeq))
                } else {
                    latestSeqGap = 1
                }
                previousSentSeq = sentSeq
                lastSentEpochMs = nowMs
                sequence &+= 1
                lastSentMotionTimestamp = motionTimestamp
                lastError = ""
            } catch {
                sendErrors += 1
                lastError = "\(error)"
            }
        }

        let heading = motion.heading
        let headingValue = heading >= 0.0 ? heading : nil

        let ageMs = packetTimestampMs > 0 && nowMs >= packetTimestampMs
            ? Double(nowMs - packetTimestampMs)
            : 0.0
        let baselineState: String
        if !arguments.recenterOnStart {
            baselineState = "disabled"
        } else if baselineInverse == nil || baselineCapturePending {
            baselineState = "capturing"
        } else {
            baselineState = "locked_\(baselineSensorLocation)"
        }

        store.update { snapshot in
            snapshot.mode = .live
            snapshot.source = "coremotion_headphones"
            snapshot.connection = connectionState
            snapshot.frameMapping = "coremotion->steam (x,+90degX,device^-1,rollSignFlip)"
            snapshot.baselineState = baselineState
            snapshot.readinessState = readinessState
            snapshot.sendGateOpen = allowPoseSend
            snapshot.syncRequired = arguments.requireSyncToStart
            snapshot.sequence = sentSeq
            snapshot.timestampMs = packetTimestampMs
            snapshot.ageMs = ageMs
            snapshot.qx = filteredQuaternion.x
            snapshot.qy = filteredQuaternion.y
            snapshot.qz = filteredQuaternion.z
            snapshot.qw = filteredQuaternion.w
            snapshot.yawDeg = eulerDeg.yaw
            snapshot.pitchDeg = eulerDeg.pitch
            snapshot.rollDeg = eulerDeg.roll
            snapshot.rotRateXDeg = rotationRateXDeg
            snapshot.rotRateYDeg = rotationRateYDeg
            snapshot.rotRateZDeg = rotationRateZDeg
            snapshot.angularSpeedDegPerSec = angularSpeed
            snapshot.gravity = gravity
            snapshot.userAccelerationG = userAccelerationG
            snapshot.velocityEstimateMps = velocityEstimateMps
            snapshot.displacementEstimateM = displacementEstimateM
            snapshot.dtSec = dtSec
            snapshot.headingDeg = headingValue
            snapshot.sensorLocation = sensorLocation
            snapshot.motionNorm = motionNorm
            snapshot.stabilityNorm = stabilityNorm
            snapshot.packetCount = packetCount
            snapshot.sendErrors = sendErrors
            snapshot.invalidSamples = invalidSamples
            snapshot.lastError = lastError
            snapshot.streamHealth.effectiveRateHz = intervalMsEma > 0.0 ? (1000.0 / intervalMsEma) : Double(arguments.hz)
            snapshot.streamHealth.intervalMs = intervalMsEma > 0.0 ? intervalMsEma : (1000.0 / Double(arguments.hz))
            snapshot.streamHealth.jitterMs = jitterMsEma
            snapshot.streamHealth.seqGap = latestSeqGap
        }

        if arguments.verbose && shouldSend {
            print(
                String(
                    format: "packet seq=%u ts_ms=%llu q=[%.6f,%.6f,%.6f,%.6f] ypr=[%.2f,%.2f,%.2f] ang=%.2f deg/s",
                    sentSeq,
                    packetTimestampMs,
                    filteredQuaternion.x,
                    filteredQuaternion.y,
                    filteredQuaternion.z,
                    filteredQuaternion.w,
                    eulerDeg.yaw,
                    eulerDeg.pitch,
                    eulerDeg.roll,
                    angularSpeed
                )
            )
        }
    }

    func registerInvalidSample(_ reason: String) {
        invalidSamples += 1
        lastError = reason
        store.update { snapshot in
            snapshot.invalidSamples = invalidSamples
            snapshot.lastError = reason
            snapshot.connection = connectionState
        }
    }
}

private final class HeadphoneConnectionDelegate: NSObject, CMHeadphoneMotionManagerDelegate {
    private let onConnectChange: (String) -> Void

    init(onConnectChange: @escaping (String) -> Void) {
        self.onConnectChange = onConnectChange
    }

    func headphoneMotionManagerDidConnect(_ manager: CMHeadphoneMotionManager) {
        _ = manager
        onConnectChange("connected")
    }

    func headphoneMotionManagerDidDisconnect(_ manager: CMHeadphoneMotionManager) {
        _ = manager
        onConnectChange("disconnected")
    }
}

private func runLive(arguments: CompanionArguments,
                     sender: UDPSender,
                     store: SnapshotStore,
                     monitor: CompanionMonitorWindow?,
                     pluginAckReceiver: PluginAckReceiver?) throws {
    let authorization = CMHeadphoneMotionManager.authorizationStatus()
    guard authorization == .authorized || authorization == .notDetermined else {
        throw CompanionError.liveModeUnavailable("authorization denied/restricted")
    }

    let manager = CMHeadphoneMotionManager()
    guard manager.isDeviceMotionAvailable else {
        throw CompanionError.liveModeUnavailable("No compatible headphone motion device available")
    }

    let queue = OperationQueue()
    queue.name = "LocusQHeadTracker.LiveMotion"
    queue.qualityOfService = arguments.schedulingProfile.operationQoS

    let monitorSleepMicros = useconds_t(max(1, Int(1_000_000 / max(5, arguments.monitorHz))))

    let processor = LiveRuntimeProcessor(arguments: arguments, sender: sender, store: store)
    let delegate = HeadphoneConnectionDelegate { state in
        processor.setConnectionState(state)
    }

    manager.delegate = delegate
    manager.startConnectionStatusUpdates()
    processor.setConnectionState("available")

    manager.startDeviceMotionUpdates(to: queue) { motion, error in
        if stopRequested() {
            return
        }
        if let error {
            processor.registerError("motion_error: \(error.localizedDescription)")
            return
        }
        guard let motion else {
            processor.registerInvalidSample("motion_nil")
            return
        }

        processor.handleMotion(motion)
    }

    print(
        "companion_start mode=live host=\(arguments.host) port=\(arguments.port) hz=\(arguments.hz) seconds=\(arguments.seconds) recenter=\(arguments.recenterOnStart) alpha=\(arguments.stabilizationAlpha) deadband=\(arguments.deadbandDeg)"
    )

    let hasDeadline = arguments.seconds > 0
    let deadline = Date().addingTimeInterval(Double(arguments.seconds))
    var stopReason = "duration_complete"
    var cachedOutputDevice = readDefaultOutputDeviceSnapshot()
    var nextOutputDevicePollAtMs = nowEpochMilliseconds()

    while true {
        if stopRequested() {
            stopReason = "signal_stop"
            break
        }
        if hasDeadline && Date() >= deadline {
            stopReason = "duration_complete"
            break
        }

        mergePluginAndDeviceDiagnostics(
            store: store,
            pluginAckReceiver: pluginAckReceiver,
            cachedOutputDevice: &cachedOutputDevice,
            nextOutputDevicePollAtMs: &nextOutputDevicePollAtMs
        )

        if let monitor {
            let snapshot = store.read()
            withMainActorSync {
                monitor.render(snapshot: snapshot, args: arguments)
            }
            usleep(monitorSleepMicros)
        } else {
            usleep(10_000)
        }
    }

    manager.stopDeviceMotionUpdates()
    manager.stopConnectionStatusUpdates()

    let finalSnapshot = store.read()
    if stopReason == "signal_stop" {
        print("companion_shutdown reason=signal packets_sent=\(finalSnapshot.packetCount)")
    }
    print("companion_done mode=live packets_sent=\(finalSnapshot.packetCount) send_errors=\(finalSnapshot.sendErrors) invalid_samples=\(finalSnapshot.invalidSamples) reason=\(stopReason)")
}
#endif

private func runCompanion(arguments: CompanionArguments) throws {
    clearStopRequested()
    installTerminationHandlers()
    defer {
        restoreTerminationHandlers()
        clearStopRequested()
    }

    let sender = try UDPSender(host: arguments.host, port: arguments.port)
    defer {
        sender.close()
    }

    let store = SnapshotStore()
    store.update { snapshot in
        snapshot.schedulingProfile = arguments.schedulingProfile.rawValue
        snapshot.monitorHz = arguments.monitorHz
    }
    let monitor = arguments.ui ? createMonitorWindowOnMain() : nil
    let pluginAckReceiver = PluginAckReceiver(
        listenPort: arguments.pluginAckPort,
        workerQoS: arguments.schedulingProfile.ackThreadQoS
    )
    pluginAckReceiver.start()
    defer {
        pluginAckReceiver.stop()
    }
    if let monitor {
        withMainActorSync {
            monitor.show()
        }
    }

    switch arguments.mode {
    case .synthetic:
        try runSynthetic(
            arguments: arguments,
            sender: sender,
            store: store,
            monitor: monitor,
            pluginAckReceiver: pluginAckReceiver
        )
    case .live:
#if canImport(CoreMotion)
        try runLive(
            arguments: arguments,
            sender: sender,
            store: store,
            monitor: monitor,
            pluginAckReceiver: pluginAckReceiver
        )
#else
        throw CompanionError.liveModeUnavailable("CoreMotion unavailable on this platform")
#endif
    }

    if let monitor {
        var cachedOutputDevice = readDefaultOutputDeviceSnapshot()
        var nextOutputDevicePollAtMs = nowEpochMilliseconds()
        mergePluginAndDeviceDiagnostics(
            store: store,
            pluginAckReceiver: pluginAckReceiver,
            cachedOutputDevice: &cachedOutputDevice,
            nextOutputDevicePollAtMs: &nextOutputDevicePollAtMs
        )
        let snapshot = store.read()
        withMainActorSync {
            monitor.render(snapshot: snapshot, args: arguments)
        }
    }
}

private final class SharedErrorBox: @unchecked Sendable {
    private let lock = NSLock()
    private var error: Error?

    func store(_ value: Error) {
        lock.lock()
        defer { lock.unlock() }
        error = value
    }

    func load() -> Error? {
        lock.lock()
        defer { lock.unlock() }
        return error
    }
}

#if canImport(AppKit)
@MainActor
private func runCompanionWithAppEventLoop(arguments: CompanionArguments) throws {
    let app = NSApplication.shared
    app.setActivationPolicy(.regular)

    let errorBox = SharedErrorBox()
    let completion = DispatchSemaphore(value: 0)

    DispatchQueue.global(qos: arguments.schedulingProfile.dispatchQoS).async {
        do {
            try runCompanion(arguments: arguments)
        } catch {
            errorBox.store(error)
        }

        DispatchQueue.main.async {
            markStopRequested()
            app.stop(nil)
            if let pulse = NSEvent.otherEvent(
                with: .applicationDefined,
                location: .zero,
                modifierFlags: [],
                timestamp: ProcessInfo.processInfo.systemUptime,
                windowNumber: 0,
                context: nil,
                subtype: 0,
                data1: 0,
                data2: 0
            ) {
                app.postEvent(pulse, atStart: false)
            }
            completion.signal()
        }
    }

    app.run()
    completion.wait()

    if let error = errorBox.load() {
        throw error
    }
}
#endif

private func sanitizeLaunchArguments(_ rawArguments: [String], launchedFromAppBundle: Bool) -> [String] {
    guard !rawArguments.isEmpty else {
        return []
    }

    // Finder/Dock may pass launch metadata flags (for example -psn_*) that are not
    // user intent. For app-bundle launches, drop these and default to no-arg mode
    // if any unrecognized system flags remain.
    let recognizedOptions = Set([
        "--mode",
        "--host",
        "--port",
        "--plugin-ack-port",
        "--hz",
        "--seconds",
        "--ui",
        "--verbose",
        "--sched-profile",
        "--monitor-hz",
        "--no-recenter",
        "--stabilize-alpha",
        "--deadband-deg",
        "--velocity-damping",
        "--yaw-amplitude",
        "--pitch-amplitude",
        "--roll-amplitude",
        "--yaw-frequency",
        "--help",
        "-h"
    ])

    let filtered = rawArguments.filter { !$0.hasPrefix("-psn_") }
    if !launchedFromAppBundle {
        return filtered
    }

    for token in filtered where token.hasPrefix("-") && !recognizedOptions.contains(token) {
        if token.hasPrefix("-NS") || token.hasPrefix("-Apple") || token.hasPrefix("-psn_") {
            return []
        }
        return []
    }

    return filtered
}

do {
    let rawArguments = Array(CommandLine.arguments.dropFirst())
    let executablePath = CommandLine.arguments.first ?? ""
    let launchedFromAppBundle = executablePath.contains(".app/Contents/MacOS/")
    let normalizedArguments = sanitizeLaunchArguments(rawArguments, launchedFromAppBundle: launchedFromAppBundle)

    let runSelectedMode: (CompanionArguments) throws -> Void = { arguments in
#if canImport(AppKit)
        if arguments.ui && launchedFromAppBundle {
            try withMainActorSyncThrowing {
                try runCompanionWithAppEventLoop(arguments: arguments)
            }
        } else {
            try runCompanion(arguments: arguments)
        }
#else
        try runCompanion(arguments: arguments)
#endif
    }

    if normalizedArguments.isEmpty {
        var launchArguments = CompanionArguments()
        launchArguments.ui = true
        launchArguments.mode = .live
        launchArguments.seconds = 0

        do {
#if canImport(CoreMotion)
            try runSelectedMode(launchArguments)
#else
            throw CompanionError.liveModeUnavailable("CoreMotion unavailable on this platform")
#endif
        } catch CompanionError.liveModeUnavailable(let reason) {
            fputs("live launch unavailable (\(reason)); falling back to synthetic UI mode\n", stderr)
            launchArguments.mode = .synthetic
            try runSelectedMode(launchArguments)
        }
    } else {
        guard let arguments = try CompanionArguments.parse(normalizedArguments) else {
            exit(EXIT_SUCCESS)
        }
        try runSelectedMode(arguments)
    }
} catch {
    fputs("error: \(error)\n", stderr)
    fputs("\(CompanionArguments.usage())\n", stderr)
    exit(EXIT_FAILURE)
}
