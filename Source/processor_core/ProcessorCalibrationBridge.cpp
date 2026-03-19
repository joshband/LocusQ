#include "../PluginProcessor.h"
#include "../processor_bridge/ProcessorBridgeUtilities.h"
#include "ProcessorConstants.h"
#include "ProcessorParameterReaders.h"

#include <algorithm>

using namespace locusq::constants;

namespace
{
constexpr const char* kCalibrationProfileSchemaV1 = "locusq-calibration-profile-v1";

constexpr std::array<const char*, 11> kCalibrationTopologyIds
{
    "mono",
    "stereo",
    "quad",
    "surround_51",
    "surround_71",
    "surround_712",
    "surround_742",
    "binaural",
    "ambisonic_1st",
    "ambisonic_3rd",
    "downmix_stereo"
};

constexpr std::array<int, 11> kCalibrationTopologyRequiredChannels
{
    1, 2, 4, 6, 8, 10, 13, 2, 4, 16, 2
};

constexpr std::array<const char*, 4> kCalibrationMonitoringPathIds
{
    "speakers",
    "stereo_downmix",
    "steam_binaural",
    "virtual_binaural"
};

constexpr std::array<const char*, 5> kCalibrationDeviceProfileIds
{
    "generic",
    "airpods_pro_2",
    "airpods_pro_3",
    "sony_wh1000xm5",
    "custom_sofa"
};

constexpr std::array<const char*, 11> kCalibrationProfileParameterIds
{
    "cal_spk_config",
    "cal_topology_profile",
    "cal_monitoring_path",
    "cal_device_profile",
    "cal_mic_channel",
    "cal_spk1_out",
    "cal_spk2_out",
    "cal_spk3_out",
    "cal_spk4_out",
    "cal_test_level",
    "cal_test_type"
};

template <size_t N>
int indexOfCaseInsensitive (const std::array<const char*, N>& values, const juce::String& target)
{
    const auto normalised = target.trim().toLowerCase();
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (normalised == values[i])
            return static_cast<int> (i);
    }

    return -1;
}

juce::String calibrationTopologyIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationTopologyIds.size()) - 1, index);
    return kCalibrationTopologyIds[static_cast<size_t> (clamped)];
}

juce::String calibrationMonitoringPathIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1, index);
    return kCalibrationMonitoringPathIds[static_cast<size_t> (clamped)];
}

juce::String calibrationDeviceProfileIdForIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1, index);
    return kCalibrationDeviceProfileIds[static_cast<size_t> (clamped)];
}

int calibrationRequiredChannelsForTopologyIndex (int index)
{
    const auto clamped = juce::jlimit (0, static_cast<int> (kCalibrationTopologyRequiredChannels.size()) - 1, index);
    return kCalibrationTopologyRequiredChannels[static_cast<size_t> (clamped)];
}

int legacySpeakerConfigForTopologyIndex (int topologyIndex)
{
    const auto requiredChannels = calibrationRequiredChannelsForTopologyIndex (topologyIndex);
    return requiredChannels <= 2 ? 1 : 0;
}

int topologyProfileForOutputChannels (int outputChannels)
{
    const auto clampedChannels = juce::jlimit (1, 16, outputChannels);
    if (clampedChannels <= 1)
        return 0;
    if (clampedChannels == 2)
        return 1;
    if (clampedChannels == 6)
        return 3;
    if (clampedChannels == 8)
        return 4;
    if (clampedChannels == 10)
        return 5;
    if (clampedChannels >= 16)
        return 9;
    if (clampedChannels >= 13)
        return 6;

    return 2;
}

juce::File resolveCompanionCalibrationProfileFile()
{
    const auto overrideProfileFile = juce::SystemStats::getEnvironmentVariable (
        "LOCUSQ_COMPANION_PROFILE_FILE",
        {}).trim();
    if (overrideProfileFile.isNotEmpty())
        return juce::File (overrideProfileFile);

    const auto overrideProfileDir = juce::SystemStats::getEnvironmentVariable (
        "LOCUSQ_COMPANION_PROFILE_DIR",
        {}).trim();
    if (overrideProfileDir.isNotEmpty())
        return juce::File (overrideProfileDir).getChildFile ("CalibrationProfile.json");

    const auto userDataDir = locusq::processor_bridge::getLocusQUserDataDirectory();

    const std::array<juce::File, 2> candidates
    {
        userDataDir.getChildFile ("CalibrationProfile.json"),
        userDataDir.getChildFile ("Companion").getChildFile ("CalibrationProfile.json")
    };

    juce::File newestExisting;
    juce::int64 newestModifiedMs = 0;
    bool foundExisting = false;

    for (const auto& candidate : candidates)
    {
        if (! candidate.existsAsFile())
            continue;

        const auto modifiedMs = candidate.getLastModificationTime().toMilliseconds();
        if (! foundExisting || modifiedMs > newestModifiedMs)
        {
            newestExisting = candidate;
            newestModifiedMs = modifiedMs;
            foundExisting = true;
        }
    }

    if (foundExisting)
        return newestExisting;

    return candidates[0];
}

juce::String getOptionString (const juce::var& options,
                              std::initializer_list<const char*> propertyNames)
{
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        for (const auto* propertyName : propertyNames)
        {
            if (! optionsObject->hasProperty (propertyName))
                continue;

            const auto value = optionsObject->getProperty (propertyName).toString().trim();
            if (value.isNotEmpty())
                return value;
        }
    }

    return {};
}

struct CalibrationProfilePayloadMetadata
{
    juce::String name;
    juce::String topologyId { calibrationTopologyIdForIndex (1) };
    juce::String monitoringPathId { calibrationMonitoringPathIdForIndex (0) };
    juce::String deviceProfileId { calibrationDeviceProfileIdForIndex (0) };
    juce::var validationSummary;
    juce::var discoveryReconciliation;
    juce::var intentSummary;
};

CalibrationProfilePayloadMetadata extractCalibrationProfilePayloadMetadata (const juce::var& payload,
                                                                           const juce::String& fallbackName);

juce::String calibrationTopologySummaryLabel (const juce::String& topologyId)
{
    const auto normalized = topologyId.trim().toLowerCase();
    if (normalized == "mono") return "Mono";
    if (normalized == "stereo") return "Stereo";
    if (normalized == "quad") return "Quad";
    if (normalized == "surround_51") return "5.1";
    if (normalized == "surround_71") return "7.1";
    if (normalized == "surround_7_1_2") return "7.1.2";
    if (normalized == "surround_7_4_2") return "7.4.2";
    if (normalized == "hoa") return "HOA";
    if (normalized == "ambisonic") return "Ambisonic";
    if (normalized == "binaural") return "Binaural";
    return topologyId;
}

juce::String calibrationMonitoringSummaryLabel (const juce::String& monitoringPathId)
{
    const auto normalized = monitoringPathId.trim().toLowerCase();
    if (normalized == "speakers") return "Speakers";
    if (normalized == "stereo_downmix") return "Stereo Downmix";
    if (normalized == "steam_binaural") return "Steam Binaural";
    if (normalized == "virtual_binaural") return "Virtual Binaural";
    return monitoringPathId;
}

juce::String calibrationReconciliationPolicyLabel (const juce::String& policyId)
{
    const auto normalized = policyId.trim().toLowerCase();
    if (normalized == "defer") return "Defer Role";
    if (normalized == "fold_front_pair") return "Fold To Front Pair";
    if (normalized == "manual_reroute_later") return "Manual Reroute Later";
    return policyId;
}

