#include "../PluginProcessor.h"
#include "../processor_bridge/ProcessorBridgeUtilities.h"
#include "../shared_contracts/BridgeStatusContract.h"
#include "ProcessorParameterReaders.h"

#include <cmath>
#include <cstdio>

namespace
{
constexpr const char* kEmitterPresetSchemaV1 = "locusq-emitter-preset-v1";
constexpr const char* kEmitterPresetSchemaV2 = "locusq-emitter-preset-v2";
constexpr const char* kEmitterPresetLayoutProperty = "layout";
constexpr const char* kEmitterPresetTypeProperty = "presetType";
constexpr const char* kEmitterPresetTypeEmitter = "emitter";
constexpr const char* kEmitterPresetTypeMotion = "motion";

constexpr std::array<const char*, 35> kEmitterPresetParameterIds
{
    "pos_azimuth", "pos_elevation", "pos_distance",
    "pos_x", "pos_y", "pos_z", "pos_coord_mode",
    "size_width", "size_depth", "size_height", "size_link", "size_uniform",
    "emit_gain", "emit_mute", "emit_solo", "emit_spread", "emit_directivity",
    "emit_dir_azimuth", "emit_dir_elevation", "emit_color",
    "phys_enable", "phys_mass", "phys_drag", "phys_elasticity",
    "phys_gravity", "phys_gravity_dir", "phys_friction",
    "phys_vel_x", "phys_vel_y", "phys_vel_z",
    "anim_enable", "anim_mode", "anim_loop", "anim_speed", "anim_sync"
};

constexpr std::array<const char*, 5> kCurveNames
{
    "linear",
    "easeIn",
    "easeOut",
    "easeInOut",
    "step"
};

constexpr std::array<const char*, 4> kChoreographyPackIds
{
    "orbit",
    "pendulum",
    "swarm_arc",
    "rise_fall"
};

juce::String outputLayoutToString (const juce::AudioChannelSet& outputSet)
{
    if (outputSet == juce::AudioChannelSet::mono())
        return "mono";
    if (outputSet == juce::AudioChannelSet::stereo())
        return "stereo";
    if (outputSet == juce::AudioChannelSet::quadraphonic()
        || outputSet == juce::AudioChannelSet::discreteChannels (4))
    {
        return "quad";
    }

    if (outputSet == juce::AudioChannelSet::create5point1())
        return "surround_5_1";

    if (outputSet == juce::AudioChannelSet::discreteChannels (8))
        return "surround_5_2_1";

    if (outputSet == juce::AudioChannelSet::create7point1())
        return "surround_7_1";

    if (outputSet == juce::AudioChannelSet::discreteChannels (10))
        return "surround_7_2_1";

    if (outputSet == juce::AudioChannelSet::create7point1point4())
        return "surround_7_1_4";

    if (outputSet == juce::AudioChannelSet::discreteChannels (13))
        return "surround_7_4_2";

    if (outputSet.size() >= SpatialRenderer::NUM_SPEAKERS)
        return "multichannel";

    return "other";
}
} // namespace

juce::String LocusQAudioProcessor::sanitisePresetName (const juce::String& presetName)
{
    return locusq::processor_bridge::sanitisePresetName (presetName);
}

juce::String LocusQAudioProcessor::normalisePresetType (const juce::String& presetType)
{
    return locusq::processor_bridge::normalisePresetType (presetType,
                                                          kEmitterPresetTypeEmitter,
                                                          kEmitterPresetTypeMotion);
}

juce::String LocusQAudioProcessor::normaliseChoreographyPackId (const juce::String& packId)
{
    return locusq::processor_bridge::normaliseChoreographyPackId (packId, kChoreographyPackIds);
}

juce::String LocusQAudioProcessor::inferPresetTypeFromPayload (const juce::var& payload)
{
    return locusq::processor_bridge::inferPresetTypeFromPayload (payload,
                                                                 kEmitterPresetTypeProperty,
                                                                 kEmitterPresetTypeEmitter,
                                                                 kEmitterPresetTypeMotion);
}

juce::String LocusQAudioProcessor::sanitiseEmitterLabel (const juce::String& label)
{
    return locusq::processor_bridge::sanitiseEmitterLabel (label);
}

juce::File LocusQAudioProcessor::getPresetDirectory() const
{
    return locusq::processor_bridge::getUserDataSubdirectory ("Presets");
}

