// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "LocusQHeadTrackingCompanion",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(
            name: "locusq-headtrack-companion",
            targets: ["LocusQHeadTrackingCompanion"]
        )
    ],
    targets: [
        .executableTarget(
            name: "LocusQHeadTrackingCompanion"
        )
    ]
)
