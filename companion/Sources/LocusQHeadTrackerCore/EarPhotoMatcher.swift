import Foundation
import CoreGraphics
import CryptoKit

public struct SubjectMatch: Sendable, Equatable {
    public let subjectId: String
    public let similarityScore: Float
    public let sofaRef: String
    public let usedFallback: Bool
    public let embeddingHash: String
    public let captureCount: Int

    public init(subjectId: String,
                similarityScore: Float,
                sofaRef: String,
                usedFallback: Bool = false,
                embeddingHash: String = "",
                captureCount: Int = 1) {
        self.subjectId = subjectId
        self.similarityScore = similarityScore
        self.sofaRef = sofaRef
        self.usedFallback = usedFallback
        self.embeddingHash = embeddingHash
        self.captureCount = captureCount
    }
}

public struct SubjectEmbeddingEntry: Codable, Sendable, Equatable {
    public let subjectId: String
    public let embedding: [Float]
    public let sofaRef: String?

    enum CodingKeys: String, CodingKey {
        case subjectId = "subject_id"
        case embedding
        case sofaRef = "sofa_ref"
    }

    public init(subjectId: String, embedding: [Float], sofaRef: String? = nil) {
        self.subjectId = subjectId
        self.embedding = embedding
        self.sofaRef = sofaRef
    }
}

public final class EarPhotoMatcher {
    public static let fallbackSubjectId = "H3"
    public static let fallbackSofaRef = "sadie2/H3_HRIR.sofa"
    public static let defaultFallbackSimilarityThreshold: Float = 0.6

    private let subjectEmbeddings: [SubjectEmbeddingEntry]

    public init(embeddingsURL: URL? = nil, preloadedEmbeddings: [SubjectEmbeddingEntry]? = nil) {
        if let preloadedEmbeddings {
            self.subjectEmbeddings = preloadedEmbeddings
            return
        }

        if let entries = Self.loadEmbeddings(from: embeddingsURL) {
            self.subjectEmbeddings = entries
            return
        }

        self.subjectEmbeddings = [
            SubjectEmbeddingEntry(subjectId: Self.fallbackSubjectId, embedding: [], sofaRef: Self.fallbackSofaRef)
        ]
    }

    public func match(earImage: CGImage) async -> SubjectMatch {
        await match(captureImages: [earImage])
    }

    public func match(captureImages: [CGImage],
                      fallbackSimilarity threshold: Float = EarPhotoMatcher.defaultFallbackSimilarityThreshold) async -> SubjectMatch {
        let embeddings = captureImages.compactMap { makeEmbedding(from: $0) }
        return matchEmbeddings(embeddings, fallbackSimilarity: threshold)
    }

    public func matchEmbeddings(_ embeddings: [[Float]],
                                fallbackSimilarity threshold: Float = EarPhotoMatcher.defaultFallbackSimilarityThreshold) -> SubjectMatch {
        guard let combinedEmbedding = Self.combineEmbeddings(embeddings) else {
            return fallbackMatch(similarityScore: 0,
                                 captureCount: embeddings.count,
                                 embeddingHash: "")
        }

        let embeddingHash = Self.hashEmbedding(combinedEmbedding)
        var best: SubjectMatch?
        for entry in subjectEmbeddings where !entry.embedding.isEmpty {
            let score = cosineSimilarity(combinedEmbedding, entry.embedding)
            let sofaRef = entry.sofaRef ?? "sadie2/\(entry.subjectId)_HRIR.sofa"
            let candidate = SubjectMatch(subjectId: entry.subjectId,
                                         similarityScore: score,
                                         sofaRef: sofaRef,
                                         usedFallback: false,
                                         embeddingHash: embeddingHash,
                                         captureCount: embeddings.count)
            if let current = best {
                if score > current.similarityScore {
                    best = candidate
                }
            } else {
                best = candidate
            }
        }

        guard let best else {
            return fallbackMatch(similarityScore: 0,
                                 captureCount: embeddings.count,
                                 embeddingHash: embeddingHash)
        }

        if best.similarityScore < threshold {
            return fallbackMatch(similarityScore: best.similarityScore,
                                 captureCount: embeddings.count,
                                 embeddingHash: embeddingHash)
        }

        return best
    }