juce::File LocusQAudioProcessor::resolvePresetFileFromOptions (const juce::var& options) const
{
    return locusq::processor_bridge::resolveNamedJsonFileFromOptions (
        options,
        getPresetDirectory(),
        [] (const juce::String& name) { return locusq::processor_bridge::sanitisePresetName (name); });
}

juce::String LocusQAudioProcessor::getSnapshotOutputLayout() const
{
    return outputLayoutToString (getBusesLayout().getMainOutputChannelSet());
}

int LocusQAudioProcessor::getSnapshotOutputChannels() const
{
    return locusq::processor_core::readSnapshotOutputChannels (getMainBusNumOutputChannels(),
                                                               getTotalNumOutputChannels());
}

juce::String LocusQAudioProcessor::keyframeCurveToString (KeyframeCurve curve)
{
    const auto index = static_cast<size_t> (juce::jlimit (0, static_cast<int> (kCurveNames.size()) - 1, static_cast<int> (curve)));
    return juce::String (kCurveNames[index]);
}

KeyframeCurve LocusQAudioProcessor::keyframeCurveFromVar (const juce::var& value)
{
    if (value.isInt() || value.isInt64() || value.isDouble())
        return static_cast<KeyframeCurve> (juce::jlimit (0, static_cast<int> (kCurveNames.size()) - 1, static_cast<int> (value)));

    const auto text = value.toString().trim();
    for (size_t i = 0; i < kCurveNames.size(); ++i)
    {
        if (text.equalsIgnoreCase (kCurveNames[i]))
            return static_cast<KeyframeCurve> (i);
    }

    return KeyframeCurve::linear;
}

std::optional<juce::var> LocusQAudioProcessor::readJsonFromFile (const juce::File& file)
{
    return locusq::processor_bridge::readJsonFromFile (file);
}

bool LocusQAudioProcessor::writeJsonToFile (const juce::File& file, const juce::var& payload)
{
    return locusq::processor_bridge::writeJsonToFile (file, payload);
}

void LocusQAudioProcessor::applyEmitterLabelToSceneSlotIfAvailable (const juce::String& label)
{
    if (emitterSlotId < 0 || ! sceneGraph.isSlotActive (emitterSlotId))
        return;

    auto data = sceneGraph.getSlot (emitterSlotId).read();
    const auto sanitised = sanitiseEmitterLabel (label);
    std::snprintf (data.label, sizeof (data.label), "%s", sanitised.toRawUTF8());
    sceneGraph.getSlot (emitterSlotId).write (data);
}

juce::var LocusQAudioProcessor::buildEmitterPresetLocked (const juce::String& presetName,
                                                          const juce::String& presetType,
                                                          const juce::String& choreographyPackId,
                                                          bool includeParameters,
                                                          bool includeTimeline) const
{
    juce::var presetVar (new juce::DynamicObject());
    auto* preset = presetVar.getDynamicObject();

    preset->setProperty ("schema", kEmitterPresetSchemaV2);
    preset->setProperty ("name", presetName);
    preset->setProperty (kEmitterPresetTypeProperty, normalisePresetType (presetType));
    preset->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    preset->setProperty ("choreographyPackId", normaliseChoreographyPackId (choreographyPackId));

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    preset->setProperty (kEmitterPresetLayoutProperty, layoutVar);

    if (includeParameters)
        preset->setProperty ("parameters", captureEmitterParameterState());

    if (includeTimeline)
        preset->setProperty ("timeline", serialiseKeyframeTimelineLocked());

    return presetVar;
}

juce::var LocusQAudioProcessor::captureEmitterParameterState() const
{
    juce::var parametersVar (new juce::DynamicObject());
    auto* parameters = parametersVar.getDynamicObject();

    for (const auto* parameterId : kEmitterPresetParameterIds)
    {
        if (auto* parameter = apvts.getParameter (parameterId))
            parameters->setProperty (parameterId, parameter->getValue());
    }

    return parametersVar;
}

bool LocusQAudioProcessor::applyEmitterParameterState (const juce::var& parametersState)
{
    auto* parameters = parametersState.getDynamicObject();
    if (parameters == nullptr)
        return false;

    for (const auto* parameterId : kEmitterPresetParameterIds)
    {
        if (! parameters->hasProperty (parameterId))
            continue;

        if (auto* parameter = apvts.getParameter (parameterId))
        {
            const auto normalized = juce::jlimit (0.0f,
                                                  1.0f,
                                                  static_cast<float> (double (parameters->getProperty (parameterId))));
            parameter->setValueNotifyingHost (normalized);
        }
    }

    return true;
}

