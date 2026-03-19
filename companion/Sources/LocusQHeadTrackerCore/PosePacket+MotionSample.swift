import Foundation

public extension MotionSample {
    func posePacket(sequence: UInt32) -> PosePacket {
        PosePacket(
            qx: qx,
            qy: qy,
            qz: qz,
            qw: qw,
            timestampMs: timestampMs,
            seq: sequence,
            angVx: angVx,
            angVy: angVy,
            angVz: angVz,
            sensorLocationFlags: (UInt32(sensorLocation) & 0x3)
                | (hasRotationRate ? 0x4 : 0x0)
        )
    }
}
