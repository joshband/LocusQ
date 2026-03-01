import Foundation

public struct CalibrationPeqBand: Codable, Sendable, Equatable {
    public enum FilterType: String, Codable, Sendable {
        case pk = "PK"
        case lsc = "LSC"
        case hsc = "HSC"
    }

    public var type: FilterType
    public var fcHz: Float
    public var gainDb: Float
    public var q: Float

    enum CodingKeys: String, CodingKey {
        case type
        case fcHz = "fc_hz"
        case gainDb = "gain_db"
        case q
    }

    public init(type: FilterType, fcHz: Float, gainDb: Float, q: Float) {
        self.type = type
        self.fcHz = fcHz
        self.gainDb = gainDb
        self.q = q
    }
}

public struct CalibrationProfileUser: Codable, Sendable, Equatable {
    public var subjectId: String
    public var sofaRef: String
    public var embeddingHash: String

    enum CodingKeys: String, CodingKey {
        case subjectId = "subject_id"
        case sofaRef = "sofa_ref"
        case embeddingHash = "embedding_hash"
    }

    public init(subjectId: String, sofaRef: String, embeddingHash: String) {
        self.subjectId = subjectId
        self.sofaRef = sofaRef
        self.embeddingHash = embeddingHash
    }
}

public struct CalibrationProfileHeadphone: Codable, Sendable, Equatable {
    public enum ModelID: String, Codable, Sendable {
        case generic = "generic"
        case airpodsPro1 = "airpods_pro_1"
        case airpodsPro2 = "airpods_pro_2"
        case airpodsPro3 = "airpods_pro_3"
        case sonyWH1000XM5 = "sony_wh1000xm5"
        case customSOFA = "custom_sofa"
    }

    public enum Mode: String, Codable, Sendable {
        case ancOn = "anc_on"
        case ancOff = "anc_off"
        case `default` = "default"
    }

    public enum EqMode: String, Codable, Sendable {
        case off = "off"
        case peq = "peq"
        case fir = "fir"
    }

    public enum HrtfMode: String, Codable, Sendable {
        case `default` = "default"
        case sofa = "sofa"
    }

    public var hpModelId: ModelID
    public var hpMode: Mode
    public var hpEqMode: EqMode
    public var hpHrtfMode: HrtfMode
    public var hpPeqBands: [CalibrationPeqBand]
    public var hpFirTaps: [Float]

    enum CodingKeys: String, CodingKey {
        case hpModelId = "hp_model_id"
        case hpMode = "hp_mode"
        case hpEqMode = "hp_eq_mode"
        case hpHrtfMode = "hp_hrtf_mode"
        case hpPeqBands = "hp_peq_bands"
        case hpFirTaps = "hp_fir_taps"
    }

    public init(
        hpModelId: ModelID,
        hpMode: Mode,
        hpEqMode: EqMode,
        hpHrtfMode: HrtfMode,
        hpPeqBands: [CalibrationPeqBand],
        hpFirTaps: [Float]
    ) {
        self.hpModelId = hpModelId
        self.hpMode = hpMode
        self.hpEqMode = hpEqMode
        self.hpHrtfMode = hpHrtfMode
        self.hpPeqBands = hpPeqBands
        self.hpFirTaps = hpFirTaps
    }
}

public struct CalibrationProfileTracking: Codable, Sendable, Equatable {
    public var hpTrackingEnabled: Bool
    public var hpYawOffsetDeg: Float

    enum CodingKeys: String, CodingKey {
        case hpTrackingEnabled = "hp_tracking_enabled"
        case hpYawOffsetDeg = "hp_yaw_offset_deg"
    }

    public init(hpTrackingEnabled: Bool, hpYawOffsetDeg: Float) {
        self.hpTrackingEnabled = hpTrackingEnabled
        self.hpYawOffsetDeg = hpYawOffsetDeg
    }
}

