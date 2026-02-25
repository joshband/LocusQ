// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "LocusQHeadTracker",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .library(name: "LocusQHeadTrackerCore", targets: ["LocusQHeadTrackerCore"]),
        .executable(name: "LocusQHeadTracker", targets: ["LocusQHeadTracker"])
    ],
    targets: [
        .target(
            name: "LocusQHeadTrackerCore",
            path: "Sources/LocusQHeadTrackerCore"
        ),
        .executableTarget(
            name: "LocusQHeadTracker",
            dependencies: ["LocusQHeadTrackerCore"],
            path: "Sources/LocusQHeadTracker"
        ),
        .testTarget(
            name: "LocusQHeadTrackerTests",
            dependencies: ["LocusQHeadTrackerCore"],
            path: "Tests/LocusQHeadTrackerTests"
        )
    ]
)
