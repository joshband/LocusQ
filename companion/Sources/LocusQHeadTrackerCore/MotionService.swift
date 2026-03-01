import Foundation

public struct MotionSample: Sendable, Equatable {
    public let qx: Float
    public let qy: Float
    public let qz: Float
    public let qw: Float
    public let timestampMs: UInt64
    public let angVx: Float          // rad/s, body frame; 0 if unavailable
    public let angVy: Float
    public let angVz: Float
    public let sensorLocation: UInt8 // 0=unknown, 1=left, 2=right
    public let hasRotationRate: Bool

    public init(
        qx: Float, qy: Float, qz: Float, qw: Float,
        timestampMs: UInt64,
        angVx: Float = 0, angVy: Float = 0, angVz: Float = 0,
        sensorLocation: UInt8 = 0,
        hasRotationRate: Bool = false
    ) {
        self.qx = qx
        self.qy = qy
        self.qz = qz
        self.qw = qw
        self.timestampMs = timestampMs
        self.angVx = angVx
        self.angVy = angVy
        self.angVz = angVz
        self.sensorLocation = sensorLocation
        self.hasRotationRate = hasRotationRate
    }
}

public protocol MotionService: AnyObject {
    var onSample: ((MotionSample) -> Void)? { get set }
    func start() throws
    func stop()
}

public enum MotionServiceError: Error, CustomStringConvertible {
    case unavailable(String)

    public var description: String {
        switch self {
        case .unavailable(let message):
            return message
        }
    }
}

#if canImport(CoreMotion)
import CoreMotion

public final class HeadphoneMotionService: MotionService {
    private let manager = CMHeadphoneMotionManager()
    private let queue: OperationQueue = {
        let q = OperationQueue()
        q.name = "LocusQHeadTracker.Motion"
        q.qualityOfService = .userInitiated
        return q
    }()

    public var onSample: ((MotionSample) -> Void)?

    public init() {}

    public func start() throws {
        let status = CMHeadphoneMotionManager.authorizationStatus()
        guard status == .authorized || status == .notDetermined else {
            throw MotionServiceError.unavailable("CMHeadphoneMotionManager authorization denied/restricted")
        }

        guard manager.isDeviceMotionAvailable else {
            throw MotionServiceError.unavailable("No compatible headphone motion device available")
        }

        manager.startDeviceMotionUpdates(to: queue) { [weak self] motion, error in
            if let error {
                print("[LocusQHeadTracker] motion error: \(error)")
                return
            }
            guard let motion, let q = motion.attitude.quaternion as CMQuaternion? else { return }

            let ts = UInt64(Date().timeIntervalSince1970 * 1000.0)
            let rot = motion.rotationRate

            var loc: UInt8 = 0
            #if swift(>=5.5)
            if #available(macOS 12, iOS 15, *) {
                switch motion.sensorLocation {
                case .headphoneLeft:  loc = 1
                case .headphoneRight: loc = 2
                default:              loc = 0
                }
            }
            #endif

            let sample = MotionSample(
                qx: Float(q.x), qy: Float(q.y), qz: Float(q.z), qw: Float(q.w),
                timestampMs: ts,
                angVx: Float(rot.x), angVy: Float(rot.y), angVz: Float(rot.z),
                sensorLocation: loc,
                hasRotationRate: true
            )
            self?.onSample?(sample)
        }
    }

    public func stop() {
        manager.stopDeviceMotionUpdates()
    }
}
#else
public final class HeadphoneMotionService: MotionService {
    public var onSample: ((MotionSample) -> Void)?

    public init() {}

    public func start() throws {
        throw MotionServiceError.unavailable("CMHeadphoneMotionManager is unavailable on this platform; run on macOS with supported headphones.")
    }

    public func stop() {}
}
#endif
