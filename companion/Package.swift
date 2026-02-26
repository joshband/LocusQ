// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "LocusQHeadTrackingCompanion",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .library(
            name: "LocusQHeadTrackerCore",
            targets: ["LocusQHeadTrackerCore"]
        ),
        .executable(
            name: "locusq-headtrack-companion",
            targets: ["LocusQHeadTrackingCompanion"]
        )
    ],
    targets: [
        .target(
            name: "LocusQHeadTrackerCore",
            path: "Sources/LocusQHeadTrackerCore"
        ),
        .executableTarget(
            name: "LocusQHeadTrackingCompanion"
        ),
        .testTarget(
            name: "LocusQHeadTrackerTests",
            dependencies: ["LocusQHeadTrackerCore"],
            path: "Tests/LocusQHeadTrackerTests"
        )
    ]
)
