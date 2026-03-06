#include "../PluginProcessor.h"
#include "ProcessorConstants.h"

using namespace locusq::constants;

//==============================================================================
// STATE SERIALIZATION
//
// Extracted from PluginProcessor.cpp (W0-A / BL-032 extension).
// Single responsibility: DAW preset save/restore (get/setStateInformation).
//==============================================================================

namespace
{
    // Resolve effective writable channel count for calibration routing,
    // guarding against transient "1 channel" reports during host startup.
    int resolveCalibrationWritableChannels (int snapshotOutputChannels,
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
} // anonymous namespace

//==============================================================================
// Schema note: the state version is encoded as the string property
// kSnapshotSchemaValueV2 ("locusq-state-v2").
//
// hp_calibration_enabled is persisted manually (not an APVTS parameter).
// See BL-038 for full calibration persistence roadmap.

void LocusQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        state.setProperty ("locusq_timeline_json",
                           juce::JSON::toString (serialiseKeyframeTimelineLocked(), true),
                           nullptr);
    }

    state.setProperty ("locusq_ui_state_json",
                       juce::JSON::toString (getUIStateFromUI(), true),
                       nullptr);

    // hp_calibration_enabled: not an APVTS param (no UI yet); persisted manually.
    state.setProperty ("hp_calibration_enabled",
                       spatialRenderer.isHeadphoneCalibrationEnabledRequested() ? 1 : 0,
                       nullptr);

    state.setProperty (kSnapshotSchemaProperty,
                       kSnapshotSchemaValueV2,
                       nullptr);
    state.setProperty (kSnapshotOutputLayoutProperty,
                       getSnapshotOutputLayout(),
                       nullptr);
    state.setProperty (kSnapshotOutputChannelsProperty,
                       getSnapshotOutputChannels(),
                       nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LocusQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

            const auto state = apvts.copyState();
            hasRestoredSnapshotState = state.hasProperty (kSnapshotSchemaProperty);

            // Restore manually-persisted headphone calibration enable flag (no APVTS backing).
            if (state.hasProperty ("hp_calibration_enabled"))
                spatialRenderer.setHeadphoneCalibrationEnabled (
                    static_cast<int> (state.getProperty ("hp_calibration_enabled")) > 0);
            hasSeededInitialEmitterColor = true;
            migrateSnapshotLayoutIfNeeded (state);
            const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
                getSnapshotOutputChannels(),
                static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
                lastAutoDetectedOutputChannels,
                getCurrentCalibrationSpeakerRouting());
            applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, false);

            if (state.hasProperty ("locusq_timeline_json"))
            {
                const auto timelineState = juce::JSON::parse (state.getProperty ("locusq_timeline_json").toString());
                if (! timelineState.isVoid())
                {
                    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
                    applyKeyframeTimelineLocked (timelineState);
                }
            }
            else
            {
                const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
                keyframeTimeline.clearTracks();
                initialiseDefaultKeyframeTimeline();
            }

            if (state.hasProperty ("locusq_ui_state_json"))
            {
                const auto uiState = juce::JSON::parse (state.getProperty ("locusq_ui_state_json").toString());
                if (! uiState.isVoid())
                    setUIStateFromUI (uiState);
            }
        }
}
