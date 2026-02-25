import Foundation

public final class TrackerApp {
    private let motionService: MotionService
    private let udpSender: UdpSender
    private var seq: UInt32 = 0

    public init(motionService: MotionService = HeadphoneMotionService(), udpSender: UdpSender) {
        self.motionService = motionService
        self.udpSender = udpSender
    }

    public func start() throws {
        motionService.onSample = { [weak self] sample in
            guard let self else { return }
            self.seq &+= 1

            let packet = PosePacket(
                qx: sample.qx,
                qy: sample.qy,
                qz: sample.qz,
                qw: sample.qw,
                timestampMs: sample.timestampMs,
                seq: self.seq
            )

            do {
                try self.udpSender.send(packet.serialize())
            } catch {
                print("[LocusQHeadTracker] UDP send failed: \(error)")
            }
        }

        try motionService.start()
    }

    public func stop() {
        motionService.stop()
    }
}
