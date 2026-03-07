import XCTest
@testable import LocusQHeadTrackerCore

final class HeadTrackingOrientationTests: XCTestCase {
    func testMapCoreMotionAttitudeToSteamPosePreservesYawHeadingDirection() {
        let desiredPose = Quaternion.fromAxisAngle(
            axisX: 0.0,
            axisY: 1.0,
            axisZ: 0.0,
            radians: degreesToRadians(30.0)
        )

        let mappedPose = mapCoreMotionAttitudeToSteamPose(makeCoreMotionAttitude(forSteamPose: desiredPose))

        assertPose(mappedPose, matches: desiredPose)

        let mappedForward = rotate(vector: (0.0, 0.0, -1.0), by: mappedPose)
        XCTAssertLessThan(mappedForward.0, -0.1, "left yaw should point the forward axis toward -X")
    }

    func testMapCoreMotionAttitudeToSteamPosePreservesPitchDirection() {
        let desiredPose = Quaternion.fromAxisAngle(
            axisX: 1.0,
            axisY: 0.0,
            axisZ: 0.0,
            radians: degreesToRadians(-20.0)
        )

        let mappedPose = mapCoreMotionAttitudeToSteamPose(makeCoreMotionAttitude(forSteamPose: desiredPose))

        assertPose(mappedPose, matches: desiredPose)

        let mappedForward = rotate(vector: (0.0, 0.0, -1.0), by: mappedPose)
        XCTAssertLessThan(mappedForward.1, -0.1, "looking down should keep the forward axis pointed toward -Y")
    }

    private func makeCoreMotionAttitude(forSteamPose steamPose: Quaternion) -> Quaternion {
        let coreMotionToSteamBasis = Quaternion.fromAxisAngle(
            axisX: 1.0,
            axisY: 0.0,
            axisZ: 0.0,
            radians: Float.pi * 0.5
        )
        let steamToCoreMotionBasis = coreMotionToSteamBasis.conjugate().normalized()
        let steamAttitude = steamPose.conjugate().normalized()

        return steamToCoreMotionBasis
            .multiplied(by: steamAttitude)
            .multiplied(by: coreMotionToSteamBasis)
            .normalized()
    }

    private func assertPose(_ actual: Quaternion,
                            matches expected: Quaternion,
                            file: StaticString = #filePath,
                            line: UInt = #line) {
        let expectedForward = rotate(vector: (0.0, 0.0, -1.0), by: expected)
        let actualForward = rotate(vector: (0.0, 0.0, -1.0), by: actual)
        let expectedRight = rotate(vector: (1.0, 0.0, 0.0), by: expected)
        let actualRight = rotate(vector: (1.0, 0.0, 0.0), by: actual)
        let expectedUp = rotate(vector: (0.0, 1.0, 0.0), by: expected)
        let actualUp = rotate(vector: (0.0, 1.0, 0.0), by: actual)

        assertVector(actualForward, matches: expectedForward, file: file, line: line)
        assertVector(actualRight, matches: expectedRight, file: file, line: line)
        assertVector(actualUp, matches: expectedUp, file: file, line: line)
    }

    private func assertVector(_ actual: (Float, Float, Float),
                              matches expected: (Float, Float, Float),
                              file: StaticString = #filePath,
                              line: UInt = #line) {
        let tolerance: Float = 1.0e-4
        XCTAssertEqual(actual.0, expected.0, accuracy: tolerance, file: file, line: line)
        XCTAssertEqual(actual.1, expected.1, accuracy: tolerance, file: file, line: line)
        XCTAssertEqual(actual.2, expected.2, accuracy: tolerance, file: file, line: line)
    }

    private func rotate(vector: (Float, Float, Float), by quaternion: Quaternion) -> (Float, Float, Float) {
        let pure = Quaternion(x: vector.0, y: vector.1, z: vector.2, w: 0.0)
        let rotated = quaternion
            .multiplied(by: pure)
            .multiplied(by: quaternion.conjugate())

        return (rotated.x, rotated.y, rotated.z)
    }

    private func degreesToRadians(_ degrees: Float) -> Float {
        degrees * (Float.pi / 180.0)
    }
}
