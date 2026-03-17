#include "../PluginProcessor.h"
#include "ProcessorConstants.h"

using namespace locusq::constants;

//==============================================================================
// Schema note: the state version is encoded as the string property
// kSnapshotSchemaValueV3 ("locusq-state-v3") as of BL-056.
//
// V2 ("locusq-state-v2") — prior schema; loaded cleanly by hasProperty guards.
// V3 ("locusq-state-v3") — BL-056: companion calibration profile handoff present.
//   Migration V2→V3 is transparent: no new mandatory state fields.
//   PEQ/FIR/SOFA data is re-polled from CalibrationProfile.json on startup.
//
// hp_calibration_enabled is persisted manually (not an APVTS parameter).

void LocusQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
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
                       kSnapshotSchemaValueV3,
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
            // Accept V2 ("locusq-state-v2") and V3 ("locusq-state-v3") states.
            // V2→V3 migration is transparent: hasProperty guards below handle
            // all optional fields; missing fields default cleanly.
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
                    const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
                    applyKeyframeTimelineLocked (timelineState);
                }
            }
            else
            {
                const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
                keyframeTimelineState.clearTracks();
                initialiseDefaultKeyframeTimeline (keyframeTimelineState);
                publishKeyframeTimelineStateToRtLocked();
            }

            if (state.hasProperty ("locusq_ui_state_json"))
            {
                const auto uiState = juce::JSON::parse (state.getProperty ("locusq_ui_state_json").toString());
                if (! uiState.isVoid())
                    setUIStateFromUI (uiState);
            }
        }
}
