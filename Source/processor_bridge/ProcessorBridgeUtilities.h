#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <optional>

namespace locusq::processor_bridge
{
inline juce::String sanitisePresetName (const juce::String& presetName)
{
    juce::String cleaned;
    for (const auto c : presetName.trim())
    {
        if (juce::CharacterFunctions::isLetterOrDigit (c)
            || c == '-'
            || c == '_'
            || c == ' ')
        {
            cleaned << c;
        }
    }

    cleaned = cleaned.trim();
    if (cleaned.isEmpty())
        cleaned = "Preset";

    return cleaned.replaceCharacter (' ', '_');
}

inline juce::String normalisePresetType (const juce::String& presetType,
                                         const juce::String& emitterPresetType,
                                         const juce::String& motionPresetType)
{
    const auto trimmed = presetType.trim().toLowerCase();
    if (trimmed == motionPresetType)
        return motionPresetType;

    return emitterPresetType;
}

template <size_t N>
inline juce::String normaliseChoreographyPackId (const juce::String& packId,
                                                 const std::array<const char*, N>& knownIds)
{
    const auto trimmed = packId.trim().toLowerCase();
    if (trimmed.isEmpty() || trimmed == "custom")
        return "custom";

    for (const auto* id : knownIds)
    {
        if (trimmed == id)
            return trimmed;
    }

    return "custom";
}

template <typename IdForIndexFn, typename IndexOfFn, size_t N>
inline juce::String normaliseCalibrationTopologyId (const juce::String& topologyId,
                                                    const std::array<const char*, N>& topologyIds,
                                                    IdForIndexFn idForIndex,
                                                    IndexOfFn indexOfCaseInsensitive)
{
    const auto trimmed = topologyId.trim().toLowerCase();
    if (trimmed.isEmpty())
        return idForIndex (1);

    if (const auto directIndex = indexOfCaseInsensitive (topologyIds, trimmed); directIndex >= 0)
        return idForIndex (directIndex);

    if (trimmed == "4x mono" || trimmed == "mono")
        return idForIndex (0);
    if (trimmed == "2x stereo" || trimmed == "stereo")
        return idForIndex (1);
    if (trimmed.contains ("quad"))
        return idForIndex (2);
    if (trimmed == "5.1" || trimmed.contains ("surround_5_1") || trimmed.contains ("surround_51"))
        return idForIndex (3);
    if (trimmed == "7.1.2" || trimmed.contains ("surround_7_1_2") || trimmed.contains ("surround_712"))
        return idForIndex (5);
    if (trimmed == "7.4.2" || trimmed.contains ("atmos") || trimmed.contains ("surround_7_4_2") || trimmed.contains ("surround_742"))
        return idForIndex (6);
    if (trimmed == "7.1" || trimmed.contains ("surround_7_1") || trimmed.contains ("surround_71"))
        return idForIndex (4);
    if (trimmed.contains ("binaural") || trimmed.contains ("headphone"))
        return idForIndex (7);
    if (trimmed.contains ("ambi") || trimmed.contains ("foa") || trimmed.contains ("hoa"))
    {
        if (trimmed.contains ("3rd") || trimmed.contains ("hoa"))
            return idForIndex (9);
        return idForIndex (8);
    }
    if (trimmed.contains ("downmix"))
        return idForIndex (10);

    return idForIndex (1);
}

template <typename IdForIndexFn, typename IndexOfFn, size_t N>
inline juce::String normaliseCalibrationMonitoringPathId (const juce::String& monitoringPathId,
                                                          const std::array<const char*, N>& monitoringIds,
                                                          IdForIndexFn idForIndex,
                                                          IndexOfFn indexOfCaseInsensitive)
{
    const auto trimmed = monitoringPathId.trim().toLowerCase();
    if (trimmed.isEmpty())
        return idForIndex (0);

    if (const auto directIndex = indexOfCaseInsensitive (monitoringIds, trimmed); directIndex >= 0)
        return idForIndex (directIndex);

    if (trimmed.contains ("speaker"))
        return idForIndex (0);
    if (trimmed.contains ("downmix"))
        return idForIndex (1);
    if (trimmed.contains ("steam") || trimmed.contains ("binaural"))
        return idForIndex (2);
    if (trimmed.contains ("virtual"))
        return idForIndex (3);

    return idForIndex (0);
}

template <typename IdForIndexFn, typename IndexOfFn, size_t N>
inline juce::String normaliseCalibrationDeviceProfileId (const juce::String& deviceProfileId,
                                                         const std::array<const char*, N>& deviceIds,
                                                         IdForIndexFn idForIndex,
                                                         IndexOfFn indexOfCaseInsensitive)
{
    const auto trimmed = deviceProfileId.trim().toLowerCase();
    if (trimmed.isEmpty())
        return idForIndex (0);

    if (const auto directIndex = indexOfCaseInsensitive (deviceIds, trimmed); directIndex >= 0)
        return idForIndex (directIndex);

    if (trimmed.contains ("airpods"))
        return idForIndex (1);
    if (trimmed.contains ("sony"))
        return idForIndex (2);
    if (trimmed.contains ("sofa") || trimmed.contains ("custom"))
        return idForIndex (3);

    return idForIndex (0);
}

inline juce::String inferPresetTypeFromPayload (const juce::var& payload,
                                                const juce::Identifier& typeProperty,
                                                const juce::String& emitterPresetType,
                                                const juce::String& motionPresetType)
{
    if (auto* preset = payload.getDynamicObject())
    {
        if (preset->hasProperty (typeProperty))
            return normalisePresetType (preset->getProperty (typeProperty).toString(),
                                        emitterPresetType,
                                        motionPresetType);

        const auto hasTimeline = preset->hasProperty ("timeline");
        const auto hasParameters = preset->getProperty ("parameters").isObject();
        if (hasTimeline && ! hasParameters)
            return motionPresetType;
    }

    return emitterPresetType;
}

inline juce::String sanitiseEmitterLabel (const juce::String& label, int maxChars = 31)
{
    auto cleaned = label.trim();
    juce::String filtered;
    for (const auto ch : cleaned)
    {
        if (juce::CharacterFunctions::isLetterOrDigit (ch)
            || ch == ' '
            || ch == '_'
            || ch == '-'
            || ch == '.'
            || ch == '('
            || ch == ')')
        {
            filtered << ch;
        }
    }

    if (filtered.isEmpty())
        filtered = "Emitter";

    if (filtered.length() > maxChars)
        filtered = filtered.substring (0, maxChars);

    return filtered.trim();
}

inline juce::File getUserDataSubdirectory (const juce::String& leafName)
{
    return juce::File::getSpecialLocation (juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile ("LocusQ")
        .getChildFile (leafName);
}

template <typename SanitiseNameFn>
inline juce::File resolveNamedJsonFileFromOptions (const juce::var& options,
                                                   const juce::File& baseDirectory,
                                                   SanitiseNameFn&& sanitiseName)
{
    juce::String payloadPath;
    juce::String payloadName;
    juce::String payloadFileName;

    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("path"))
            payloadPath = optionsObject->getProperty ("path").toString().trim();
        if (optionsObject->hasProperty ("name"))
            payloadName = optionsObject->getProperty ("name").toString().trim();
        if (optionsObject->hasProperty ("file"))
            payloadFileName = optionsObject->getProperty ("file").toString().trim();
    }

    if (payloadPath.isNotEmpty())
        return juce::File (payloadPath);

    if (payloadFileName.isNotEmpty())
        return baseDirectory.getChildFile (juce::File (payloadFileName).getFileName());

    if (payloadName.isNotEmpty())
        return baseDirectory.getChildFile (sanitiseName (payloadName) + ".json");

    return {};
}

inline std::optional<juce::var> readJsonFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return std::nullopt;

    const auto payload = juce::JSON::parse (file.loadFileAsString());
    if (payload.isVoid())
        return std::nullopt;

    return payload;
}

inline bool writeJsonToFile (const juce::File& file, const juce::var& payload)
{
    return file.replaceWithText (juce::JSON::toString (payload, true));
}
} // namespace locusq::processor_bridge

