import Foundation

#if canImport(CoreAudio)
import CoreAudio
#endif

public struct DetectedHeadphone: Sendable, Equatable {
    public enum ModelID: String, Sendable {
        case airpodsPro1 = "airpods_pro_1"
        case airpodsPro2 = "airpods_pro_2"
        case airpodsPro3 = "airpods_pro_3"
        case sonyWH1000XM5 = "sony_wh1000xm5"
        case generic = "generic"
    }

    public let modelId: ModelID
    public let displayName: String
    public let defaultMode: CalibrationProfileHeadphone.Mode

    public init(modelId: ModelID, displayName: String, defaultMode: CalibrationProfileHeadphone.Mode) {
        self.modelId = modelId
        self.displayName = displayName
        self.defaultMode = defaultMode
    }
}

public enum HeadphoneDeviceDetector {
    public static func detect() -> DetectedHeadphone {
        let deviceName = (currentOutputDeviceName() ?? "").trimmingCharacters(in: .whitespacesAndNewlines)
        let normalized = deviceName.lowercased()

        if normalized.contains("wh-1000xm5") || normalized.contains("wh1000xm5") {
            return DetectedHeadphone(
                modelId: .sonyWH1000XM5,
                displayName: "Sony WH-1000XM5",
                defaultMode: .ancOn
            )
        }

        if normalized.contains("airpods pro") {
            if normalized.contains("3rd generation") || normalized.contains("gen 3") || normalized.contains("pro 3") {
                return DetectedHeadphone(
                    modelId: .airpodsPro3,
                    displayName: "AirPods Pro (3rd gen)",
                    defaultMode: .ancOn
                )
            }

            if normalized.contains("2nd generation") || normalized.contains("gen 2") || normalized.contains("pro 2") {
                return DetectedHeadphone(
                    modelId: .airpodsPro2,
                    displayName: "AirPods Pro (2nd gen)",
                    defaultMode: .ancOn
                )
            }

            return DetectedHeadphone(
                modelId: .airpodsPro1,
                displayName: "AirPods Pro (1st gen)",
                defaultMode: .ancOn
            )
        }

        return DetectedHeadphone(
            modelId: .generic,
            displayName: deviceName.isEmpty ? "Generic Headphones" : deviceName,
            defaultMode: .default
        )
    }

    private static func currentOutputDeviceName() -> String? {
#if canImport(CoreAudio)
        var deviceID: AudioObjectID = kAudioObjectUnknown
        var size = UInt32(MemoryLayout<AudioObjectID>.size)
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )

        let status = AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &address,
            0,
            nil,
            &size,
            &deviceID
        )

        guard status == noErr, deviceID != kAudioObjectUnknown else {
            return nil
        }

        var nameAddress = AudioObjectPropertyAddress(
            mSelector: kAudioObjectPropertyName,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )
        var name = "" as CFString
        var nameSize = UInt32(MemoryLayout<CFString>.size)
        let nameStatus: OSStatus = withUnsafeMutablePointer(to: &name) { namePtr in
            AudioObjectGetPropertyData(
                deviceID,
                &nameAddress,
                0,
                nil,
                &nameSize,
                UnsafeMutableRawPointer(namePtr)
            )
        }

        guard nameStatus == noErr else {
            return nil
        }
        return name as String
#else
        return nil
#endif
    }
}