juce::String buildCalibrationIntentSegment (const juce::var& source, const juce::String& label)
{
    auto* object = source.getDynamicObject();
    if (object == nullptr)
        return {};

    juce::StringArray pairs;
    for (const auto& property : object->getProperties())
    {
        const auto role = property.name.toString().trim().toUpperCase();
        const auto policy = calibrationReconciliationPolicyLabel (property.value.toString());
        if (role.isNotEmpty() && policy.isNotEmpty())
            pairs.add (role + "=" + policy);
    }

    if (pairs.isEmpty())
        return {};

    return label + ": " + pairs.joinIntoString ("; ") + ".";
}

juce::String buildCalibrationRemediationSegment (const juce::var& source, const juce::String& label)
{
    auto* object = source.getDynamicObject();
    if (object == nullptr)
        return {};

    juce::StringArray notes;
    for (const auto& property : object->getProperties())
    {
        const auto role = property.name.toString().trim().toUpperCase();
        const auto policyId = property.value.toString().trim().toLowerCase();
        if (role.isEmpty() || policyId.isEmpty())
            continue;

        if (policyId == "defer")
            notes.add (role + " still needs direct measurement or mapping before this layout is complete");
        else if (policyId == "fold_front_pair")
            notes.add (role + " is folded to the front pair and should be rerouted once more writable outputs are available");
        else if (policyId == "manual_reroute_later")
            notes.add (role + " still needs a manual reroute decision before full-layout playback");
    }

    if (notes.isEmpty())
        return {};

    return label + " remediation: " + notes.joinIntoString ("; ") + ".";
}

juce::var buildCalibrationRemediationActions (const juce::var& discoveryReconciliation)
{
    juce::Array<juce::var> actions;
    auto appendAction = [&actions] (const juce::String& id,
                                    const juce::String& label,
                                    const juce::String& targetCardId,
                                    const juce::String& statusMessage,
                                    const juce::String& policyType,
                                    const juce::StringArray& roles)
    {
        juce::var actionVar (new juce::DynamicObject());
        auto* action = actionVar.getDynamicObject();
        action->setProperty ("id", id);
        action->setProperty ("label", label);
        action->setProperty ("targetCardId", targetCardId);
        action->setProperty ("targetMode", "speaker_room");
        action->setProperty ("statusMessage", statusMessage);
        action->setProperty ("policyType", policyType);
        juce::Array<juce::var> roleArray;
        for (const auto& role : roles)
            roleArray.add (juce::var (role));
        action->setProperty ("roles", juce::var (roleArray));
        actions.add (actionVar);
    };

    auto collectRolesForPolicy = [&discoveryReconciliation] (const juce::String& policyId)
    {
        juce::StringArray roles;
        if (auto* reconciliation = discoveryReconciliation.getDynamicObject())
        {
            for (const auto* bucketName : { "output", "topology" })
            {
                if (auto* bucket = reconciliation->getProperty (bucketName).getDynamicObject())
                {
                    for (const auto& property : bucket->getProperties())
                    {
                        if (property.value.toString().trim().toLowerCase() == policyId)
                            roles.addIfNotAlreadyThere (property.name.toString().trim().toUpperCase());
                    }
                }
            }
        }
        return roles;
    };

    const auto manualRoles = collectRolesForPolicy ("manual_reroute_later");
    if (! manualRoles.isEmpty())
    {
        appendAction ("manual_reroute_roles",
                      "REROUTE ROLES NOW",
                      "cal-card-mapping",
                      "Manual reroute needed for "
                          + manualRoles.joinIntoString (", ")
                          + ". Open Output Mapping and assign unique outputs.",
                      "manual_reroute_later",
                      manualRoles);
    }

    const auto deferredRoles = collectRolesForPolicy ("defer");
    if (! deferredRoles.isEmpty())
    {
        appendAction ("revisit_deferred_roles",
                      "REVISIT DEFERRED ROLES",
                      "cal-card-discovery",
                      "Deferred roles "
                          + deferredRoles.joinIntoString (", ")
                          + " still need direct measurement or mapping before the layout is complete.",
                      "defer",
                      deferredRoles);
    }

    const auto foldedRoles = collectRolesForPolicy ("fold_front_pair");
    if (! foldedRoles.isEmpty())
    {
        appendAction ("use_wider_output_surface",
                      "USE WIDER OUTPUT SURFACE",
                      "cal-card-discovery",
                      "Folded roles "
                          + foldedRoles.joinIntoString (", ")
                          + " should be rerouted once more writable outputs are available.",
                      "fold_front_pair",
                      foldedRoles);
    }

    return juce::var (actions);
}

juce::var buildCalibrationProfileIntentSummary (const juce::String& topologyId,
                                                const juce::String& monitoringPathId,
                                                const juce::var& validationSummary,
                                                const juce::var& discoveryReconciliation)
{
    juce::var summaryVar (new juce::DynamicObject());
    auto* summary = summaryVar.getDynamicObject();

    const auto tupleLabel = calibrationTopologySummaryLabel (topologyId)
                            + " · "
                            + calibrationMonitoringSummaryLabel (monitoringPathId);
    summary->setProperty ("tupleLabel", tupleLabel);

    const auto profileReady = validationSummary.getDynamicObject() != nullptr
                              && static_cast<bool> (validationSummary.getProperty ("profileValid", false));
    summary->setProperty ("profileStateLabel", profileReady ? "Profile marked ready." : "Profile metadata available.");

    juce::String bestMapSummary;
    juce::String bestTopologySummary;
    juce::String bestMapRemediation;
    juce::String bestTopologyRemediation;
    if (auto* reconciliation = discoveryReconciliation.getDynamicObject())
    {
        bestMapSummary = buildCalibrationIntentSegment (reconciliation->getProperty ("output"), "Best Map");
        bestTopologySummary = buildCalibrationIntentSegment (reconciliation->getProperty ("topology"), "Best Topology");
        bestMapRemediation = buildCalibrationRemediationSegment (reconciliation->getProperty ("output"), "Best Map");
        bestTopologyRemediation = buildCalibrationRemediationSegment (reconciliation->getProperty ("topology"), "Best Topology");
    }

    if (bestMapSummary.isNotEmpty())
        summary->setProperty ("bestMapSummary", bestMapSummary);
    if (bestTopologySummary.isNotEmpty())
        summary->setProperty ("bestTopologySummary", bestTopologySummary);
    if (bestMapRemediation.isNotEmpty())
        summary->setProperty ("bestMapRemediation", bestMapRemediation);
    if (bestTopologyRemediation.isNotEmpty())
        summary->setProperty ("bestTopologyRemediation", bestTopologyRemediation);
    const auto remediationActions = buildCalibrationRemediationActions (discoveryReconciliation);
    if (const auto* actionArray = remediationActions.getArray(); actionArray != nullptr && ! actionArray->isEmpty())
        summary->setProperty ("remediationActions", remediationActions);

    juce::StringArray parts;
    parts.add (tupleLabel);
    parts.add (profileReady ? "Profile marked ready." : "Profile metadata available.");
    if (bestMapSummary.isNotEmpty())
        parts.add (bestMapSummary);
    if (bestTopologySummary.isNotEmpty())
        parts.add (bestTopologySummary);
    if (bestMapRemediation.isNotEmpty())
        parts.add (bestMapRemediation);
    if (bestTopologyRemediation.isNotEmpty())
        parts.add (bestTopologyRemediation);
    if (bestMapSummary.isEmpty() && bestTopologySummary.isEmpty())
        parts.add ("No saved role reconciliation notes for this profile.");
    summary->setProperty ("plainText", parts.joinIntoString (" "));
    return summaryVar;
}

