import Darwin
import Foundation

#if canImport(CoreMotion)
import CoreMotion
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

private struct CompanionArguments {
    var host: String = "127.0.0.1"
    var port: UInt16 = 19765
    var hz: Int = 60
    var seconds: Int = 0
    var mode: CompanionMode = .synthetic
    var ui: Bool = false
    var verbose: Bool = false

    // Stabilization and frame controls (live mode)
    var recenterOnStart: Bool = true
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
          --hz <int>                Target send rate in Hz (default: 60)
          --seconds <int>           Duration in seconds; 0 = run until signal (default: 0)
          --ui                       Show monitor window with raw + derived telemetry
          --verbose                  Print per-packet logs

        Live Mode Controls:
          --no-recenter             Disable startup recenter transform
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
            case "--no-recenter":
                args.recenterOnStart = false
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

private func markStopRequested() {
    gStopRequested = 1
}

private func clearStopRequested() {
    gStopRequested = 0
}

private func stopRequested() -> Bool {
    gStopRequested != 0
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

private struct RuntimeSnapshot {
    var mode: CompanionMode = .synthetic
    var source: String = "synthetic_generator"
    var connection: String = "n/a"
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

#if canImport(AppKit)
#if canImport(WebKit)
@MainActor
private final class CompanionMonitorWindow: NSObject {
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

        let configuration = WKWebViewConfiguration()
        configuration.defaultWebpagePreferences.allowsContentJavaScript = true
        webView = WKWebView(frame: frame, configuration: configuration)
        webView.autoresizingMask = [.width, .height]

        super.init()

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
            "controls": [
                "recenterOnStart": args.recenterOnStart,
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
              --ok: #46e089;
              --warn: #ffca66;
              --error: #ff6c6c;
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
              grid-template-columns: minmax(520px, 1.05fr) minmax(460px, 0.95fr);
              gap: 12px;
              width: 100%;
              height: 100%;
              padding: 12px;
            }
            @media (max-width: 1280px) {
              #root {
                grid-template-columns: 1fr;
                grid-template-rows: 52% 48%;
              }
            }
            .card {
              border: 1px solid var(--card-border);
              border-radius: 14px;
              background: linear-gradient(180deg, rgba(17, 23, 34, 0.96), rgba(10, 14, 22, 0.98));
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
              padding: 10px 14px;
              border-bottom: 1px solid rgba(84, 115, 160, 0.28);
              background: rgba(16, 28, 44, 0.4);
            }
            .headerTitle {
              font-size: 12px;
              letter-spacing: 0.18em;
              text-transform: uppercase;
              color: #89b8de;
            }
            #statusPill {
              font-size: 12px;
              font-weight: 700;
              padding: 5px 11px;
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
              min-height: 420px;
            }
            #viz {
              position: absolute;
              inset: 0;
            }
            #legend {
              position: absolute;
              left: 12px;
              bottom: 12px;
              display: grid;
              grid-template-columns: repeat(2, minmax(140px, 1fr));
              gap: 6px 12px;
              padding: 10px 12px;
              border-radius: 10px;
              border: 1px solid rgba(76, 108, 149, 0.45);
              background: rgba(10, 17, 27, 0.72);
              font-size: 11px;
              color: #a9c3df;
              backdrop-filter: blur(2px);
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
              grid-template-rows: auto auto 1fr;
              min-height: 0;
            }
            #smoothingControls {
              display: grid;
              grid-template-columns: repeat(2, minmax(160px, 1fr));
              gap: 10px 14px;
              padding: 10px 14px;
              border-bottom: 1px solid rgba(84, 115, 160, 0.28);
              background: rgba(10, 18, 30, 0.45);
              font-size: 11px;
            }
            .controlBlock {
              display: grid;
              gap: 5px;
            }
            .controlBlock label {
              color: var(--muted);
              text-transform: uppercase;
              letter-spacing: 0.12em;
              font-size: 10px;
            }
            .controlBlock input[type="range"] {
              width: 100%;
              accent-color: var(--accent);
            }
            .controlValue {
              color: #c6e6ff;
              font-size: 12px;
            }
            #metrics {
              overflow: auto;
              padding: 10px 12px 14px;
            }
            .section {
              border: 1px solid rgba(74, 102, 140, 0.32);
              border-radius: 10px;
              margin-bottom: 10px;
              overflow: hidden;
              background: rgba(7, 12, 20, 0.72);
            }
            .section h3 {
              margin: 0;
              padding: 7px 10px;
              font-size: 11px;
              letter-spacing: 0.16em;
              text-transform: uppercase;
              color: #8cb8dc;
              border-bottom: 1px solid rgba(74, 102, 140, 0.25);
              background: rgba(12, 21, 34, 0.48);
            }
            .rows {
              display: grid;
              grid-template-columns: minmax(145px, 1fr) minmax(160px, 1fr);
            }
            .rowLabel, .rowValue {
              font-size: 12px;
              padding: 5px 9px;
              border-bottom: 1px solid rgba(67, 89, 119, 0.19);
              line-height: 1.25;
            }
            .rowLabel { color: var(--muted); }
            .rowValue {
              color: #e4f2ff;
              text-align: right;
              word-break: break-word;
            }
            .rowValue.raw {
              color: #ffdca2;
            }
            #threeNotice {
              position: absolute;
              top: 14px;
              right: 14px;
              font-size: 11px;
              padding: 7px 10px;
              border-radius: 8px;
              color: #ffcf82;
              border: 1px solid rgba(255, 181, 83, 0.44);
              background: rgba(75, 47, 13, 0.35);
              display: none;
              z-index: 2;
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
              </div>
              <div id="metrics">
                <div class="section">
                  <h3>Transport</h3>
                  <div class="rows">
                    <div class="rowLabel">Mode</div><div id="mode" class="rowValue">n/a</div>
                    <div class="rowLabel">Source</div><div id="source" class="rowValue">n/a</div>
                    <div class="rowLabel">Connection</div><div id="connection" class="rowValue">n/a</div>
                    <div class="rowLabel">Destination</div><div id="destination" class="rowValue">n/a</div>
                    <div class="rowLabel">Rate</div><div id="rateHz" class="rowValue">n/a</div>
                    <div class="rowLabel">Duration</div><div id="durationText" class="rowValue">n/a</div>
                    <div class="rowLabel">Sequence</div><div id="sequence" class="rowValue">n/a</div>
                    <div class="rowLabel">Timestamp</div><div id="timestampMs" class="rowValue">n/a</div>
                    <div class="rowLabel">Packets</div><div id="packetCount" class="rowValue">n/a</div>
                    <div class="rowLabel">Send Errors</div><div id="sendErrors" class="rowValue">n/a</div>
                    <div class="rowLabel">Invalid Samples</div><div id="invalidSamples" class="rowValue">n/a</div>
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
                    <div class="rowLabel">Recenter on Start</div><div id="recenterOnStart" class="rowValue">n/a</div>
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
                root: null,
                arrows: {},
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
                state.camera = new THREE.PerspectiveCamera(42, 1, 0.01, 60);
                state.camera.position.set(2.2, 1.35, 2.6);
                state.camera.lookAt(0, 0.15, 0);

                state.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
                state.renderer.setPixelRatio(window.devicePixelRatio || 1);
                container.appendChild(state.renderer.domElement);

                const hemi = new THREE.HemisphereLight(0x8cc6ff, 0x192030, 1.05);
                state.scene.add(hemi);
                const key = new THREE.DirectionalLight(0xb9e7ff, 0.9);
                key.position.set(2.0, 3.0, 2.0);
                state.scene.add(key);

                const grid = new THREE.GridHelper(5, 30, 0x22344a, 0x152334);
                grid.position.y = -0.35;
                state.scene.add(grid);

                const axes = new THREE.AxesHelper(0.5);
                axes.position.set(-2.1, -0.35, 1.9);
                state.scene.add(axes);

                const root = new THREE.Group();
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
                  new THREE.MeshStandardMaterial({ color: 0x5cc4ff, emissive: 0x123348 })
                );
                leftPod.position.set(-0.35, 0.01, 0.02);
                root.add(leftPod);

                const rightPod = new THREE.Mesh(
                  new THREE.SphereGeometry(0.058, 24, 20),
                  new THREE.MeshStandardMaterial({ color: 0xf48ca2, emissive: 0x3d1822 })
                );
                rightPod.position.set(0.35, 0.01, 0.02);
                root.add(rightPod);

                const frontCue = new THREE.Mesh(
                  new THREE.ConeGeometry(0.07, 0.22, 18),
                  new THREE.MeshStandardMaterial({ color: 0xff7a7a, emissive: 0x471919 })
                );
                frontCue.position.set(0, -0.26, -1.12);
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
                  state.renderer.setSize(width, height, false);
                };
                window.addEventListener("resize", resize);
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

                pill.className = "";
                if (hasErrors) {
                  pill.classList.add("error");
                  pill.textContent = "ERROR";
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
                const controls = snapshot.controls || {};

                setText("quatRaw", "[" + f(num(q.x), 4) + ", " + f(num(q.y), 4) + ", " + f(num(q.z), 4) + ", " + f(num(q.w, 1), 4) + "]");
                setText("yprRaw", f(num(ypr.yaw), 2) + " / " + f(num(ypr.pitch), 2) + " / " + f(num(ypr.roll), 2) + " deg");
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
                setText("recenterOnStart", controls.recenterOnStart ? "true" : "false");
                setText("stabilizationAlpha", f(num(controls.stabilizationAlpha), 3));
                setText("deadbandDeg", f(num(controls.deadbandDeg), 3));
                setText("velocityDamping", f(num(controls.velocityDamping), 3));
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

                state.root.quaternion.copy(state.filter.quaternion);

                const forward = new THREE.Vector3(0, 0, -1).applyQuaternion(state.filter.quaternion);
                const up = new THREE.Vector3(0, 1, 0).applyQuaternion(state.filter.quaternion);
                const right = new THREE.Vector3(1, 0, 0).applyQuaternion(state.filter.quaternion);

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
private final class CompanionMonitorWindow {
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
            rate=%dHz duration=%@
            seq=%u timestamp=%llu age=%.2fms
            q=[%.6f, %.6f, %.6f, %.6f]
            ypr=[%.2f, %.2f, %.2f] deg
            rot=[%.2f, %.2f, %.2f] deg/s angular=%.2f deg/s
            accel=[%.3f, %.3f, %.3f]g velocity=[%.3f, %.3f, %.3f]m/s
            displacement=[%.3f, %.3f, %.3f]m heading=%@
            packet_count=%d send_errors=%d invalid=%d
            """,
            snapshot.mode.rawValue,
            snapshot.source,
            snapshot.connection,
            args.host,
            args.port,
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
            snapshot.invalidSamples
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

private func clamp01(_ value: Double) -> Double {
    max(0.0, min(1.0, value))
}

private func pumpMainRunLoopSlice() {
    _ = RunLoop.main.run(mode: .default, before: Date().addingTimeInterval(0.01))
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
                          monitor: CompanionMonitorWindow?) throws {
    let packetCount = (arguments.seconds == 0)
        ? Int.max
        : max(1, arguments.hz * arguments.seconds)
    let intervalSeconds = 1.0 / Double(arguments.hz)
    let microsPerTick = useconds_t(max(1.0, intervalSeconds * 1_000_000.0))

    print(
        "companion_start mode=synthetic host=\(arguments.host) port=\(arguments.port) hz=\(arguments.hz) seconds=\(arguments.seconds) retries=\(ReliabilityConfig.maxSendRetries)"
    )

    var seq: UInt32 = 1
    var sentPackets = 0
    var stopReason = "duration_complete"

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
        }

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
            pumpMainRunLoopSlice()
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

    private var sequence: UInt32 = 1
    private var lastSentMotionTimestamp: TimeInterval = -.greatestFiniteMagnitude
    private var baselineInverse: Quaternion?
    private var smoothedQuaternion = Quaternion.identity
    private var hasSmoothedQuaternion = false

    private var previousMotionTimestamp: TimeInterval?
    private var velocityEstimateMps = Vector3.zero
    private var displacementEstimateM = Vector3.zero

    private(set) var packetCount = 0
    private(set) var sendErrors = 0
    private(set) var invalidSamples = 0
    private var lastError = ""

    private var connectionState = "awaiting_device"

    init(arguments: CompanionArguments, sender: UDPSender, store: SnapshotStore) {
        self.arguments = arguments
        self.sender = sender
        self.store = store
    }

    func setConnectionState(_ state: String) {
        connectionState = state
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
        let rawQuaternion = Quaternion(
            x: Float(motion.attitude.quaternion.x),
            y: Float(motion.attitude.quaternion.y),
            z: Float(motion.attitude.quaternion.z),
            w: Float(motion.attitude.quaternion.w)
        ).normalized()

        var recenteredQuaternion = rawQuaternion
        if arguments.recenterOnStart {
            if baselineInverse == nil {
                baselineInverse = rawQuaternion.conjugate().normalized()
            }
            if let baselineInverse {
                recenteredQuaternion = baselineInverse.multiplied(by: rawQuaternion).normalized()
            }
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

        let gravity = Vector3(
            x: motion.gravity.x,
            y: motion.gravity.y,
            z: motion.gravity.z
        )
        let userAccelerationG = Vector3(
            x: motion.userAcceleration.x,
            y: motion.userAcceleration.y,
            z: motion.userAcceleration.z
        )

        let accelerationMps2 = userAccelerationG * 9.80665
        velocityEstimateMps = ((velocityEstimateMps + (accelerationMps2 * dtSec)) * arguments.velocityDamping)
            .clamped(min: -3.0, max: 3.0)
        displacementEstimateM = (displacementEstimateM + (velocityEstimateMps * dtSec)).clamped(min: -1.5, max: 1.5)

        let rotationRateXDeg = motion.rotationRate.x * 57.2957795
        let rotationRateYDeg = motion.rotationRate.y * 57.2957795
        let rotationRateZDeg = motion.rotationRate.z * 57.2957795
        let angularSpeed = sqrt(
            (rotationRateXDeg * rotationRateXDeg)
                + (rotationRateYDeg * rotationRateYDeg)
                + (rotationRateZDeg * rotationRateZDeg)
        )

        let motionNorm = clamp01((angularSpeed / 220.0) * 0.7 + (userAccelerationG.magnitude() / 1.8) * 0.3)
        let stabilityNorm = clamp01(1.0 - motionNorm)

        let nowMs = nowEpochMilliseconds()
        let shouldSend = (motionTimestamp - lastSentMotionTimestamp) >= (1.0 / Double(arguments.hz))
        var sentSeq = sequence == 0 ? 0 : sequence - 1
        var packetTimestampMs: UInt64 = nowMs

        if shouldSend {
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

        let ageMs = packetTimestampMs > 0 && nowMs >= packetTimestampMs
            ? Double(nowMs - packetTimestampMs)
            : 0.0

        store.update { snapshot in
            snapshot.mode = .live
            snapshot.source = "coremotion_headphones"
            snapshot.connection = connectionState
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
                     monitor: CompanionMonitorWindow?) throws {
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
    queue.qualityOfService = .userInitiated

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

    while true {
        if stopRequested() {
            stopReason = "signal_stop"
            break
        }
        if hasDeadline && Date() >= deadline {
            stopReason = "duration_complete"
            break
        }

        if let monitor {
            let snapshot = store.read()
            withMainActorSync {
                monitor.render(snapshot: snapshot, args: arguments)
            }
            pumpMainRunLoopSlice()
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
    let monitor = arguments.ui ? createMonitorWindowOnMain() : nil
    if let monitor {
        withMainActorSync {
            monitor.show()
        }
    }

    switch arguments.mode {
    case .synthetic:
        try runSynthetic(arguments: arguments, sender: sender, store: store, monitor: monitor)
    case .live:
#if canImport(CoreMotion)
        try runLive(arguments: arguments, sender: sender, store: store, monitor: monitor)
#else
        throw CompanionError.liveModeUnavailable("CoreMotion unavailable on this platform")
#endif
    }

    if let monitor {
        let snapshot = store.read()
        withMainActorSync {
            monitor.render(snapshot: snapshot, args: arguments)
        }
        for _ in 0..<10 {
            pumpMainRunLoopSlice()
        }
    }
}

do {
    guard let arguments = try CompanionArguments.parse(Array(CommandLine.arguments.dropFirst())) else {
        exit(EXIT_SUCCESS)
    }
    try runCompanion(arguments: arguments)
} catch {
    fputs("error: \(error)\n", stderr)
    fputs("\(CompanionArguments.usage())\n", stderr)
    exit(EXIT_FAILURE)
}
