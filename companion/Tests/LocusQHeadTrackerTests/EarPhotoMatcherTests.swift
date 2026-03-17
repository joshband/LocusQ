import CoreGraphics
import Foundation
import XCTest
@testable import LocusQHeadTrackerCore

final class EarPhotoMatcherTests: XCTestCase {
    func testSingleImageMatchReturnsExactSubject() async throws {
        let image = try makeTestImage(seed: 19)
        let bootstrapMatcher = EarPhotoMatcher()
        let embedding = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: image))

        let matcher = EarPhotoMatcher(preloadedEmbeddings: [
            SubjectEmbeddingEntry(subjectId: "H7", embedding: embedding, sofaRef: "sadie2/H7_HRIR.sofa")
        ])

        let match = await matcher.match(captureImages: [image])
        XCTAssertEqual(match.subjectId, "H7")
        XCTAssertEqual(match.sofaRef, "sadie2/H7_HRIR.sofa")
        XCTAssertFalse(match.usedFallback)
        XCTAssertEqual(match.captureCount, 1)
    }

    func testMultiImageAverageStaysCloserToDominantSubject() async throws {
        let imageA = try makeTestImage(seed: 23)
        let imageB = try makeTestImage(seed: 211)
        let bootstrapMatcher = EarPhotoMatcher()
        let embeddingA = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: imageA))
        let embeddingB = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: imageB))

        let matcher = EarPhotoMatcher(preloadedEmbeddings: [
            SubjectEmbeddingEntry(subjectId: "A1", embedding: embeddingA, sofaRef: "sadie2/A1_HRIR.sofa"),
            SubjectEmbeddingEntry(subjectId: "B4", embedding: embeddingB, sofaRef: "sadie2/B4_HRIR.sofa")
        ])

        let match = await matcher.match(captureImages: [imageA, imageA, imageB])
        XCTAssertEqual(match.subjectId, "A1")
        XCTAssertFalse(match.usedFallback)
        XCTAssertEqual(match.captureCount, 3)
        XCTAssertGreaterThan(match.similarityScore, 0.6)
    }

    func testLowSimilarityFallsBackToDefaultSubject() async throws {
        let image = try makeTestImage(seed: 101)
        let bootstrapMatcher = EarPhotoMatcher()
        let embedding = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: image))

        var inverseEmbedding = embedding.map { -$0 }
        let norm = sqrt(inverseEmbedding.reduce(Float.zero) { partial, value in
            partial + (value * value)
        })
        for index in inverseEmbedding.indices {
            inverseEmbedding[index] /= norm
        }

        let matcher = EarPhotoMatcher(preloadedEmbeddings: [
            SubjectEmbeddingEntry(subjectId: "Z9", embedding: inverseEmbedding, sofaRef: "sadie2/Z9_HRIR.sofa")
        ])

        let match = await matcher.match(captureImages: [image], fallbackSimilarity: 0.95)
        XCTAssertEqual(match.subjectId, EarPhotoMatcher.fallbackSubjectId)
        XCTAssertEqual(match.sofaRef, EarPhotoMatcher.fallbackSofaRef)
        XCTAssertTrue(match.usedFallback)
    }

    func testEmbeddingHashIsStableForEquivalentInputs() async throws {
        let left = try makeTestImage(seed: 47)
        let right = try makeTestImage(seed: 89)
        let bootstrapMatcher = EarPhotoMatcher()
        let leftEmbedding = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: left))
        let rightEmbedding = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: right))

        let matcher = EarPhotoMatcher(preloadedEmbeddings: [
            SubjectEmbeddingEntry(subjectId: "L2", embedding: leftEmbedding, sofaRef: "sadie2/L2_HRIR.sofa"),
            SubjectEmbeddingEntry(subjectId: "R6", embedding: rightEmbedding, sofaRef: "sadie2/R6_HRIR.sofa")
        ])

        let first = await matcher.match(captureImages: [left, right])
        let second = await matcher.match(captureImages: [left, right])
        XCTAssertEqual(first.embeddingHash, second.embeddingHash)
        XCTAssertFalse(first.embeddingHash.isEmpty)
    }

    func testMatchEmbeddingsP90StaysUnderFiftyMilliseconds() throws {
        let image = try makeTestImage(seed: 313)
        let bootstrapMatcher = EarPhotoMatcher()
        let embedding = try XCTUnwrap(bootstrapMatcher.makeEmbedding(from: image))

        let matcher = EarPhotoMatcher(preloadedEmbeddings: [
            SubjectEmbeddingEntry(subjectId: "H9", embedding: embedding, sofaRef: "sadie2/H9_HRIR.sofa")
        ])

        let warmupIterations = 3
        for _ in 0..<warmupIterations {
            _ = matcher.matchEmbeddings([embedding, embedding, embedding])
        }

        let sampleIterations = 21
        var durationsMs: [Double] = []
        durationsMs.reserveCapacity(sampleIterations)
        let clock = ContinuousClock()

        for _ in 0..<sampleIterations {
            let start = clock.now
            let match = matcher.matchEmbeddings([embedding, embedding, embedding])
            let elapsed = start.duration(to: clock.now)

            XCTAssertEqual(match.subjectId, "H9")
            XCTAssertFalse(match.usedFallback)

            let elapsedMs = Double(elapsed.components.seconds) * 1_000.0
                + Double(elapsed.components.attoseconds) / 1_000_000_000_000_000.0
            durationsMs.append(elapsedMs)
        }

        let sortedDurations = durationsMs.sorted()
        let p90Index = Int(Double(sortedDurations.count - 1) * 0.9)
        let p90DurationMs = sortedDurations[p90Index]
        XCTAssertLessThan(
            p90DurationMs,
            50.0,
            "BL-058 nearest-neighbor match p90 should stay under 50ms; observed \(p90DurationMs) ms"
        )
    }

    private func makeTestImage(seed: UInt32,
                               width: Int = 32,
                               height: Int = 32) throws -> CGImage {
        var state = seed == 0 ? 1 : seed
        var pixels = [UInt8](repeating: 0, count: width * height)

        for index in pixels.indices {
            state = 1664525 &* state &+ 1013904223
            pixels[index] = UInt8(truncatingIfNeeded: state >> 24)
        }

        let colorSpace = CGColorSpaceCreateDeviceGray()
        let bytesPerRow = width
        guard let provider = CGDataProvider(data: Data(pixels) as CFData) else {
            throw NSError(domain: "EarPhotoMatcherTests", code: 1)
        }

        guard let image = CGImage(width: width,
                                  height: height,
                                  bitsPerComponent: 8,
                                  bitsPerPixel: 8,
                                  bytesPerRow: bytesPerRow,
                                  space: colorSpace,
                                  bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.none.rawValue),
                                  provider: provider,
                                  decode: nil,
                                  shouldInterpolate: true,
                                  intent: .defaultIntent) else {
            throw NSError(domain: "EarPhotoMatcherTests", code: 2)
        }

        return image
    }
}