void ensureCalibrationProfileIntentSummary (juce::var& payload,
                                            const juce::String& fallbackName)
{
    auto* profile = payload.getDynamicObject();
    if (profile == nullptr)
        return;

    if (profile->hasProperty ("intentSummary"))
        return;

    const auto metadata = extractCalibrationProfilePayloadMetadata (payload, fallbackName);
    profile->setProperty ("intentSummary",
                          buildCalibrationProfileIntentSummary (metadata.topologyId,
                                                                metadata.monitoringPathId,
                                                                metadata.validationSummary,
                                                                metadata.discoveryReconciliation));
}

CalibrationProfilePayloadMetadata extractCalibrationProfilePayloadMetadata (const juce::var& payload,
                                                                           const juce::String& fallbackName)
{
    CalibrationProfilePayloadMetadata metadata;
    metadata.name = fallbackName;

    if (auto* profile = payload.getDynamicObject())
    {
        if (profile->hasProperty ("name"))
            metadata.name = profile->getProperty ("name").toString().trim();

        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                metadata.topologyId = locusq::processor_bridge::normaliseCalibrationTopologyId (
                    context->getProperty ("topologyProfile").toString(),
                    kCalibrationTopologyIds,
                    [] (int index) { return calibrationTopologyIdForIndex (index); },
                    [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
            if (context->hasProperty ("monitoringPath"))
                metadata.monitoringPathId = locusq::processor_bridge::normaliseCalibrationMonitoringPathId (
                    context->getProperty ("monitoringPath").toString(),
                    kCalibrationMonitoringPathIds,
                    [] (int index) { return calibrationMonitoringPathIdForIndex (index); },
                    [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
            if (context->hasProperty ("deviceProfile"))
                metadata.deviceProfileId = locusq::processor_bridge::normaliseCalibrationDeviceProfileId (
                    context->getProperty ("deviceProfile").toString(),
                    kCalibrationDeviceProfileIds,
                    [] (int index) { return calibrationDeviceProfileIdForIndex (index); },
                    [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
        }

        if (profile->hasProperty ("validationSummary"))
            metadata.validationSummary = profile->getProperty ("validationSummary");
        if (profile->hasProperty ("discoveryReconciliation"))
            metadata.discoveryReconciliation = profile->getProperty ("discoveryReconciliation");
        if (profile->hasProperty ("intentSummary"))
            metadata.intentSummary = profile->getProperty ("intentSummary");
    }

    if (metadata.name.isEmpty())
        metadata.name = fallbackName;

    if (metadata.intentSummary.isVoid())
        metadata.intentSummary = buildCalibrationProfileIntentSummary (metadata.topologyId,
                                                                      metadata.monitoringPathId,
                                                                      metadata.validationSummary,
                                                                      metadata.discoveryReconciliation);

    return metadata;
}

bool isCalibrationProfilePayloadCompatible (const juce::var& payload)
{
    auto* profile = payload.getDynamicObject();
    if (profile == nullptr)
        return false;

    if (profile->hasProperty ("schema"))
    {
        const auto schema = profile->getProperty ("schema").toString().trim();
        if (schema.isNotEmpty() && schema != kCalibrationProfileSchemaV1)
            return false;
    }

    auto* controls = profile->getProperty ("controls").getDynamicObject();
    if (controls == nullptr || controls->getProperties().isEmpty())
        return false;

    for (const auto* parameterId : kCalibrationProfileParameterIds)
    {
        if (controls->hasProperty (parameterId))
            return true;
    }

    return false;
}

juce::File makeUniqueImportedCalibrationProfileFile (const juce::File& directory,
                                                     const juce::File& sourceFile,
                                                     const juce::String& requestedName,
                                                     juce::String& resolvedDisplayName)
{
    auto baseDisplayName = requestedName.trim();
    if (baseDisplayName.isEmpty())
        baseDisplayName = sourceFile.getFileNameWithoutExtension();

    auto candidateDisplayName = baseDisplayName;
    auto candidateFile = directory.getChildFile (locusq::processor_bridge::sanitisePresetName (candidateDisplayName) + ".json");

    if (candidateFile.getFullPathName() == sourceFile.getFullPathName() || ! candidateFile.existsAsFile())
    {
        resolvedDisplayName = candidateDisplayName;
        return candidateFile;
    }

    for (int suffix = 2; suffix < 1000; ++suffix)
    {
        candidateDisplayName = baseDisplayName + " (Imported " + juce::String (suffix) + ")";
        candidateFile = directory.getChildFile (locusq::processor_bridge::sanitisePresetName (candidateDisplayName) + ".json");
        if (candidateFile.getFullPathName() == sourceFile.getFullPathName() || ! candidateFile.existsAsFile())
        {
            resolvedDisplayName = candidateDisplayName;
            return candidateFile;
        }
    }

    resolvedDisplayName = baseDisplayName + " (Imported)";
    return directory.getChildFile (locusq::processor_bridge::sanitisePresetName (resolvedDisplayName)
                                   + "_" + juce::String (juce::Time::getCurrentTime().toMilliseconds())
                                   + ".json");
}

void populateCalibrationProfileResponse (juce::DynamicObject& result,
                                         const juce::var& payload,
                                         const juce::File& file,
                                         const juce::String& fallbackName)
{
    const auto metadata = extractCalibrationProfilePayloadMetadata (payload, fallbackName);

    result.setProperty ("name", metadata.name);
    result.setProperty ("file", file.getFileName());
    result.setProperty ("path", file.getFullPathName());
    result.setProperty ("topologyProfile", metadata.topologyId);
    result.setProperty ("monitoringPath", metadata.monitoringPathId);
    result.setProperty ("deviceProfile", metadata.deviceProfileId);
    result.setProperty ("profileTupleKey", metadata.topologyId + "::" + metadata.monitoringPathId);
    if (! metadata.validationSummary.isVoid())
        result.setProperty ("validationSummary", metadata.validationSummary);
    if (! metadata.discoveryReconciliation.isVoid())
        result.setProperty ("discoveryReconciliation", metadata.discoveryReconciliation);
    if (! metadata.intentSummary.isVoid())
        result.setProperty ("intentSummary", metadata.intentSummary);
}
} // namespace

juce::String LocusQAudioProcessor::normaliseCalibrationTopologyId (const juce::String& topologyId)
{
    return locusq::processor_bridge::normaliseCalibrationTopologyId (
        topologyId,
        kCalibrationTopologyIds,
        [] (int index) { return calibrationTopologyIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::normaliseCalibrationMonitoringPathId (const juce::String& monitoringPathId)
{
    return locusq::processor_bridge::normaliseCalibrationMonitoringPathId (
        monitoringPathId,
        kCalibrationMonitoringPathIds,
        [] (int index) { return calibrationMonitoringPathIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::String LocusQAudioProcessor::normaliseCalibrationDeviceProfileId (const juce::String& deviceProfileId)
{
    return locusq::processor_bridge::normaliseCalibrationDeviceProfileId (
        deviceProfileId,
        kCalibrationDeviceProfileIds,
        [] (int index) { return calibrationDeviceProfileIdForIndex (index); },
        [] (const auto& ids, const juce::String& value) { return indexOfCaseInsensitive (ids, value); });
}

juce::File LocusQAudioProcessor::getCalibrationProfileDirectory() const
{
    return locusq::processor_bridge::getUserDataSubdirectory ("CalibrationProfiles");
}

juce::File LocusQAudioProcessor::resolveCalibrationProfileFileFromOptions (const juce::var& options) const
{
    return locusq::processor_bridge::resolveNamedJsonFileFromOptions (
        options,
        getCalibrationProfileDirectory(),
        [] (const juce::String& name) { return locusq::processor_bridge::sanitisePresetName (name); });
}

std::array<int, SpatialRenderer::NUM_SPEAKERS> LocusQAudioProcessor::getCurrentCalibrationSpeakerRouting() const
{
    return locusq::processor_core::readCalibrationSpeakerRouting (apvts);
}

int LocusQAudioProcessor::getCurrentCalibrationSpeakerConfigIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (apvts,
                                                               "cal_spk_config",
                                                               0,
                                                               1,
                                                               0);
}

int LocusQAudioProcessor::getCurrentCalibrationTopologyProfileIndex() const
{
    if (apvts.getRawParameterValue ("cal_topology_profile") != nullptr)
    {
        return locusq::processor_core::readDiscreteParameterIndex (
            apvts,
            "cal_topology_profile",
            0,
            static_cast<int> (kCalibrationTopologyIds.size()) - 1,
            1);
    }

    const auto legacyConfig = getCurrentCalibrationSpeakerConfigIndex();
    return legacyConfig == 1 ? 1 : 2;
}

int LocusQAudioProcessor::getCurrentCalibrationMonitoringPathIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (
        apvts,
        "cal_monitoring_path",
        0,
        static_cast<int> (kCalibrationMonitoringPathIds.size()) - 1,
        0);
}

int LocusQAudioProcessor::getCurrentCalibrationDeviceProfileIndex() const
{
    return locusq::processor_core::readDiscreteParameterIndex (
        apvts,
        "cal_device_profile",
        0,
        static_cast<int> (kCalibrationDeviceProfileIds.size()) - 1,
        0);
}

int LocusQAudioProcessor::getRequiredCalibrationChannelsForTopologyIndex (int topologyIndex) const
{
    return calibrationRequiredChannelsForTopologyIndex (topologyIndex);
}

int LocusQAudioProcessor::resolveCalibrationWritableChannels (
    int snapshotOutputChannels,
    int layoutOutputChannels,
    int cachedAutoOutputChannels,
    const std::array<int, SpatialRenderer::NUM_SPEAKERS>& routing) noexcept
{
    const auto snapshot = juce::jlimit (1, SpatialRenderer::NUM_SPEAKERS, snapshotOutputChannels);
    const auto layout = juce::jlimit (0, SpatialRenderer::NUM_SPEAKERS, layoutOutputChannels);
    const auto cached = juce::jlimit (0, SpatialRenderer::NUM_SPEAKERS, cachedAutoOutputChannels);

    int effective = juce::jmax (snapshot, layout);

    if (effective <= 1)
    {
        const bool routingUsesMultipleOutputs = std::any_of (
            routing.begin(),
            routing.end(),
            [] (int channel) { return channel > 1; });

        if (routingUsesMultipleOutputs)
            effective = juce::jmax (effective, cached);
    }

    return juce::jlimit (1, SpatialRenderer::NUM_SPEAKERS, effective);
}

void LocusQAudioProcessor::applyAutoDetectedCalibrationRoutingIfAppropriate (int outputChannels, bool force)
{
    const auto clampedOutputChannels = juce::jlimit (1, 16, outputChannels);

    std::array<int, SpatialRenderer::NUM_SPEAKERS> autoRouting { 1, 2, 3, 4 };
    int autoSpeakerConfig = 0;
    int autoTopologyProfile = topologyProfileForOutputChannels (clampedOutputChannels);

    if (clampedOutputChannels == 1)
    {
        autoSpeakerConfig = 1;
        autoRouting = { 1, 1, 1, 1 };
    }
    else if (clampedOutputChannels == 2)
    {
        autoSpeakerConfig = 1;
        autoRouting = { 1, 2, 1, 2 };
    }
    else if (clampedOutputChannels == 3)
    {
        autoSpeakerConfig = 0;
        autoRouting = { 1, 2, 3, 3 };
    }

    const auto currentRouting = getCurrentCalibrationSpeakerRouting();
    const auto currentSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto currentTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto isFactoryMonoRouting = currentSpeakerConfig == 0
                                      && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 3, 4 };
    const auto isFactoryStereoRouting = currentSpeakerConfig == 1
                                        && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 1, 2 };
    const auto isFactoryMonoByChoice = currentSpeakerConfig == 0
                                       && currentRouting == std::array<int, SpatialRenderer::NUM_SPEAKERS> { 1, 2, 1, 2 };
    const auto isFactoryTopologyProfile = currentTopologyProfile == 2 || currentTopologyProfile == 1;
    const auto followsPreviousAuto = hasAppliedAutoDetectedCalibrationRouting
                                     && currentTopologyProfile == lastAutoDetectedTopologyProfile
                                     && currentSpeakerConfig == lastAutoDetectedSpeakerConfig
                                     && currentRouting == lastAutoDetectedSpeakerRouting;

    if (! force
        && ! followsPreviousAuto
        && ! isFactoryMonoRouting
        && ! isFactoryStereoRouting
        && ! isFactoryMonoByChoice
        && ! isFactoryTopologyProfile)
    {
        return;
    }

    if (hasAppliedAutoDetectedCalibrationRouting
        && clampedOutputChannels == lastAutoDetectedOutputChannels
        && autoTopologyProfile == lastAutoDetectedTopologyProfile
        && autoSpeakerConfig == lastAutoDetectedSpeakerConfig
        && autoRouting == lastAutoDetectedSpeakerRouting)
    {
        return;
    }

    setIntegerParameterValueNotifyingHost ("cal_topology_profile", autoTopologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", autoSpeakerConfig);
    setIntegerParameterValueNotifyingHost ("cal_spk1_out", autoRouting[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", autoRouting[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", autoRouting[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", autoRouting[3]);

    hasAppliedAutoDetectedCalibrationRouting = true;
    lastAutoDetectedOutputChannels = clampedOutputChannels;
    lastAutoDetectedTopologyProfile = autoTopologyProfile;
    lastAutoDetectedSpeakerConfig = autoSpeakerConfig;
    lastAutoDetectedSpeakerRouting = autoRouting;
}

void LocusQAudioProcessor::setIntegerParameterValueNotifyingHost (const char* parameterId, int value)
{
    locusq::processor_core::setIntegerParameterValueNotifyingHost (apvts, parameterId, value);
}

void LocusQAudioProcessor::migrateSnapshotLayoutIfNeeded (const juce::ValueTree& restoredState)
{
    int storedOutputChannels = 0;
    if (restoredState.hasProperty (kSnapshotOutputChannelsProperty))
    {
        storedOutputChannels = juce::jlimit (1,
                                             kMaxSnapshotOutputChannels,
                                             static_cast<int> (restoredState.getProperty (kSnapshotOutputChannelsProperty)));
    }
    else if (restoredState.hasProperty (kSnapshotOutputLayoutProperty))
    {
        const auto storedLayout = restoredState.getProperty (kSnapshotOutputLayoutProperty).toString().trim().toLowerCase();
        if (storedLayout == "mono")
            storedOutputChannels = 1;
        else if (storedLayout == "stereo")
            storedOutputChannels = 2;
        else if (storedLayout == "quad")
            storedOutputChannels = SpatialRenderer::NUM_SPEAKERS;
        else if (storedLayout == "surround_5_1")
            storedOutputChannels = 6;
        else if (storedLayout == "surround_5_2_1")
            storedOutputChannels = 8;
        else if (storedLayout == "surround_7_1")
            storedOutputChannels = 8;
        else if (storedLayout == "surround_7_2_1")
            storedOutputChannels = 10;
        else if (storedLayout == "surround_7_1_4")
            storedOutputChannels = 12;
        else if (storedLayout == "surround_7_4_2")
            storedOutputChannels = 13;
        else if (storedLayout == "multichannel")
            storedOutputChannels = juce::jmax (SpatialRenderer::NUM_SPEAKERS, storedOutputChannels);
    }

    const auto currentOutputChannels = juce::jlimit (1,
                                                     kMaxSnapshotOutputChannels,
                                                     getSnapshotOutputChannels());
    const auto isLegacySnapshot = ! restoredState.hasProperty (kSnapshotSchemaProperty);
    const auto hasLayoutMismatch = (storedOutputChannels > 0 && storedOutputChannels != currentOutputChannels);

    if (! isLegacySnapshot && ! hasLayoutMismatch)
        return;

    std::array<int, SpatialRenderer::NUM_SPEAKERS> migratedSpeakerMap { 1, 2, 3, 4 };
    int migratedSpeakerConfig = 0;
    const int migratedTopologyProfile = topologyProfileForOutputChannels (currentOutputChannels);

    if (currentOutputChannels == 1)
    {
        migratedSpeakerMap.fill (1);
        migratedSpeakerConfig = 1;
    }
    else if (currentOutputChannels == 2)
    {
        migratedSpeakerMap = { 1, 2, 1, 2 };
        migratedSpeakerConfig = 1;
    }

    setIntegerParameterValueNotifyingHost ("cal_topology_profile", migratedTopologyProfile);
    setIntegerParameterValueNotifyingHost ("cal_spk_config", migratedSpeakerConfig);
    setIntegerParameterValueNotifyingHost ("cal_spk1_out", migratedSpeakerMap[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", migratedSpeakerMap[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", migratedSpeakerMap[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", migratedSpeakerMap[3]);
}

juce::var LocusQAudioProcessor::buildCalibrationProfileState (const juce::String& profileName,
                                                              const juce::var& validationSummary,
                                                              const juce::var& discoveryReconciliation) const
{
    juce::var profileVar (new juce::DynamicObject());
    auto* profile = profileVar.getDynamicObject();

    profile->setProperty ("schema", kCalibrationProfileSchemaV1);
    profile->setProperty ("name", profileName);
    profile->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));

    juce::var contextVar (new juce::DynamicObject());
    auto* context = contextVar.getDynamicObject();
    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPathIndex = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfileIndex = getCurrentCalibrationDeviceProfileIndex();
    context->setProperty ("topologyProfileIndex", topologyIndex);
    context->setProperty ("topologyProfile", calibrationTopologyIdForIndex (topologyIndex));
    context->setProperty ("monitoringPathIndex", monitoringPathIndex);
    context->setProperty ("monitoringPath", calibrationMonitoringPathIdForIndex (monitoringPathIndex));
    context->setProperty ("deviceProfileIndex", deviceProfileIndex);
    context->setProperty ("deviceProfile", calibrationDeviceProfileIdForIndex (deviceProfileIndex));
    context->setProperty ("requiredChannels", getRequiredCalibrationChannelsForTopologyIndex (topologyIndex));
    context->setProperty ("writableChannels", resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting()));
    profile->setProperty ("context", contextVar);

    juce::var controlsVar (new juce::DynamicObject());
    auto* controls = controlsVar.getDynamicObject();
    for (const auto* parameterId : kCalibrationProfileParameterIds)
    {
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        {
            const auto scaledValue = parameter->convertFrom0to1 (parameter->getValue());
            controls->setProperty (parameterId, scaledValue);
        }
    }
    profile->setProperty ("controls", controlsVar);

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    profile->setProperty ("layout", layoutVar);

    if (! validationSummary.isVoid())
        profile->setProperty ("validationSummary", validationSummary);
    if (! discoveryReconciliation.isVoid())
        profile->setProperty ("discoveryReconciliation", discoveryReconciliation);
    profile->setProperty ("intentSummary",
                          buildCalibrationProfileIntentSummary (context->getProperty ("topologyProfile").toString(),
                                                                context->getProperty ("monitoringPath").toString(),
                                                                validationSummary,
                                                                discoveryReconciliation));

    return profileVar;
}

bool LocusQAudioProcessor::applyCalibrationProfileState (const juce::var& profileState)
{
    auto* profile = profileState.getDynamicObject();
    if (profile == nullptr)
        return false;

    if (profile->hasProperty ("schema"))
    {
        const auto schema = profile->getProperty ("schema").toString().trim();
        if (schema.isNotEmpty() && schema != kCalibrationProfileSchemaV1)
            return false;
    }

    auto* controls = profile->getProperty ("controls").getDynamicObject();
    if (controls == nullptr)
        return false;

    for (const auto& property : controls->getProperties())
    {
        const auto parameterId = property.name.toString();
        if (parameterId.isEmpty())
            continue;

        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        {
            const auto scaledValue = static_cast<float> (double (property.value));
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (scaledValue));
        }
    }

    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfile = getCurrentCalibrationDeviceProfileIndex();
    setIntegerParameterValueNotifyingHost ("cal_spk_config", legacySpeakerConfigForTopologyIndex (topologyIndex));
    setIntegerParameterValueNotifyingHost ("rend_headphone_mode", (monitoringPath == 2 || monitoringPath == 3) ? 1 : 0);
    setIntegerParameterValueNotifyingHost ("rend_headphone_profile", deviceProfile);

    int rendererSpatialProfileIndex = 0;
    switch (topologyIndex)
    {
        case 0: rendererSpatialProfileIndex = 1; break;
        case 1: rendererSpatialProfileIndex = 1; break;
        case 2: rendererSpatialProfileIndex = 2; break;
        case 3: rendererSpatialProfileIndex = 3; break;
        case 4: rendererSpatialProfileIndex = 4; break;
        case 5: rendererSpatialProfileIndex = 4; break;
        case 6: rendererSpatialProfileIndex = 5; break;
        case 7: rendererSpatialProfileIndex = 9; break;
        case 8: rendererSpatialProfileIndex = 6; break;
        case 9: rendererSpatialProfileIndex = 7; break;
        case 10: rendererSpatialProfileIndex = 9; break;
        default: break;
    }
    setIntegerParameterValueNotifyingHost ("rend_spatial_profile", rendererSpatialProfileIndex);

    return true;
}

juce::var LocusQAudioProcessor::listCalibrationProfilesFromUI() const
{
    juce::Array<juce::var> profiles;
    const auto profileDir = getCalibrationProfileDirectory();
    if (! profileDir.exists())
        return juce::var (profiles);

    juce::Array<juce::File> files;
    profileDir.findChildFiles (files, juce::File::findFiles, false, "*.json");
    std::sort (files.begin(), files.end(), [] (const juce::File& lhs, const juce::File& rhs)
    {
        return lhs.getLastModificationTime() > rhs.getLastModificationTime();
    });

    for (const auto& file : files)
    {
        juce::var entryVar (new juce::DynamicObject());
        auto* entry = entryVar.getDynamicObject();

        juce::String displayName = file.getFileNameWithoutExtension();
        juce::String topologyId = calibrationTopologyIdForIndex (1);
        juce::String monitoringPathId = calibrationMonitoringPathIdForIndex (0);
        juce::String deviceProfileId = calibrationDeviceProfileIdForIndex (0);
        juce::var validationSummary;
        juce::var discoveryReconciliation;
        juce::var intentSummary;

        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* profile = payload->getDynamicObject())
            {
                if (profile->hasProperty ("name"))
                    displayName = profile->getProperty ("name").toString();

                if (auto* context = profile->getProperty ("context").getDynamicObject())
                {
                    if (context->hasProperty ("topologyProfile"))
                        topologyId = normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString());
                    if (context->hasProperty ("monitoringPath"))
                        monitoringPathId = normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString());
                    if (context->hasProperty ("deviceProfile"))
                        deviceProfileId = normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString());
                }

                if (profile->hasProperty ("validationSummary"))
                    validationSummary = profile->getProperty ("validationSummary");
                if (profile->hasProperty ("discoveryReconciliation"))
                    discoveryReconciliation = profile->getProperty ("discoveryReconciliation");
                if (profile->hasProperty ("intentSummary"))
                    intentSummary = profile->getProperty ("intentSummary");
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        entry->setProperty ("topologyProfile", topologyId);
        entry->setProperty ("monitoringPath", monitoringPathId);
        entry->setProperty ("deviceProfile", deviceProfileId);
        entry->setProperty ("profileTupleKey", topologyId + "::" + monitoringPathId);
        if (! validationSummary.isVoid())
            entry->setProperty ("validationSummary", validationSummary);
        if (! discoveryReconciliation.isVoid())
            entry->setProperty ("discoveryReconciliation", discoveryReconciliation);
        if (! intentSummary.isVoid())
            entry->setProperty ("intentSummary", intentSummary);
        else
            entry->setProperty ("intentSummary",
                                buildCalibrationProfileIntentSummary (topologyId,
                                                                      monitoringPathId,
                                                                      validationSummary,
                                                                      discoveryReconciliation));
        profiles.add (entryVar);
    }

    return juce::var (profiles);
}

juce::var LocusQAudioProcessor::saveCalibrationProfileFromUI (const juce::var& options)
{
    juce::String requestedName;
    juce::var validationSummary;
    juce::var discoveryReconciliation;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("validationSummary"))
            validationSummary = optionsObject->getProperty ("validationSummary");
        if (optionsObject->hasProperty ("discoveryReconciliation"))
            discoveryReconciliation = optionsObject->getProperty ("discoveryReconciliation");
    }

    const auto topologyIndex = getCurrentCalibrationTopologyProfileIndex();
    const auto monitoringPathIndex = getCurrentCalibrationMonitoringPathIndex();
    const auto deviceProfileIndex = getCurrentCalibrationDeviceProfileIndex();
    const auto topologyId = calibrationTopologyIdForIndex (topologyIndex);
    const auto monitoringPathId = calibrationMonitoringPathIdForIndex (monitoringPathIndex);
    const auto deviceProfileId = calibrationDeviceProfileIdForIndex (deviceProfileIndex);

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = topologyId + "_" + monitoringPathId + "_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");

    const auto safeName = sanitisePresetName (requestedName);
    auto profileDir = getCalibrationProfileDirectory();
    profileDir.createDirectory();
    const auto profileFile = profileDir.getChildFile (safeName + ".json");
    const auto payload = buildCalibrationProfileState (requestedName, validationSummary, discoveryReconciliation);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! writeJsonToFile (profileFile, payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    result->setProperty ("topologyProfile", topologyId);
    result->setProperty ("monitoringPath", monitoringPathId);
    result->setProperty ("deviceProfile", deviceProfileId);
    result->setProperty ("profileTupleKey", topologyId + "::" + monitoringPathId);
    if (! validationSummary.isVoid())
        result->setProperty ("validationSummary", validationSummary);
    if (! discoveryReconciliation.isVoid())
        result->setProperty ("discoveryReconciliation", discoveryReconciliation);
    return response;
}

juce::var LocusQAudioProcessor::loadCalibrationProfileFromUI (const juce::var& options)
{
    const auto profileFile = resolveCalibrationProfileFileFromOptions (options);
    bool enforceTupleMatch = false;
    juce::String expectedTopologyId;
    juce::String expectedMonitoringPathId;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("enforceTupleMatch"))
            enforceTupleMatch = static_cast<bool> (optionsObject->getProperty ("enforceTupleMatch"));
        if (optionsObject->hasProperty ("topologyProfile"))
            expectedTopologyId = optionsObject->getProperty ("topologyProfile").toString();
        else if (optionsObject->hasProperty ("topologyProfileIndex"))
            expectedTopologyId = calibrationTopologyIdForIndex (static_cast<int> (optionsObject->getProperty ("topologyProfileIndex")));

        if (optionsObject->hasProperty ("monitoringPath"))
            expectedMonitoringPathId = optionsObject->getProperty ("monitoringPath").toString();
        else if (optionsObject->hasProperty ("monitoringPathIndex"))
            expectedMonitoringPathId = calibrationMonitoringPathIdForIndex (static_cast<int> (optionsObject->getProperty ("monitoringPathIndex")));
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! profileFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (profileFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    auto loadedTopologyId = calibrationTopologyIdForIndex (getCurrentCalibrationTopologyProfileIndex());
    auto loadedMonitoringPathId = calibrationMonitoringPathIdForIndex (getCurrentCalibrationMonitoringPathIndex());
    auto loadedDeviceProfileId = calibrationDeviceProfileIdForIndex (getCurrentCalibrationDeviceProfileIndex());
    if (auto* profile = payload->getDynamicObject())
    {
        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                loadedTopologyId = normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString());
            if (context->hasProperty ("monitoringPath"))
                loadedMonitoringPathId = normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString());
            if (context->hasProperty ("deviceProfile"))
                loadedDeviceProfileId = normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString());
        }
    }

    if (expectedTopologyId.isEmpty())
        expectedTopologyId = calibrationTopologyIdForIndex (getCurrentCalibrationTopologyProfileIndex());
    if (expectedMonitoringPathId.isEmpty())
        expectedMonitoringPathId = calibrationMonitoringPathIdForIndex (getCurrentCalibrationMonitoringPathIndex());
    expectedTopologyId = normaliseCalibrationTopologyId (expectedTopologyId);
    expectedMonitoringPathId = normaliseCalibrationMonitoringPathId (expectedMonitoringPathId);

    if (enforceTupleMatch
        && (loadedTopologyId != expectedTopologyId
            || loadedMonitoringPathId != expectedMonitoringPathId))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message",
                             "Calibration profile tuple mismatch (profile="
                                 + loadedTopologyId + "/"
                                 + loadedMonitoringPathId + ", current="
                                 + expectedTopologyId + "/"
                                 + expectedMonitoringPathId + ").");
        return response;
    }

    if (! applyCalibrationProfileState (*payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile payload is not compatible.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", profileFile.getFileNameWithoutExtension());
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    result->setProperty ("topologyProfile", loadedTopologyId);
    result->setProperty ("monitoringPath", loadedMonitoringPathId);
    result->setProperty ("deviceProfile", loadedDeviceProfileId);
    result->setProperty ("profileTupleKey", loadedTopologyId + "::" + loadedMonitoringPathId);
    if (auto* profile = payload->getDynamicObject())
    {
        if (profile->hasProperty ("name"))
            result->setProperty ("name", profile->getProperty ("name").toString());

        if (auto* context = profile->getProperty ("context").getDynamicObject())
        {
            if (context->hasProperty ("topologyProfile"))
                result->setProperty ("topologyProfile", normaliseCalibrationTopologyId (context->getProperty ("topologyProfile").toString()));
            if (context->hasProperty ("monitoringPath"))
                result->setProperty ("monitoringPath", normaliseCalibrationMonitoringPathId (context->getProperty ("monitoringPath").toString()));
            if (context->hasProperty ("deviceProfile"))
                result->setProperty ("deviceProfile", normaliseCalibrationDeviceProfileId (context->getProperty ("deviceProfile").toString()));
        }

        if (profile->hasProperty ("validationSummary"))
            result->setProperty ("validationSummary", profile->getProperty ("validationSummary"));
        if (profile->hasProperty ("discoveryReconciliation"))
            result->setProperty ("discoveryReconciliation", profile->getProperty ("discoveryReconciliation"));
    }

    return response;
}

juce::var LocusQAudioProcessor::renameCalibrationProfileFromUI (const juce::var& options)
{
    const auto sourceFile = resolveCalibrationProfileFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    juce::String requestedName;
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("newName"))
            requestedName = optionsObject->getProperty ("newName").toString();
        else if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile name is required.");
        return response;
    }

    const auto safeName = sanitisePresetName (requestedName);
    const auto destinationFile = getCalibrationProfileDirectory().getChildFile (safeName + ".json");
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    if (! samePath && destinationFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile name already exists.");
        return response;
    }

    auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    auto updatedPayload = *payload;
    if (auto* profile = updatedPayload.getDynamicObject(); profile != nullptr)
    {
        profile->setProperty ("name", requestedName);
        profile->setProperty ("updatedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    }

    if (! writeJsonToFile (destinationFile, updatedPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write calibration profile file.");
        return response;
    }

    if (! samePath)
        sourceFile.deleteFile();

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", destinationFile.getFileName());
    result->setProperty ("path", destinationFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::deleteCalibrationProfileFromUI (const juce::var& options)
{
    const auto profileFile = resolveCalibrationProfileFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! profileFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    if (! profileFile.deleteFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to delete calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("file", profileFile.getFileName());
    result->setProperty ("path", profileFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::exportCalibrationProfileFromUI (const juce::var& options) const
{
    const auto sourceFile = resolveCalibrationProfileFileFromOptions (options);
    const auto destinationPath = getOptionString (options, { "destinationPath", "exportPath" });

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file not found.");
        return response;
    }

    if (destinationPath.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile export destination is required.");
        return response;
    }

    auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile file is invalid JSON.");
        return response;
    }

    if (! isCalibrationProfilePayloadCompatible (*payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile payload is not compatible.");
        return response;
    }

    ensureCalibrationProfileIntentSummary (*payload, sourceFile.getFileNameWithoutExtension());

    auto destinationFile = juce::File (destinationPath);
    if (! destinationFile.hasFileExtension ("json"))
        destinationFile = destinationFile.withFileExtension (".json");

    const auto parentDirectory = destinationFile.getParentDirectory();
    if (parentDirectory != juce::File() && ! parentDirectory.exists() && ! parentDirectory.createDirectory())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to create export directory.");
        return response;
    }

    if (destinationFile.getFullPathName() != sourceFile.getFullPathName()
        && ! writeJsonToFile (destinationFile, *payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write exported calibration profile file.");
        return response;
    }

    result->setProperty ("ok", true);
    populateCalibrationProfileResponse (*result,
                                        *payload,
                                        destinationFile,
                                        sourceFile.getFileNameWithoutExtension());
    result->setProperty ("sourcePath", sourceFile.getFullPathName());
    result->setProperty ("exportPath", destinationFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::importCalibrationProfileFromUI (const juce::var& options)
{
    const auto sourcePath = getOptionString (options, { "sourcePath", "path", "importPath" });
    const auto sourceFile = juce::File (sourcePath);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (sourcePath.isEmpty())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile import source is required.");
        return response;
    }

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile import file not found.");
        return response;
    }

    auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile import file is invalid JSON.");
        return response;
    }

    if (! isCalibrationProfilePayloadCompatible (*payload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Calibration profile payload is not compatible.");
        return response;
    }

    ensureCalibrationProfileIntentSummary (*payload, sourceFile.getFileNameWithoutExtension());

    auto profileDirectory = getCalibrationProfileDirectory();
    if (! profileDirectory.exists() && ! profileDirectory.createDirectory())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to create calibration profile directory.");
        return response;
    }

    const auto metadata = extractCalibrationProfilePayloadMetadata (*payload,
                                                                   sourceFile.getFileNameWithoutExtension());
    juce::String resolvedDisplayName;
    const auto destinationFile = makeUniqueImportedCalibrationProfileFile (profileDirectory,
                                                                           sourceFile,
                                                                           metadata.name,
                                                                           resolvedDisplayName);
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    auto importedPayload = *payload;
    if (! samePath)
    {
        if (auto* profile = importedPayload.getDynamicObject(); profile != nullptr)
        {
            profile->setProperty ("name", resolvedDisplayName);
            profile->setProperty ("importedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
        }

        if (! writeJsonToFile (destinationFile, importedPayload))
        {
            result->setProperty ("ok", false);
            result->setProperty ("message", "Failed to write imported calibration profile file.");
            return response;
        }
    }

    const auto& resolvedPayload = samePath ? *payload : importedPayload;
    const auto resolvedFile = samePath ? sourceFile : destinationFile;
    result->setProperty ("ok", true);
    populateCalibrationProfileResponse (*result,
                                        resolvedPayload,
                                        resolvedFile,
                                        resolvedDisplayName.isNotEmpty()
                                            ? resolvedDisplayName
                                            : sourceFile.getFileNameWithoutExtension());
    result->setProperty ("sourcePath", sourceFile.getFullPathName());
    result->setProperty ("importedPath", resolvedFile.getFullPathName());
    result->setProperty ("importedFromLibrary", samePath);
    return response;
}

void LocusQAudioProcessor::pollCompanionCalibrationProfileFromDisk()
{
    const auto profileFile = resolveCompanionCalibrationProfileFile();

    if (! profileFile.existsAsFile())
    {
        if (companionCalibrationProfileLastModifiedMs != -1)
        {
            spatialRenderer.clearFirImpulseResponse();
            spatialRenderer.setHeadphoneCalibrationEnabled (false);
            spatialRenderer.setRequestedSofaHrtf ({}, false);
            pendingCompanionCalibrationRuntimeReload = true;
        }

        companionCalibrationProfileLastModifiedMs = -1;
        cachedCalibrationDevice = "unknown";
        cachedCalibrationEqMode = "off";
        cachedCalibrationHrtfMode = "default";
        cachedCalibrationSofaRef.clear();
        cachedCalibrationProfileSource = "unknown";
        cachedCalibrationHeadphoneProvenance = "unavailable";
        cachedCalibrationVerificationProvenance = "unavailable";
        cachedCalibrationRequestedSofa = false;
        cachedCalibrationTrackingEnabled = false;
        cachedCalibrationFirLatency = 0;
        cachedCalibrationProfileUpdatedAtUtcMs = 0;
        cachedExternalizationScore = -1.0f;
        cachedFrontBackConfusionRate = -1.0f;
        calibrationProfileTrackingEnabled.store (false, std::memory_order_relaxed);
        calibrationProfileYawOffsetDeg.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const auto modifiedMs = profileFile.getLastModificationTime().toMilliseconds();
    if (modifiedMs == companionCalibrationProfileLastModifiedMs)
        return;

    const auto cacheFailedRead = [&]() noexcept
    {
        companionCalibrationProfileLastModifiedMs = modifiedMs;
    };

    const auto payload = juce::JSON::parse (profileFile.loadFileAsString());
    auto* root = payload.getDynamicObject();
    if (root == nullptr)
    {
        cacheFailedRead();
        return;
    }

    const auto schema = root->getProperty ("schema").toString().trim();
    if (schema.isEmpty() || schema != kCalibrationProfileSchemaV1)
    {
        cacheFailedRead();
        return;
    }

    auto* headphone = root->getProperty ("headphone").getDynamicObject();
    if (headphone == nullptr)
    {
        cacheFailedRead();
        return;
    }

    auto* user = root->getProperty ("user").getDynamicObject();
    auto* provenance = root->getProperty ("provenance").getDynamicObject();

    const auto readProvenanceToken = [provenance] (const char* key, const juce::String& fallback) -> juce::String
    {
        if (provenance == nullptr)
            return fallback;
        const auto value = provenance->getProperty (key).toString().trim().toLowerCase();
        return value.isNotEmpty() ? value : fallback;
    };

    const auto readOptionalUtcMs = [provenance] (const char* key, std::int64_t fallback) -> std::int64_t
    {
        if (provenance == nullptr)
            return fallback;
        const auto value = provenance->getProperty (key);
        if (value.isInt() || value.isInt64() || value.isDouble())
            return static_cast<std::int64_t> (static_cast<double> (value));
        return fallback;
    };

    auto modelId = headphone->getProperty ("hp_model_id").toString().trim().toLowerCase();
    if (modelId.isEmpty())
        modelId = "generic";

    int profileIndex = 0;
    if (modelId == "airpods_pro_1" || modelId == "airpods_pro_2")
        profileIndex = 1;
    else if (modelId == "airpods_pro_3")
        profileIndex = 2;
    else if (modelId == "sony_wh1000xm5")
        profileIndex = 3;
    else if (modelId == "custom_sofa")
        profileIndex = 4;

    setIntegerParameterValueNotifyingHost ("cal_device_profile", profileIndex);
    setIntegerParameterValueNotifyingHost ("rend_headphone_profile", profileIndex);

    const auto eqMode = headphone->getProperty ("hp_eq_mode").toString().trim().toLowerCase();
    if (eqMode == "peq")
    {
        spatialRenderer.clearFirImpulseResponse();
        const auto bandsVar = headphone->getProperty ("hp_peq_bands");
        spatialRenderer.applyJsonPeqBands (bandsVar, 0.0f, currentSampleRate);
        spatialRenderer.setHeadphoneCalibrationEngine (1);
        spatialRenderer.setHeadphoneCalibrationEnabled (true);
    }
    else if (eqMode == "fir")
    {
        const auto firTapsVar = headphone->getProperty ("hp_fir_taps");
        const bool firLoaded = spatialRenderer.loadFirTapsFromJson (firTapsVar);
        spatialRenderer.setHeadphoneCalibrationEngine (2);
        spatialRenderer.setHeadphoneCalibrationEnabled (firLoaded);
        if (! firLoaded)
            DBG ("LocusQ: CalibrationProfile requested FIR EQ, but hp_fir_taps was empty or invalid.");
    }
    else
    {
        spatialRenderer.clearFirImpulseResponse();
        spatialRenderer.setHeadphoneCalibrationEnabled (false);
    }

    const auto hrtfMode = headphone->getProperty ("hp_hrtf_mode").toString().trim().toLowerCase();
    const auto sofaRef = user != nullptr ? user->getProperty ("sofa_ref").toString().trim() : juce::String {};
    const bool requestedSofaHrtf = hrtfMode == "sofa" && sofaRef.isNotEmpty();
    const bool sofaRequestChanged = requestedSofaHrtf != cachedCalibrationRequestedSofa
                                    || sofaRef != cachedCalibrationSofaRef;

    if (sofaRequestChanged)
    {
        spatialRenderer.setRequestedSofaHrtf (sofaRef, requestedSofaHrtf);
        pendingCompanionCalibrationRuntimeReload = true;
        cachedCalibrationRequestedSofa = requestedSofaHrtf;
        cachedCalibrationSofaRef = sofaRef;
    }

    {
        const juce::String modelIdForCache = headphone->getProperty ("hp_model_id").toString().trim().toLowerCase();
        if (modelIdForCache == "airpods_pro_1")
            cachedCalibrationDevice = "AirPods Pro (1st gen)";
        else if (modelIdForCache == "airpods_pro_2")
            cachedCalibrationDevice = "AirPods Pro (2nd gen)";
        else if (modelIdForCache == "airpods_pro_3")
            cachedCalibrationDevice = "AirPods Pro (3rd gen)";
        else if (modelIdForCache == "sony_wh1000xm5")
            cachedCalibrationDevice = "Sony WH-1000XM5";
        else if (modelIdForCache == "generic" || modelIdForCache.isEmpty())
            cachedCalibrationDevice = "Generic Headphones";
        else
            cachedCalibrationDevice = "Unknown Device";

        cachedCalibrationProfileSource = readProvenanceToken ("profile_source", "companion_estimated");
        cachedCalibrationHeadphoneProvenance = readProvenanceToken (
            "headphone_provenance",
            (modelIdForCache == "generic" || modelIdForCache.isEmpty()) ? "generic" : "detected");
        cachedCalibrationEqMode = eqMode.isEmpty() ? "off" : eqMode;
        cachedCalibrationHrtfMode = spatialRenderer.isUsingSofaHrtf() ? "sofa" : "default";

        cachedCalibrationFirLatency = spatialRenderer.getHeadphoneCalibrationLatencySamples();

        bool trackingEnabled = false;
        float trackingYawOffsetDeg = 0.0f;
        auto* tracking = root->getProperty ("tracking").getDynamicObject();
        if (tracking != nullptr)
        {
            const auto trackingVar = tracking->getProperty ("hp_tracking_enabled");
            trackingEnabled = trackingVar.isBool()
                ? static_cast<bool> (trackingVar)
                : trackingVar.toString().trim().toLowerCase() == "true";

            const auto yawOffsetVar = tracking->getProperty ("hp_yaw_offset_deg");
            if (yawOffsetVar.isDouble() || yawOffsetVar.isInt())
                trackingYawOffsetDeg = static_cast<float> (static_cast<double> (yawOffsetVar));
            else
                trackingYawOffsetDeg = yawOffsetVar.toString().getFloatValue();
        }
        trackingYawOffsetDeg = juce::jlimit (-180.0f, 180.0f, trackingYawOffsetDeg);
        cachedCalibrationTrackingEnabled = trackingEnabled;
        calibrationProfileTrackingEnabled.store (trackingEnabled, std::memory_order_relaxed);
        calibrationProfileYawOffsetDeg.store (trackingYawOffsetDeg, std::memory_order_relaxed);

        auto* verification = root->getProperty ("verification").getDynamicObject();
        if (verification != nullptr)
        {
            const auto extScoreVar = verification->getProperty ("externalization_score");
            if (extScoreVar.isDouble() || extScoreVar.isInt())
                cachedExternalizationScore = static_cast<float> (static_cast<double> (extScoreVar));

            const auto fbVar = verification->getProperty ("front_back_confusion_rate");
            if (fbVar.isDouble() || fbVar.isInt())
                cachedFrontBackConfusionRate = static_cast<float> (static_cast<double> (fbVar));
        }

        const bool verificationPresent = cachedExternalizationScore >= 0.0f
            || cachedFrontBackConfusionRate >= 0.0f;
        cachedCalibrationVerificationProvenance = readProvenanceToken (
            "verification_provenance",
            verificationPresent ? "estimated" : "unavailable");
        cachedCalibrationProfileUpdatedAtUtcMs = readOptionalUtcMs (
            "updated_at_utc_ms",
            static_cast<std::int64_t> (modifiedMs));
    }

    companionCalibrationProfileLastModifiedMs = modifiedMs;
}

void LocusQAudioProcessor::applyPendingCompanionCalibrationProfileReload()
{
    if (! pendingCompanionCalibrationRuntimeReload)
        return;

    const juce::ScopedLock callbackLock (getCallbackLock());
    spatialRenderer.reloadSteamAudioRuntime();
    pendingCompanionCalibrationRuntimeReload = false;
}