bool LocusQAudioProcessor::applyEmitterPresetLocked (const juce::var& presetState)
{
    auto* preset = presetState.getDynamicObject();
    if (preset == nullptr)
        return false;

    if (preset->hasProperty ("schema"))
    {
        const auto schema = preset->getProperty ("schema").toString();
        if (schema.isNotEmpty()
            && schema != kEmitterPresetSchemaV1
            && schema != kEmitterPresetSchemaV2)
        {
            return false;
        }
    }

    if (auto* layout = preset->getProperty (kEmitterPresetLayoutProperty).getDynamicObject())
    {
        if (layout->hasProperty ("outputChannels"))
        {
            const auto parsedChannels = static_cast<int> (layout->getProperty ("outputChannels"));
            if (parsedChannels <= 0)
                return false;
        }

        if (layout->hasProperty ("outputLayout")
            && layout->getProperty ("outputLayout").toString().trim().isEmpty())
        {
            return false;
        }
    }

    if (preset->hasProperty ("parameters")
        && ! applyEmitterParameterState (preset->getProperty ("parameters")))
    {
        return false;
    }

    if (preset->hasProperty ("timeline"))
        applyKeyframeTimelineLocked (preset->getProperty ("timeline"));

    {
        const auto choreographyPack = preset->hasProperty ("choreographyPackId")
            ? normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString())
            : juce::String ("custom");
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        choreographyPackState = choreographyPack;
    }

    return true;
}

juce::var LocusQAudioProcessor::listEmitterPresetsFromUI() const
{
    juce::Array<juce::var> presets;
    const auto presetDir = getPresetDirectory();
    if (! presetDir.exists())
        return juce::var (presets);

    juce::Array<juce::File> files;
    presetDir.findChildFiles (files, juce::File::findFiles, false, "*.json");

    for (const auto& file : files)
    {
        juce::var entryVar (new juce::DynamicObject());
        auto* entry = entryVar.getDynamicObject();

        juce::String displayName = file.getFileNameWithoutExtension();
        juce::String choreographyPackId = "custom";
        juce::String presetType = kEmitterPresetTypeEmitter;
        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* preset = payload->getDynamicObject())
            {
                if (preset->hasProperty ("name"))
                    displayName = preset->getProperty ("name").toString();

                if (preset->hasProperty ("choreographyPackId"))
                    choreographyPackId = normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString());

                presetType = inferPresetTypeFromPayload (*payload);
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        entry->setProperty ("choreographyPackId", choreographyPackId);
        entry->setProperty ("presetType", presetType);
        presets.add (entryVar);
    }

    return juce::var (presets);
}

juce::var LocusQAudioProcessor::saveEmitterPresetFromUI (const juce::var& options)
{
    juce::String requestedName = "Preset";
    juce::String presetType = kEmitterPresetTypeEmitter;
    juce::String choreographyPackId = "custom";
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("presetType"))
            presetType = optionsObject->getProperty ("presetType").toString();
        if (optionsObject->hasProperty ("choreographyPackId"))
            choreographyPackId = optionsObject->getProperty ("choreographyPackId").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = "Preset_" + juce::String (juce::Time::getCurrentTime().toMilliseconds());

    presetType = normalisePresetType (presetType);
    choreographyPackId = normaliseChoreographyPackId (choreographyPackId);
    const auto includeParameters = presetType == kEmitterPresetTypeEmitter;
    const auto includeTimeline = presetType == kEmitterPresetTypeMotion;

    const auto safeName = sanitisePresetName (requestedName);
    auto presetDir = getPresetDirectory();
    presetDir.createDirectory();
    const auto presetFile = presetDir.getChildFile (safeName + ".json");

    juce::var beforeState;
    juce::var afterState;
    juce::var presetPayload;
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        beforeState = captureAuthoringStateSnapshotLocked();
        presetPayload = buildEmitterPresetLocked (requestedName, presetType, choreographyPackId, includeParameters, includeTimeline);
        afterState = cloneJsonLikeVar (beforeState);
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();
    const auto beforeFileState = captureAuthoringFileState (presetFile);

    if (! writeJsonToFile (presetFile, presetPayload))
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Failed to write preset file.");
        return response;
    }

    response = commitAuthoringHistoryEntry ("preset_save",
                                            "Save Preset",
                                            beforeState,
                                            afterState,
                                            { beforeFileState },
                                            { captureAuthoringFileState (presetFile) },
                                            {},
                                            makeSelectionHint (presetFile.getFullPathName(), presetType));
    result = response.getDynamicObject();
    result->setProperty (locusq::shared_contracts::bridge_status::kOk, true);
    result->setProperty (locusq::shared_contracts::bridge_status::kName, requestedName);
    result->setProperty (locusq::shared_contracts::bridge_status::kFile, presetFile.getFileName());
    result->setProperty (locusq::shared_contracts::bridge_status::kPath, presetFile.getFullPathName());
    result->setProperty ("choreographyPackId", choreographyPackId);
    result->setProperty ("presetType", presetType);
    return response;
}

