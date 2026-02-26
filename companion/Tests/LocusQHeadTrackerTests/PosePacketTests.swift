import XCTest
@testable import LocusQHeadTrackerCore

final class PosePacketTests: XCTestCase {
    func testSerializeMatchesV1WireFormat() {
        let packet = PosePacket(
            qx: 1.25,
            qy: -2.5,
            qz: 0.5,
            qw: -0.75,
            timestampMs: 0x0102030405060708,
            seq: 0xAABBCCDD
        )

        let encoded = packet.serialize()
        XCTAssertEqual(encoded.count, PosePacket.encodedSize)

        XCTAssertEqual(readUInt32(encoded, at: 0), PosePacket.magic)
        XCTAssertEqual(readUInt32(encoded, at: 4), PosePacket.version)
        XCTAssertEqual(readFloat(encoded, at: 8), packet.qx)
        XCTAssertEqual(readFloat(encoded, at: 12), packet.qy)
        XCTAssertEqual(readFloat(encoded, at: 16), packet.qz)
        XCTAssertEqual(readFloat(encoded, at: 20), packet.qw)
        XCTAssertEqual(readUInt64(encoded, at: 24), packet.timestampMs)
        XCTAssertEqual(readUInt32(encoded, at: 32), packet.seq)
    }

    private func readUInt32(_ data: Data, at offset: Int) -> UInt32 {
        data.withUnsafeBytes { raw in
            let value = raw.load(fromByteOffset: offset, as: UInt32.self)
            return UInt32(littleEndian: value)
        }
    }

    private func readUInt64(_ data: Data, at offset: Int) -> UInt64 {
        data.withUnsafeBytes { raw in
            let value = raw.load(fromByteOffset: offset, as: UInt64.self)
            return UInt64(littleEndian: value)
        }
    }

    private func readFloat(_ data: Data, at offset: Int) -> Float {
        Float(bitPattern: readUInt32(data, at: offset))
    }
}
