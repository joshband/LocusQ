import Foundation
import CoreGraphics

public struct SubjectMatch: Sendable, Equatable {
    public let subjectId: String
    public let similarityScore: Float
    public let sofaRef: String

    public init(subjectId: String, similarityScore: Float, sofaRef: String) {
        self.subjectId = subjectId
        self.similarityScore = similarityScore
        self.sofaRef = sofaRef
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
            SubjectEmbeddingEntry(subjectId: "H3", embedding: [], sofaRef: "sadie2/H3_HRIR.sofa")
        ]
    }

    public func match(earImage: CGImage) async -> SubjectMatch {
        guard let embedding = computeEmbedding(from: earImage), !embedding.isEmpty else {
            return SubjectMatch(subjectId: "H3", similarityScore: 0, sofaRef: "sadie2/H3_HRIR.sofa")
        }

        var best: SubjectMatch?
        for entry in subjectEmbeddings where !entry.embedding.isEmpty {
            let score = cosineSimilarity(embedding, entry.embedding)
            let sofaRef = entry.sofaRef ?? "sadie2/\(entry.subjectId)_HRIR.sofa"
            if let current = best {
                if score > current.similarityScore {
                    best = SubjectMatch(subjectId: entry.subjectId, similarityScore: score, sofaRef: sofaRef)
                }
            } else {
                best = SubjectMatch(subjectId: entry.subjectId, similarityScore: score, sofaRef: sofaRef)
            }
        }

        return best ?? SubjectMatch(subjectId: "H3", similarityScore: 0, sofaRef: "sadie2/H3_HRIR.sofa")
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

    private func computeEmbedding(from image: CGImage) -> [Float]? {
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
        normalizeL2(&embedding)
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

    private func normalizeL2(_ values: inout [Float]) {
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