juce::var LocusQAudioProcessor::loadEmitterPresetFromUI (const juce::var& options)
{
    const auto presetFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (presetFile);
    if (! payload.has_value())
    {
        result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
        result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset file is invalid JSON.");
        return response;
    }

    juce::var beforeState;
    juce::var afterState;
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        beforeState = captureAuthoringStateSnapshotLocked();
        if (! applyEmitterPresetLocked (*payload))
        {
            result->setProperty (locusq::shared_contracts::bridge_status::kOk, false);
            result->setProperty (locusq::shared_contracts::bridge_status::kMessage, "Preset payload is not compatible.");
            return response;
        }
        afterState = captureAuthoringStateSnapshotLocked();
    }

    const auto inferredPresetType = inferPresetTypeFromPayload (*payload);
    response = commitAuthoringHistoryEntry ("preset_load",
                                            "Load Preset",
                                            beforeState,
                                            afterState,
                                            {},
                                            {},
                                            makeSelectionHint (presetFile.getFullPathName(), inferredPresetType),
                                            makeSelectionHint (presetFile.getFullPathName(), inferredPresetType));
    result = response.getDynamicObject();
    result->setProperty (locusq::shared_contracts::bridge_status::kOk, true);
    result->setProperty (locusq::shared_contracts::bridge_status::kName, presetFile.getFileNameWithoutExtension());
    result->setProperty (locusq::shared_contracts::bridge_status::kFile, presetFile.getFileName());
    result->setProperty (locusq::shared_contracts::bridge_status::kPath, presetFile.getFullPathName());
    result->setProperty ("presetType", inferredPresetType);
    if (auto* preset = payload->getDynamicObject(); preset != nullptr
        && preset->hasProperty ("choreographyPackId"))
    {
        result->setProperty ("choreographyPackId",
                             normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString()));
    }
    else
    {
        result->setProperty ("choreographyPackId", "custom");
    }
    return response;
}

juce::var LocusQAudioProcessor::renameEmitterPresetFromUI (const juce::var& options)
{
    const auto sourceFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! sourceFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
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
        result->setProperty ("message", "Preset name is required.");
        return response;
    }

    const auto safeName = sanitisePresetName (requestedName);
    const auto destinationFile = getPresetDirectory().getChildFile (safeName + ".json");
    const auto samePath = destinationFile.getFullPathName() == sourceFile.getFullPathName();

    if (! samePath && destinationFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset name already exists.");
        return response;
    }

    const auto payload = readJsonFromFile (sourceFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file is invalid JSON.");
        return response;
    }

    auto updatedPayload = *payload;
    if (auto* preset = updatedPayload.getDynamicObject(); preset != nullptr)
    {
        preset->setProperty ("name", requestedName);
        preset->setProperty ("updatedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));
    }

    juce::var beforeState;
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        beforeState = captureAuthoringStateSnapshotLocked();
    }
    const auto afterState = cloneJsonLikeVar (beforeState);
    const auto beforeSourceFileState = captureAuthoringFileState (sourceFile);
    const auto beforeDestinationFileState = captureAuthoringFileState (destinationFile);

    if (! writeJsonToFile (destinationFile, updatedPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write preset file.");
        return response;
    }

    if (! samePath)
        sourceFile.deleteFile();

    const auto renamedPresetType = inferPresetTypeFromPayload (updatedPayload);
    response = commitAuthoringHistoryEntry ("preset_rename",
                                            "Rename Preset",
                                            beforeState,
                                            afterState,
                                            { beforeSourceFileState, beforeDestinationFileState },
                                            { captureAuthoringFileState (sourceFile), captureAuthoringFileState (destinationFile) },
                                            makeSelectionHint (sourceFile.getFullPathName(), renamedPresetType),
                                            makeSelectionHint (destinationFile.getFullPathName(), renamedPresetType));
    result = response.getDynamicObject();
    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", destinationFile.getFileName());
    result->setProperty ("path", destinationFile.getFullPathName());
    result->setProperty ("presetType", renamedPresetType);
    if (auto* preset = updatedPayload.getDynamicObject(); preset != nullptr
        && preset->hasProperty ("choreographyPackId"))
    {
        result->setProperty ("choreographyPackId",
                             normaliseChoreographyPackId (preset->getProperty ("choreographyPackId").toString()));
    }
    else
    {
        result->setProperty ("choreographyPackId", "custom");
    }
    return response;
}