public struct CalibrationProfileVerification: Codable, Sendable, Equatable {
    public var externalizationScore: Float?
    public var frontBackConfusionRate: Float?
    public var localizationAccuracy: Float?
    public var preferenceScore: Float?

    enum CodingKeys: String, CodingKey {
        case externalizationScore = "externalization_score"
        case frontBackConfusionRate = "front_back_confusion_rate"
        case localizationAccuracy = "localization_accuracy"
        case preferenceScore = "preference_score"
    }

    public init(
        externalizationScore: Float? = nil,
        frontBackConfusionRate: Float? = nil,
        localizationAccuracy: Float? = nil,
        preferenceScore: Float? = nil
    ) {
        self.externalizationScore = externalizationScore
        self.frontBackConfusionRate = frontBackConfusionRate
        self.localizationAccuracy = localizationAccuracy
        self.preferenceScore = preferenceScore
    }
}

public struct CalibrationProfile: Codable, Sendable, Equatable {
    public static let schemaV1 = "locusq-calibration-profile-v1"

    public var schema: String
    public var user: CalibrationProfileUser
    public var headphone: CalibrationProfileHeadphone
    public var tracking: CalibrationProfileTracking
    public var verification: CalibrationProfileVerification

    public init(
        schema: String = schemaV1,
        user: CalibrationProfileUser,
        headphone: CalibrationProfileHeadphone,
        tracking: CalibrationProfileTracking,
        verification: CalibrationProfileVerification
    ) {
        self.schema = schema
        self.user = user
        self.headphone = headphone
        self.tracking = tracking
        self.verification = verification
    }
}

public extension CalibrationProfile {
    static var defaultProfile: CalibrationProfile {
        CalibrationProfile(
            user: .init(
                subjectId: "H3",
                sofaRef: "sadie2/H3_HRIR.sofa",
                embeddingHash: ""
            ),
            headphone: .init(
                hpModelId: .generic,
                hpMode: .default,
                hpEqMode: .off,
                hpHrtfMode: .default,
                hpPeqBands: [],
                hpFirTaps: []
            ),
            tracking: .init(
                hpTrackingEnabled: false,
                hpYawOffsetDeg: 0.0
            ),
            verification: .init()
        )
    }

    static var profileURL: URL {
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        return appSupport
            .appending(path: "LocusQ", directoryHint: .isDirectory)
            .appending(path: "CalibrationProfile.json", directoryHint: .notDirectory)
    }

    static var pluginCompatibilityProfileURL: URL {
        let library = FileManager.default.urls(for: .libraryDirectory, in: .userDomainMask)[0]
        return library
            .appending(path: "LocusQ", directoryHint: .isDirectory)
            .appending(path: "CalibrationProfile.json", directoryHint: .notDirectory)
    }

    func writeToDisk(at url: URL = Self.profileURL) throws {
        try FileManager.default.createDirectory(
            at: url.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )

        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        let data = try encoder.encode(self)
        try data.write(to: url, options: .atomic)

        // Keep plugin and companion paths mirrored to avoid cross-process path drift.
        let mirrors = [Self.profileURL, Self.pluginCompatibilityProfileURL]
        for mirrorURL in mirrors where mirrorURL.path != url.path {
            try? FileManager.default.createDirectory(
                at: mirrorURL.deletingLastPathComponent(),
                withIntermediateDirectories: true
            )
            try? data.write(to: mirrorURL, options: .atomic)
        }
    }

    static func readFromDisk(at url: URL = Self.profileURL) -> CalibrationProfile? {
        var seenPaths = Set<String>()
        let candidates = [url, Self.profileURL, Self.pluginCompatibilityProfileURL]

        for candidate in candidates {
            guard seenPaths.insert(candidate.path).inserted else {
                continue
            }
            guard let data = try? Data(contentsOf: candidate) else {
                continue
            }
            if let decoded = try? JSONDecoder().decode(CalibrationProfile.self, from: data) {
                return decoded
            }
        }

        return nil
    }
}
