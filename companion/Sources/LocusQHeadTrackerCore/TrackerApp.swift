import Foundation

public final class TrackerApp {
    private let motionService: MotionService
    private let udpSender: UdpSender
    private var seq: UInt32

    public init(motionService: MotionService = HeadphoneMotionService(), udpSender: UdpSender) {
        self.motionService = motionService
        self.udpSender = udpSender
        var seed = UInt32((Date().timeIntervalSince1970 * 1000.0).rounded()) // truncating wrap is intentional
        if seed == 0 {
            seed = 1
        }
        self.seq = seed
    }

    public func start() throws {
        persistInitialCalibrationProfile()

        motionService.onSample = { [weak self] sample in
            guard let self else { return }
            self.seq &+= 1

            let flags: UInt32 = (UInt32(sample.sensorLocation) & 0x3)
                              | (sample.hasRotationRate ? 0x4 : 0x0)

            let packet = PosePacket(
                qx: sample.qx,
                qy: sample.qy,
                qz: sample.qz,
                qw: sample.qw,
                timestampMs: sample.timestampMs,
                seq: self.seq,
                angVx: sample.angVx,
                angVy: sample.angVy,
                angVz: sample.angVz,
                sensorLocationFlags: flags
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

    private func persistInitialCalibrationProfile() {
        // Read existing profile to preserve any user-set calibration data (PEQ bands,
        // HRTF subject, etc.). Only fall back to defaultProfile when no file exists yet.
        let existingProfile = CalibrationProfile.readFromDisk()
        var profile = existingProfile ?? CalibrationProfile.defaultProfile

        // Always refresh device detection so a newly connected headphone is reflected.
        let detected = HeadphoneDeviceDetector.detect()
        switch detected.modelId {
        case .airpodsPro1: profile.headphone.hpModelId = .airpodsPro1
        case .airpodsPro2: profile.headphone.hpModelId = .airpodsPro2
        case .airpodsPro3: profile.headphone.hpModelId = .airpodsPro3
        case .sonyWH1000XM5: profile.headphone.hpModelId = .sonyWH1000XM5
        case .generic: profile.headphone.hpModelId = .generic
        }
        profile.headphone.hpMode = detected.defaultMode

        // Tracking defaults to enabled only for AirPods Pro families when creating a
        // new profile; preserve the user's choice when updating an existing profile.
        if existingProfile == nil {
            profile.tracking.hpTrackingEnabled =
                profile.headphone.hpModelId == .airpodsPro1
                || profile.headphone.hpModelId == .airpodsPro2
                || profile.headphone.hpModelId == .airpodsPro3
        }

        do {
            try profile.writeToDisk()
        } catch {
            print("[LocusQHeadTracker] failed to persist CalibrationProfile.json: \(error)")
        }
    }
}
