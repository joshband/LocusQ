import Foundation

public struct PosePacket: Sendable, Equatable {
    public static let magic: UInt32 = 0x4C515054
    public static let version: UInt32 = 1
    public static let encodedSize = 36

    public let qx: Float
    public let qy: Float
    public let qz: Float
    public let qw: Float
    public let timestampMs: UInt64
    public let seq: UInt32

    public init(qx: Float, qy: Float, qz: Float, qw: Float, timestampMs: UInt64, seq: UInt32) {
        self.qx = qx
        self.qy = qy
        self.qz = qz
        self.qw = qw
        self.timestampMs = timestampMs
        self.seq = seq
    }

    public func serialize() -> Data {
        var data = Data(capacity: Self.encodedSize)

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