    private static func loadEmbeddings(from explicitURL: URL?) -> [SubjectEmbeddingEntry]? {
        let candidateURLs: [URL] = {
            if let explicitURL {
                return [explicitURL]
            }
            var urls: [URL] = []
            if let bundleURL = Bundle.main.url(forResource: "sadie2_embeddings", withExtension: "json") {
                urls.append(bundleURL)
            }
            let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
            if let appSupport {
                urls.append(appSupport.appending(path: "LocusQ/sadie2_embeddings.json", directoryHint: .notDirectory))
            }
            return urls
        }()

        for url in candidateURLs {
            guard let data = try? Data(contentsOf: url) else { continue }
            if let entries = try? JSONDecoder().decode([SubjectEmbeddingEntry].self, from: data) {
                return entries
            }
        }
        return nil
    }

    public func makeEmbedding(from image: CGImage) -> [Float]? {
        let width = 32
        let height = 32
        var pixels = [UInt8](repeating: 0, count: width * height)

        guard let context = CGContext(
            data: &pixels,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: width,
            space: CGColorSpaceCreateDeviceGray(),
            bitmapInfo: CGImageAlphaInfo.none.rawValue
        ) else {
            return nil
        }

        context.interpolationQuality = .high
        context.draw(image, in: CGRect(x: 0, y: 0, width: width, height: height))

        var embedding = pixels.map { Float($0) / 255.0 }
        Self.normalizeL2(&embedding)
        return embedding
    }

    private func cosineSimilarity(_ lhs: [Float], _ rhs: [Float]) -> Float {
        guard lhs.count == rhs.count, !lhs.isEmpty else { return 0 }
        var dot: Float = 0
        var lhsNorm: Float = 0
        var rhsNorm: Float = 0
        for index in 0..<lhs.count {
            let a = lhs[index]
            let b = rhs[index]
            dot += a * b
            lhsNorm += a * a
            rhsNorm += b * b
        }
        let denom = sqrt(lhsNorm) * sqrt(rhsNorm)
        return denom > 0 ? dot / denom : 0
    }

    private func fallbackMatch(similarityScore: Float,
                               captureCount: Int,
                               embeddingHash: String) -> SubjectMatch {
        SubjectMatch(subjectId: Self.fallbackSubjectId,
                     similarityScore: similarityScore,
                     sofaRef: Self.fallbackSofaRef,
                     usedFallback: true,
                     embeddingHash: embeddingHash,
                     captureCount: captureCount)
    }

    private static func combineEmbeddings(_ embeddings: [[Float]]) -> [Float]? {
        let validEmbeddings = embeddings.filter { !$0.isEmpty }
        guard let first = validEmbeddings.first else {
            return nil
        }

        let dimension = first.count
        guard dimension > 0 else {
            return nil
        }

        var combined = [Float](repeating: 0, count: dimension)
        var usedCount = 0

        for embedding in validEmbeddings where embedding.count == dimension {
            for index in 0..<dimension {
                combined[index] += embedding[index]
            }
            usedCount += 1
        }

        guard usedCount > 0 else {
            return nil
        }

        let scale = 1.0 / Float(usedCount)
        for index in 0..<dimension {
            combined[index] *= scale
        }
        normalizeL2(&combined)
        return combined
    }

    private static func hashEmbedding(_ embedding: [Float]) -> String {
        var bytes = Data(capacity: embedding.count * MemoryLayout<UInt32>.size)
        for value in embedding {
            var bitPattern = value.bitPattern.littleEndian
            Swift.withUnsafeBytes(of: &bitPattern) { rawBytes in
                bytes.append(contentsOf: rawBytes)
            }
        }

        let digest = SHA256.hash(data: bytes)
        return digest.prefix(12).map { String(format: "%02x", $0) }.joined()
    }

    private static func normalizeL2(_ values: inout [Float]) {
        var sum: Float = 0
        for value in values {
            sum += value * value
        }
        let norm = sqrt(sum)
        guard norm > 0 else { return }
        for index in values.indices {
            values[index] /= norm
        }
    }
}
