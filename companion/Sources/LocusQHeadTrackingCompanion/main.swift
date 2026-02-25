import Darwin
import Foundation

private enum CompanionError: Error, CustomStringConvertible {
    case invalidArgument(String)
    case socketCreateFailed(Int32)
    case invalidIPv4Address(String)
    case sendFailed(Int32)
    case sendRetryLimitExceeded(Int32, Int)
    case interrupted(String)

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
        }
    }
}

private enum ReliabilityConfig {
    static let maxSendRetries = 3
    static let retryBackoffMicros: [useconds_t] = [1_000, 2_000, 5_000]
}

private struct Quaternion {
    var x: Float
    var y: Float
    var z: Float
    var w: Float

    func normalized() -> Quaternion {
        let length = sqrtf((x * x) + (y * y) + (z * z) + (w * w))
        guard length > 0 else { return Quaternion(x: 0, y: 0, z: 0, w: 1) }
        return Quaternion(x: x / length, y: y / length, z: z / length, w: w / length)
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
        payload.appendLittleEndian(UInt32(0)) // reserved
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

private struct CompanionArguments {
    var host: String = "127.0.0.1"
    var port: UInt16 = 19765
    var hz: Int = 30
    var seconds: Int = 2
    var yawAmplitudeDeg: Float = 35.0
    var pitchAmplitudeDeg: Float = 10.0
    var rollAmplitudeDeg: Float = 5.0
    var yawFrequencyHz: Float = 0.25

    static func usage() -> String {
        """
        LocusQ Head-Tracking Companion (Slice C MVP)

        Usage:
          locusq-headtrack-companion [options]

        Options:
          --host <ipv4>             Destination host (default: 127.0.0.1)
          --port <uint16>           Destination UDP port (default: 19765)
          --hz <int>                Packet send rate in Hz (default: 30)
          --seconds <int>           Total send duration in seconds (default: 2)
          --yaw-amplitude <float>   Synthetic yaw amplitude in degrees (default: 35)
          --pitch-amplitude <float> Synthetic pitch amplitude in degrees (default: 10)
          --roll-amplitude <float>  Synthetic roll amplitude in degrees (default: 5)
          --yaw-frequency <float>   Synthetic yaw oscillation frequency in Hz (default: 0.25)
          --help                    Show this help message
        """
    }

    static func parse(_ raw: [String]) throws -> CompanionArguments? {
        var args = CompanionArguments()
        var index = 0
        while index < raw.count {
            let key = raw[index]
            switch key {
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
                guard let parsed = Int(value), parsed > 0 else {
                    throw CompanionError.invalidArgument("\(key) requires positive integer")
                }
                args.seconds = parsed
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

private func nowEpochMilliseconds() -> UInt64 {
    UInt64((Date().timeIntervalSince1970 * 1000.0).rounded())
}

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

    let packetCount = max(1, arguments.hz * arguments.seconds)
    let intervalSeconds = 1.0 / Double(arguments.hz)
    let microsPerTick = useconds_t(max(1.0, intervalSeconds * 1_000_000.0))

    print(
        "companion_start host=\(arguments.host) port=\(arguments.port) hz=\(arguments.hz) seconds=\(arguments.seconds) retries=\(ReliabilityConfig.maxSendRetries)"
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

        seq &+= 1

        if stopRequested() {
            stopReason = "signal_stop"
            break
        }

        usleep(microsPerTick)
    }

    if stopReason == "signal_stop" {
        print("companion_shutdown reason=signal packets_sent=\(sentPackets) requested_packets=\(packetCount)")
    }
    print("companion_done packets_sent=\(sentPackets) requested_packets=\(packetCount) reason=\(stopReason)")
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
