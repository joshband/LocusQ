import Foundation

public struct Quaternion: Equatable, Sendable {
    public var x: Float
    public var y: Float
    public var z: Float
    public var w: Float

    public static let identity = Quaternion(x: 0, y: 0, z: 0, w: 1)

    public init(x: Float, y: Float, z: Float, w: Float) {
        self.x = x
        self.y = y
        self.z = z
        self.w = w
    }

    public static func fromAxisAngle(axisX: Float, axisY: Float, axisZ: Float, radians: Float) -> Quaternion {
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

    public func normalized() -> Quaternion {
        let length = sqrtf((x * x) + (y * y) + (z * z) + (w * w))
        guard length > 0 else { return .identity }
        return Quaternion(x: x / length, y: y / length, z: z / length, w: w / length)
    }

    public func conjugate() -> Quaternion {
        Quaternion(x: -x, y: -y, z: -z, w: w)
    }

    public func dot(_ other: Quaternion) -> Float {
        (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w)
    }

    public func multiplied(by other: Quaternion) -> Quaternion {
        Quaternion(
            x: (w * other.x) + (x * other.w) + (y * other.z) - (z * other.y),
            y: (w * other.y) - (x * other.z) + (y * other.w) + (z * other.x),
            z: (w * other.z) + (x * other.y) - (y * other.x) + (z * other.w),
            w: (w * other.w) - (x * other.x) - (y * other.y) - (z * other.z)
        )
    }

    public func nlerp(to target: Quaternion, alpha: Float) -> Quaternion {
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

    public func angularDistanceDeg(to other: Quaternion) -> Float {
        let d = abs(dot(other))
        let clamped = max(-1.0 as Float, min(1.0 as Float, d))
        let angle = 2.0 * acosf(clamped)
        return angle * 57.2957795
    }

    public func toEulerDegrees() -> (yaw: Float, pitch: Float, roll: Float) {
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

public func quaternionFromYawPitchRoll(yawDeg: Float, pitchDeg: Float, rollDeg: Float) -> Quaternion {
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

private let coreMotionToSteamBasis = Quaternion.fromAxisAngle(
    axisX: 1.0,
    axisY: 0.0,
    axisZ: 0.0,
    radians: Float.pi * 0.5
)

private let steamToCoreMotionBasis = coreMotionToSteamBasis.conjugate().normalized()

public func mapCoreMotionAttitudeToSteamPose(_ attitude: Quaternion) -> Quaternion {
    // CoreMotion attitude quaternions are reference->device in a +X right / +Y forward / +Z up frame.
    // LocusQ consumes a device pose in +X right / +Y up / -Z ahead, so after the basis remap we
    // conjugate into the device-pose direction and keep the result in canonical basis space.
    // Round-tripping through the Euler helper here inverts pitch because those labels do not map
    // one-to-one to the Steam listener basis.
    let steamAttitude = coreMotionToSteamBasis
        .multiplied(by: attitude)
        .multiplied(by: steamToCoreMotionBasis)
        .normalized()
    return steamAttitude.conjugate().normalized()
}
