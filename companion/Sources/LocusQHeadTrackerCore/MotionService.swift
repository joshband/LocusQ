import Foundation

public struct MotionSample: Sendable, Equatable {
    public let qx: Float
    public let qy: Float
    public let qz: Float
    public let qw: Float
    public let timestampMs: UInt64

    public init(qx: Float, qy: Float, qz: Float, qw: Float, timestampMs: UInt64) {
        self.qx = qx
        self.qy = qy
        self.qz = qz
        self.qw = qw
        self.timestampMs = timestampMs
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
            guard let q = motion?.attitude.quaternion else { return }

            let ts = UInt64(Date().timeIntervalSince1970 * 1000.0)
            let sample = MotionSample(qx: Float(q.x), qy: Float(q.y), qz: Float(q.z), qw: Float(q.w), timestampMs: ts)
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