juce::var LocusQAudioProcessor::deleteEmitterPresetFromUI (const juce::var& options)
{
    const auto presetFile = resolvePresetFileFromOptions (options);

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (presetFile);

    juce::var beforeState;
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        beforeState = captureAuthoringStateSnapshotLocked();
    }
    const auto afterState = cloneJsonLikeVar (beforeState);
    const auto beforeFileState = captureAuthoringFileState (presetFile);

    if (! presetFile.deleteFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to delete preset file.");
        return response;
    }

    const auto deletedPresetType = payload.has_value()
        ? inferPresetTypeFromPayload (*payload)
        : juce::String (kEmitterPresetTypeEmitter);
    response = commitAuthoringHistoryEntry ("preset_delete",
                                            "Delete Preset",
                                            beforeState,
                                            afterState,
                                            { beforeFileState },
                                            { captureAuthoringFileState (presetFile) },
                                            makeSelectionHint (presetFile.getFullPathName(), deletedPresetType),
                                            {});
    result = response.getDynamicObject();
    result->setProperty ("ok", true);
    result->setProperty ("file", presetFile.getFileName());
    result->setProperty ("path", presetFile.getFullPathName());
    result->setProperty ("presetType", deletedPresetType);
    return response;
}

juce::var LocusQAudioProcessor::getUIStateFromUI() const
{
    juce::var stateVar (new juce::DynamicObject());
    auto* state = stateVar.getDynamicObject();

    juce::String emitterLabelSnapshot;
    juce::String physicsPresetSnapshot;
    juce::String choreographyPackSnapshot;
    {
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        emitterLabelSnapshot = emitterLabelState;
        physicsPresetSnapshot = physicsPresetState;
        choreographyPackSnapshot = choreographyPackState;
    }

    if (emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId))
    {
        const auto slotData = sceneGraph.getSlot (emitterSlotId).read();
        const auto slotLabel = juce::String::fromUTF8 (slotData.label).trim();
        if (slotLabel.isNotEmpty())
            emitterLabelSnapshot = slotLabel;
    }

    if (physicsPresetSnapshot.isEmpty())
        physicsPresetSnapshot = "off";
    if (choreographyPackSnapshot.isEmpty())
        choreographyPackSnapshot = "custom";

    state->setProperty ("emitterLabel", sanitiseEmitterLabel (emitterLabelSnapshot));
    state->setProperty ("physicsPreset", physicsPresetSnapshot);
    state->setProperty ("choreographyPack", normaliseChoreographyPackId (choreographyPackSnapshot));
    return stateVar;
}

bool LocusQAudioProcessor::setUIStateFromUI (const juce::var& stateVar)
{
    auto* state = stateVar.getDynamicObject();
    if (state == nullptr)
        return false;

    bool changed = false;

    if (state->hasProperty ("emitterLabel"))
    {
        const auto nextLabel = sanitiseEmitterLabel (state->getProperty ("emitterLabel").toString());
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            emitterLabelState = nextLabel;
        }
        emitterLabelRtState.store (std::make_shared<juce::String> (nextLabel));
        applyEmitterLabelToSceneSlotIfAvailable (nextLabel);
        changed = true;
    }

    if (state->hasProperty ("physicsPreset"))
    {
        auto preset = state->getProperty ("physicsPreset").toString().trim().toLowerCase();
        if (preset != "off" && preset != "bounce" && preset != "float" && preset != "orbit")
            preset = "custom";

        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            physicsPresetState = preset;
        }

        changed = true;
    }

    if (state->hasProperty ("choreographyPack"))
    {
        const auto choreographyPack = normaliseChoreographyPackId (state->getProperty ("choreographyPack").toString());
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            choreographyPackState = choreographyPack;
        }
        changed = true;
    }

    return changed;
}
