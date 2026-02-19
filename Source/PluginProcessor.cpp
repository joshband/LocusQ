#include "PluginProcessor.h"

#if ! defined (LOCUSQ_TESTING) || ! LOCUSQ_TESTING
#include "PluginEditor.h"
#endif

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
const char* toCalibrationStateString (CalibrationEngine::State state)
{
    switch (state)
    {
        case CalibrationEngine::State::Idle:      return "idle";
        case CalibrationEngine::State::Playing:   return "playing";
        case CalibrationEngine::State::Recording: return "recording";
        case CalibrationEngine::State::Analyzing: return "analyzing";
        case CalibrationEngine::State::Complete:  return "complete";
        case CalibrationEngine::State::Error:     return "error";
    }

    return "unknown";
}

TestSignalGenerator::Type toSignalType (int typeIndex)
{
    switch (juce::jlimit (0, 3, typeIndex))
    {
        case 0: return TestSignalGenerator::Type::LogSweep;
        case 1: return TestSignalGenerator::Type::PinkNoise;
        case 2: return TestSignalGenerator::Type::WhiteNoise;
        case 3: return TestSignalGenerator::Type::Impulse;
        default: break;
    }

    return TestSignalGenerator::Type::LogSweep;
}

int toSignalTypeIndex (juce::String type)
{
    type = type.trim().toLowerCase();

    if (type == "sweep" || type == "logsweep" || type == "log_sweep")
        return 0;
    if (type == "pink" || type == "pinknoise" || type == "pink_noise")
        return 1;
    if (type == "white" || type == "whitenoise" || type == "white_noise")
        return 2;
    if (type == "impulse")
        return 3;

    return 0;
}

const juce::String kTrackPosAzimuth { "pos_azimuth" };
const juce::String kTrackPosElevation { "pos_elevation" };
const juce::String kTrackPosDistance { "pos_distance" };
const juce::String kTrackPosX { "pos_x" };
const juce::String kTrackPosY { "pos_y" };
const juce::String kTrackPosZ { "pos_z" };
const juce::String kTrackSizeUniform { "size_uniform" };

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

    if (outputSet.size() >= SpatialRenderer::NUM_SPEAKERS)
        return "multichannel";

    return "other";
}

constexpr const char* kSnapshotSchemaProperty = "locusq_snapshot_schema";
constexpr const char* kSnapshotSchemaValueV2 = "locusq-state-v2";
constexpr const char* kSnapshotOutputLayoutProperty = "locusq_output_layout";
constexpr const char* kSnapshotOutputChannelsProperty = "locusq_output_channels";
constexpr const char* kEmitterPresetSchemaV1 = "locusq-emitter-preset-v1";
constexpr const char* kEmitterPresetSchemaV2 = "locusq-emitter-preset-v2";
constexpr const char* kEmitterPresetLayoutProperty = "layout";

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
}

//==============================================================================
LocusQAudioProcessor::LocusQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      sceneGraph (SceneGraph::getInstance())
{
    initialiseDefaultKeyframeTimeline();

    // Register with scene graph based on initial mode
    // Mode registration happens in prepareToPlay once we know the context
}

LocusQAudioProcessor::~LocusQAudioProcessor()
{
    // Unregister from scene graph
    if (emitterSlotId >= 0)
        sceneGraph.unregisterEmitter (emitterSlotId);

    if (rendererRegistered)
        sceneGraph.unregisterRenderer();
}

//==============================================================================
void LocusQAudioProcessor::syncSceneGraphRegistrationForMode (LocusQMode mode)
{
    if (mode != LocusQMode::Emitter && emitterSlotId >= 0)
    {
        sceneGraph.unregisterEmitter (emitterSlotId);
        emitterSlotId = -1;
        lastPhysThrowGate = false;
        lastPhysResetGate = false;
    }

    if (mode != LocusQMode::Renderer && rendererRegistered)
    {
        sceneGraph.unregisterRenderer();
        rendererRegistered = false;
    }

    if (mode == LocusQMode::Emitter && emitterSlotId < 0)
    {
        emitterSlotId = sceneGraph.registerEmitter();
        DBG ("LocusQ: Registered emitter, slot " + juce::String (emitterSlotId));

        juce::String restoredLabel;
        {
            const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
            restoredLabel = emitterLabelState;
        }
        applyEmitterLabelToSceneSlotIfAvailable (restoredLabel);
    }
    else if (mode == LocusQMode::Renderer && ! rendererRegistered)
    {
        rendererRegistered = sceneGraph.registerRenderer();
        DBG ("LocusQ: Registered renderer: " + juce::String (rendererRegistered ? "OK" : "FAILED (already exists)"));
    }
}

//==============================================================================
void LocusQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        keyframeTimeline.prepare (sampleRate);
        initialiseDefaultKeyframeTimeline();
    }

    // Prepare physics engine (Phase 2.4)
    physicsEngine.prepare (sampleRate);

    // Prepare spatial renderer (Phase 2.2)
    spatialRenderer.prepare (sampleRate, samplesPerBlock);

    // Prepare calibration engine (Phase 2.3)
    calibrationEngine.prepare (sampleRate, samplesPerBlock);

    syncSceneGraphRegistrationForMode (getCurrentMode());
}

void LocusQAudioProcessor::releaseResources()
{
    physicsEngine.shutdown();
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        keyframeTimeline.reset();
    }
}

bool LocusQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainInput  = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    const bool supportedInput =
        (mainInput == juce::AudioChannelSet::mono())
        || (mainInput == juce::AudioChannelSet::stereo());

    if (! supportedInput)
        return false;

    const bool supportedOutput =
        (mainOutput == juce::AudioChannelSet::mono())
        || (mainOutput == juce::AudioChannelSet::stereo())
        || (mainOutput == juce::AudioChannelSet::quadraphonic())
        || (mainOutput == juce::AudioChannelSet::discreteChannels (4));

    return supportedOutput;
}

//==============================================================================
void LocusQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    const auto ticksPerSecond = static_cast<double> (juce::Time::getHighResolutionTicksPerSecond());
    const auto blockStartTicks = juce::Time::getHighResolutionTicks();

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Check bypass
    auto* bypassParam = apvts.getRawParameterValue ("bypass");
    if (bypassParam->load() > 0.5f)
        return;

    auto mode = getCurrentMode();
    syncSceneGraphRegistrationForMode (mode);

    switch (mode)
    {
        case LocusQMode::Calibrate:
        {
            // Read mic input channel from parameter (1-indexed → 0-indexed)
            int micCh = static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()) - 1;
            micCh = juce::jlimit (0, buffer.getNumChannels() - 1, micCh);

            // CalibrationEngine manages signal generation, recording, and analysis.
            // processBlock() is RT safe: no allocation, atomic state reads only.
            calibrationEngine.processBlock (buffer, micCh);
            break;
        }

        case LocusQMode::Emitter:
        {
            if (emitterSlotId >= 0)
            {
                // Publish audio buffer pointer for renderer to consume
                sceneGraph.getSlot (emitterSlotId).setAudioBuffer (
                    buffer.getArrayOfReadPointers(),
                    buffer.getNumChannels(),
                    buffer.getNumSamples());

                // Publish spatial state
                const auto emitterStartTicks = juce::Time::getHighResolutionTicks();
                publishEmitterState (buffer.getNumSamples());
                const auto emitterElapsedTicks = juce::Time::getHighResolutionTicks() - emitterStartTicks;
                const auto emitterMs = (static_cast<double> (emitterElapsedTicks) * 1000.0) / ticksPerSecond;
                updatePerfEma (perfEmitterPublishMs, emitterMs);
            }

            // Audio passes through unchanged in Emitter mode
            break;
        }

        case LocusQMode::Renderer:
        {
            // Publish global physics controls for emitters
            sceneGraph.setPhysicsRateIndex (
                static_cast<int> (apvts.getRawParameterValue ("rend_phys_rate")->load()));
            sceneGraph.setPhysicsPaused (
                apvts.getRawParameterValue ("rend_phys_pause")->load() > 0.5f);
            sceneGraph.setPhysicsWallCollisionEnabled (
                apvts.getRawParameterValue ("rend_phys_walls")->load() > 0.5f);

            // Update renderer DSP parameters from APVTS
            updateRendererParameters();

            // Clear output buffer (renderer generates its own audio from emitters)
            buffer.clear();

            // Spatialize all emitters into output
            const auto rendererStartTicks = juce::Time::getHighResolutionTicks();
            spatialRenderer.process (buffer, sceneGraph);
            const auto rendererElapsedTicks = juce::Time::getHighResolutionTicks() - rendererStartTicks;
            const auto rendererMs = (static_cast<double> (rendererElapsedTicks) * 1000.0) / ticksPerSecond;
            updatePerfEma (perfRendererProcessMs, rendererMs);
            break;
        }
    }

    sceneGraph.advanceSampleCounter (buffer.getNumSamples());

    const auto blockElapsedTicks = juce::Time::getHighResolutionTicks() - blockStartTicks;
    const auto blockMs = (static_cast<double> (blockElapsedTicks) * 1000.0) / ticksPerSecond;
    updatePerfEma (perfProcessBlockMs, blockMs);
}

//==============================================================================
void LocusQAudioProcessor::updateRendererParameters()
{
    // Quality tier (Draft/Final)
    spatialRenderer.setQualityTier (
        static_cast<int> (apvts.getRawParameterValue ("rend_quality")->load()));

    // Distance model
    spatialRenderer.setDistanceModel (
        static_cast<int> (apvts.getRawParameterValue ("rend_distance_model")->load()));
    spatialRenderer.setReferenceDistance (
        apvts.getRawParameterValue ("rend_distance_ref")->load());
    spatialRenderer.setMaxDistance (
        apvts.getRawParameterValue ("rend_distance_max")->load());

    // Air absorption
    spatialRenderer.setAirAbsorptionEnabled (
        apvts.getRawParameterValue ("rend_air_absorb")->load() > 0.5f);

    // Doppler
    spatialRenderer.setDopplerEnabled (
        apvts.getRawParameterValue ("rend_doppler")->load() > 0.5f);
    spatialRenderer.setDopplerScale (
        apvts.getRawParameterValue ("rend_doppler_scale")->load());

    // Room acoustics
    spatialRenderer.setRoomEnabled (
        apvts.getRawParameterValue ("rend_room_enable")->load() > 0.5f);
    spatialRenderer.setRoomMix (
        apvts.getRawParameterValue ("rend_room_mix")->load());
    spatialRenderer.setRoomSize (
        apvts.getRawParameterValue ("rend_room_size")->load());
    spatialRenderer.setRoomDamping (
        apvts.getRawParameterValue ("rend_room_damping")->load());
    spatialRenderer.setEarlyReflectionsOnly (
        apvts.getRawParameterValue ("rend_room_er_only")->load() > 0.5f);

    // Master gain
    spatialRenderer.setMasterGain (
        apvts.getRawParameterValue ("rend_master_gain")->load());

    // Per-speaker trims
    spatialRenderer.setSpeakerTrim (0, apvts.getRawParameterValue ("rend_spk1_gain")->load());
    spatialRenderer.setSpeakerTrim (1, apvts.getRawParameterValue ("rend_spk2_gain")->load());
    spatialRenderer.setSpeakerTrim (2, apvts.getRawParameterValue ("rend_spk3_gain")->load());
    spatialRenderer.setSpeakerTrim (3, apvts.getRawParameterValue ("rend_spk4_gain")->load());

    spatialRenderer.setSpeakerDelay (0, apvts.getRawParameterValue ("rend_spk1_delay")->load());
    spatialRenderer.setSpeakerDelay (1, apvts.getRawParameterValue ("rend_spk2_delay")->load());
    spatialRenderer.setSpeakerDelay (2, apvts.getRawParameterValue ("rend_spk3_delay")->load());
    spatialRenderer.setSpeakerDelay (3, apvts.getRawParameterValue ("rend_spk4_delay")->load());
}

//==============================================================================
void LocusQAudioProcessor::initialiseDefaultKeyframeTimeline()
{
    if (keyframeTimeline.hasAnyTrack())
        return;

    KeyframeTrack azimuthTrack { kTrackPosAzimuth };
    azimuthTrack.setKeyframes ({
        { 0.0, -60.0f, KeyframeCurve::easeInOut },
        { 2.0, 20.0f,  KeyframeCurve::easeInOut },
        { 4.0, 95.0f,  KeyframeCurve::easeInOut },
        { 6.0, 10.0f,  KeyframeCurve::easeInOut },
        { 8.0, -60.0f, KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (azimuthTrack));

    KeyframeTrack elevationTrack { kTrackPosElevation };
    elevationTrack.setKeyframes ({
        { 0.0,  0.0f,  KeyframeCurve::easeInOut },
        { 2.0,  18.0f, KeyframeCurve::easeInOut },
        { 4.0,  2.0f,  KeyframeCurve::easeInOut },
        { 6.0, -14.0f, KeyframeCurve::easeInOut },
        { 8.0,  0.0f,  KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (elevationTrack));

    KeyframeTrack distanceTrack { kTrackPosDistance };
    distanceTrack.setKeyframes ({
        { 0.0, 2.1f, KeyframeCurve::easeInOut },
        { 2.0, 3.6f, KeyframeCurve::easeInOut },
        { 4.0, 2.4f, KeyframeCurve::easeInOut },
        { 6.0, 1.3f, KeyframeCurve::easeInOut },
        { 8.0, 2.1f, KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (distanceTrack));

    KeyframeTrack sizeTrack { kTrackSizeUniform };
    sizeTrack.setKeyframes ({
        { 0.0, 0.45f, KeyframeCurve::easeInOut },
        { 2.0, 0.62f, KeyframeCurve::easeInOut },
        { 4.0, 0.35f, KeyframeCurve::easeInOut },
        { 6.0, 0.74f, KeyframeCurve::easeInOut },
        { 8.0, 0.45f, KeyframeCurve::easeInOut }
    });
    keyframeTimeline.addOrReplaceTrack (std::move (sizeTrack));

    keyframeTimeline.setDurationSeconds (8.0);
    keyframeTimeline.setLooping (true);
    keyframeTimeline.setPlaybackRate (1.0f);
}

std::optional<double> LocusQAudioProcessor::getTransportTimeSeconds() const
{
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto timeSeconds = position->getTimeInSeconds())
                return *timeSeconds;

            if (const auto samplePosition = position->getTimeInSamples())
                return static_cast<double> (*samplePosition) / juce::jmax (1.0, currentSampleRate);

            if (const auto ppq = position->getPpqPosition())
            {
                if (const auto bpm = position->getBpm(); bpm && *bpm > 1.0e-6)
                    return (*ppq * 60.0) / *bpm;
            }
        }
    }

    return std::nullopt;
}

//==============================================================================
void LocusQAudioProcessor::publishEmitterState (int numSamplesInBlock)
{
    if (emitterSlotId < 0)
        return;

    const auto existingData = sceneGraph.getSlot (emitterSlotId).read();

    EmitterData data;
    data.active = true;
    std::memcpy (data.label, existingData.label, sizeof (data.label));
    data.label[sizeof (data.label) - 1] = '\0';

    const auto coordMode = apvts.getRawParameterValue ("pos_coord_mode")->load();
    float azimuthDeg = apvts.getRawParameterValue ("pos_azimuth")->load();
    float elevationDeg = apvts.getRawParameterValue ("pos_elevation")->load();
    float distance = apvts.getRawParameterValue ("pos_distance")->load();
    float posX = apvts.getRawParameterValue ("pos_x")->load();
    float posY = apvts.getRawParameterValue ("pos_y")->load();
    float posZ = apvts.getRawParameterValue ("pos_z")->load();
    float sizeUniform = apvts.getRawParameterValue ("size_uniform")->load();

    const bool animationEnabled = apvts.getRawParameterValue ("anim_enable")->load() > 0.5f;
    const bool internalAnimation = animationEnabled
                               && static_cast<int> (apvts.getRawParameterValue ("anim_mode")->load()) == 1;

    if (internalAnimation)
    {
        const juce::SpinLock::ScopedTryLockType timelineLock (keyframeTimelineLock);
        if (timelineLock.isLocked())
        {
            keyframeTimeline.setLooping (apvts.getRawParameterValue ("anim_loop")->load() > 0.5f);
            keyframeTimeline.setPlaybackRate (apvts.getRawParameterValue ("anim_speed")->load());

            bool advancedFromTransport = false;
            if (apvts.getRawParameterValue ("anim_sync")->load() > 0.5f)
            {
                if (const auto transportTimeSeconds = getTransportTimeSeconds())
                {
                    const auto playbackSeconds = (*transportTimeSeconds) * static_cast<double> (keyframeTimeline.getPlaybackRate());
                    keyframeTimeline.setCurrentTimeSeconds (playbackSeconds);
                    advancedFromTransport = true;
                }
            }

            if (! advancedFromTransport)
            {
                const auto blockDurationSeconds = (currentSampleRate > 0.0)
                                                ? static_cast<double> (numSamplesInBlock) / currentSampleRate
                                                : 0.0;
                keyframeTimeline.advance (blockDurationSeconds);
            }

            if (coordMode < 0.5f)
            {
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosAzimuth))
                    azimuthDeg = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosElevation))
                    elevationDeg = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosDistance))
                    distance = *value;
            }
            else
            {
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosX))
                    posX = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosY))
                    posY = *value;
                if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackPosZ))
                    posZ = *value;
            }

            if (const auto value = keyframeTimeline.evaluateTrackAtCurrentTime (kTrackSizeUniform))
                sizeUniform = *value;
        }
    }

    Vec3 basePosition;

    if (coordMode < 0.5f) // Spherical
    {
        const float azimuthRad = azimuthDeg * juce::MathConstants<float>::pi / 180.0f;
        const float elevationRad = elevationDeg * juce::MathConstants<float>::pi / 180.0f;

        basePosition.x = distance * std::cos (elevationRad) * std::sin (azimuthRad);
        basePosition.z = distance * std::cos (elevationRad) * std::cos (azimuthRad);
        basePosition.y = distance * std::sin (elevationRad);
    }
    else // Cartesian
    {
        basePosition.x = posX;
        basePosition.y = posZ; // Z in param = Y in 3D (height)
        basePosition.z = posY;
    }

    data.position = basePosition;

    const bool linkedSize = apvts.getRawParameterValue ("size_link")->load() > 0.5f;
    if (linkedSize)
    {
        const float clampedSize = juce::jlimit (0.01f, 20.0f, sizeUniform);
        data.size = { clampedSize, clampedSize, clampedSize };
    }
    else
    {
        data.size.x = apvts.getRawParameterValue ("size_width")->load();
        data.size.y = apvts.getRawParameterValue ("size_height")->load();
        data.size.z = apvts.getRawParameterValue ("size_depth")->load();
    }

    data.gain        = apvts.getRawParameterValue ("emit_gain")->load();
    data.spread      = apvts.getRawParameterValue ("emit_spread")->load();
    data.directivity = apvts.getRawParameterValue ("emit_directivity")->load();
    data.muted       = apvts.getRawParameterValue ("emit_mute")->load() > 0.5f;
    data.soloed      = apvts.getRawParameterValue ("emit_solo")->load() > 0.5f;

    const float aimAzimuth = apvts.getRawParameterValue ("emit_dir_azimuth")->load();
    const float aimElevation = apvts.getRawParameterValue ("emit_dir_elevation")->load();
    const float aimAzimuthRad = aimAzimuth * juce::MathConstants<float>::pi / 180.0f;
    const float aimElevationRad = aimElevation * juce::MathConstants<float>::pi / 180.0f;
    data.directivityAim.x = std::cos (aimElevationRad) * std::sin (aimAzimuthRad);
    data.directivityAim.z = std::cos (aimElevationRad) * std::cos (aimAzimuthRad);
    data.directivityAim.y = std::sin (aimElevationRad);

    const bool physicsEnabled = apvts.getRawParameterValue ("phys_enable")->load() > 0.5f;
    data.physicsEnabled = physicsEnabled;

    physicsEngine.setUpdateRateIndex (sceneGraph.getPhysicsRateIndex());
    physicsEngine.setPaused (sceneGraph.isPhysicsPaused());
    physicsEngine.setWallCollisionEnabled (sceneGraph.isPhysicsWallCollisionEnabled());

    if (auto profile = sceneGraph.getRoomProfile(); profile != nullptr && profile->valid)
        physicsEngine.setRoomDimensions (profile->dimensions);

    physicsEngine.setRestPosition (basePosition);
    physicsEngine.setPhysicsEnabled (physicsEnabled);
    physicsEngine.setMass (apvts.getRawParameterValue ("phys_mass")->load());
    physicsEngine.setDrag (apvts.getRawParameterValue ("phys_drag")->load());
    physicsEngine.setElasticity (apvts.getRawParameterValue ("phys_elasticity")->load());
    physicsEngine.setFriction (apvts.getRawParameterValue ("phys_friction")->load());
    physicsEngine.setGravity (
        apvts.getRawParameterValue ("phys_gravity")->load(),
        static_cast<int> (apvts.getRawParameterValue ("phys_gravity_dir")->load()));

    const bool throwGate = apvts.getRawParameterValue ("phys_throw")->load() > 0.5f;
    if (throwGate && ! lastPhysThrowGate)
    {
        const Vec3 throwVelocity
        {
            apvts.getRawParameterValue ("phys_vel_x")->load(),
            apvts.getRawParameterValue ("phys_vel_z")->load(), // Z in param = Y in 3D (height)
            apvts.getRawParameterValue ("phys_vel_y")->load()
        };
        physicsEngine.requestThrow (throwVelocity);
    }
    lastPhysThrowGate = throwGate;

    const bool resetGate = apvts.getRawParameterValue ("phys_reset")->load() > 0.5f;
    if (resetGate && ! lastPhysResetGate)
        physicsEngine.requestReset();
    lastPhysResetGate = resetGate;

    if (physicsEnabled)
    {
        const auto physicsState = physicsEngine.getState();
        if (physicsState.initialized)
        {
            data.position = physicsState.position;
            data.velocity = physicsState.velocity;
        }
    }
    else
    {
        data.velocity = {};
    }

    data.colorIndex = static_cast<uint8_t> (
        static_cast<int> (apvts.getRawParameterValue ("emit_color")->load()) % 16);

    sceneGraph.getSlot (emitterSlotId).write (data);
}

//==============================================================================
LocusQMode LocusQAudioProcessor::getCurrentMode() const
{
    auto* modeParam = apvts.getRawParameterValue ("mode");
    int modeVal = static_cast<int> (modeParam->load());
    return static_cast<LocusQMode> (juce::jlimit (0, 2, modeVal));
}

void LocusQAudioProcessor::primeRendererStateFromCurrentParameters()
{
    if (getCurrentMode() == LocusQMode::Renderer)
        updateRendererParameters();
}

//==============================================================================
juce::String LocusQAudioProcessor::getSceneStateJSON() const
{
    // Build JSON scene snapshot for WebView
    juce::String json = "{\"emitters\":[";
    bool first = true;
    double timelineTime = 0.0;
    double timelineDuration = 0.0;
    bool timelineLooping = false;
    const auto outputSet = getBusesLayout().getMainOutputChannelSet();
    const auto outputChannels = getMainBusNumOutputChannels();
    const auto outputLayout = outputLayoutToString (outputSet);
    const juce::String internalSpeakerLabelsJson { "[\"FL\",\"FR\",\"RR\",\"RL\"]" };
    const juce::String quadOutputMapJson { "[0,1,3,2]" };
    juce::String outputChannelLabelsJson { "[\"M\"]" };
    juce::String rendererOutputMode { "mono_sum" };

    if (outputChannels >= SpatialRenderer::NUM_SPEAKERS)
    {
        outputChannelLabelsJson = "[\"FL\",\"FR\",\"RL\",\"RR\"]";
        rendererOutputMode = "quad_map_first4";
    }
    else if (outputChannels >= 2)
    {
        outputChannelLabelsJson = "[\"L\",\"R\"]";
        rendererOutputMode = "stereo_downmix";
    }

    {
        const juce::SpinLock::ScopedTryLockType timelineLock (keyframeTimelineLock);
        if (timelineLock.isLocked())
        {
            timelineTime = keyframeTimeline.getCurrentTimeSeconds();
            timelineDuration = keyframeTimeline.getDurationSeconds();
            timelineLooping = keyframeTimeline.isLooping();
        }
    }

    for (int i = 0; i < SceneGraph::MAX_EMITTERS; ++i)
    {
        if (! sceneGraph.isSlotActive (i)) continue;
        auto data = sceneGraph.getSlot (i).read();
        if (! data.active) continue;

        if (! first) json += ",";
        first = false;

        json += "{\"id\":" + juce::String (i)
              + ",\"x\":" + juce::String (data.position.x, 3)
              + ",\"y\":" + juce::String (data.position.y, 3)
              + ",\"z\":" + juce::String (data.position.z, 3)
              + ",\"sx\":" + juce::String (data.size.x, 2)
              + ",\"sy\":" + juce::String (data.size.y, 2)
              + ",\"sz\":" + juce::String (data.size.z, 2)
              + ",\"gain\":" + juce::String (data.gain, 1)
              + ",\"spread\":" + juce::String (data.spread, 2)
              + ",\"color\":" + juce::String (data.colorIndex)
              + ",\"muted\":" + juce::String (data.muted ? "true" : "false")
              + ",\"soloed\":" + juce::String (data.soloed ? "true" : "false")
              + ",\"physics\":" + juce::String (data.physicsEnabled ? "true" : "false")
              + ",\"vx\":" + juce::String (data.velocity.x, 3)
              + ",\"vy\":" + juce::String (data.velocity.y, 3)
              + ",\"vz\":" + juce::String (data.velocity.z, 3)
              + ",\"label\":\"" + juce::String (data.label) + "\""
              + "}";
    }

    json += "],\"emitterCount\":" + juce::String (sceneGraph.getActiveEmitterCount())
          + ",\"localEmitterId\":" + juce::String (emitterSlotId)
          + ",\"rendererActive\":" + juce::String (sceneGraph.isRendererRegistered() ? "true" : "false")
          + ",\"rendererEligibleEmitters\":" + juce::String (spatialRenderer.getLastEligibleEmitterCount())
          + ",\"rendererProcessedEmitters\":" + juce::String (spatialRenderer.getLastProcessedEmitterCount())
          + ",\"rendererCulledBudget\":" + juce::String (spatialRenderer.getLastBudgetCulledEmitterCount())
          + ",\"rendererCulledActivity\":" + juce::String (spatialRenderer.getLastActivityCulledEmitterCount())
          + ",\"rendererGuardrailActive\":" + juce::String (spatialRenderer.wasGuardrailActiveLastBlock() ? "true" : "false")
          + ",\"outputChannels\":" + juce::String (outputChannels)
          + ",\"outputLayout\":\"" + outputLayout + "\""
          + ",\"rendererOutputMode\":\"" + rendererOutputMode + "\""
          + ",\"rendererOutputChannels\":" + outputChannelLabelsJson
          + ",\"rendererInternalSpeakers\":" + internalSpeakerLabelsJson
          + ",\"rendererQuadMap\":" + quadOutputMapJson
          + ",\"animEnabled\":" + juce::String (apvts.getRawParameterValue ("anim_enable")->load() > 0.5f ? "true" : "false")
          + ",\"animMode\":" + juce::String (static_cast<int> (apvts.getRawParameterValue ("anim_mode")->load()))
          + ",\"animTime\":" + juce::String (timelineTime, 3)
          + ",\"animDuration\":" + juce::String (timelineDuration, 3)
          + ",\"animLooping\":" + juce::String (timelineLooping ? "true" : "false")
          + ",\"perfBlockMs\":" + juce::String (perfProcessBlockMs, 4)
          + ",\"perfEmitterMs\":" + juce::String (perfEmitterPublishMs, 4)
          + ",\"perfRendererMs\":" + juce::String (perfRendererProcessMs, 4)
          + "}";

    return json;
}

//==============================================================================
bool LocusQAudioProcessor::startCalibrationFromUI (const juce::var& options)
{
    if (getCurrentMode() != LocusQMode::Calibrate)
        return false;

    if (calibrationEngine.getState() != CalibrationEngine::State::Idle)
        return false;

    int testTypeIndex = static_cast<int> (apvts.getRawParameterValue ("cal_test_type")->load());
    float levelDb     = apvts.getRawParameterValue ("cal_test_level")->load();
    float sweepSecs   = 3.0f;
    float tailSecs    = 1.5f;
    int micChannel    = static_cast<int> (apvts.getRawParameterValue ("cal_mic_channel")->load()) - 1;
    int speakerCh[4] =
    {
        static_cast<int> (apvts.getRawParameterValue ("cal_spk1_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk2_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk3_out")->load()) - 1,
        static_cast<int> (apvts.getRawParameterValue ("cal_spk4_out")->load()) - 1
    };

    if (auto* obj = options.getDynamicObject())
    {
        if (obj->hasProperty ("testType"))
        {
            const auto& value = obj->getProperty ("testType");
            if (value.isString())
                testTypeIndex = toSignalTypeIndex (value.toString());
            else
                testTypeIndex = static_cast<int> (value);
        }

        if (obj->hasProperty ("testLevelDb"))
            levelDb = static_cast<float> (double (obj->getProperty ("testLevelDb")));

        if (obj->hasProperty ("sweepSeconds"))
            sweepSecs = static_cast<float> (double (obj->getProperty ("sweepSeconds")));

        if (obj->hasProperty ("tailSeconds"))
            tailSecs = static_cast<float> (double (obj->getProperty ("tailSeconds")));

        if (obj->hasProperty ("micChannel"))
            micChannel = static_cast<int> (obj->getProperty ("micChannel"));

        if (obj->hasProperty ("speakerChannels"))
        {
            const auto channels = obj->getProperty ("speakerChannels");
            if (auto* arr = channels.getArray())
            {
                const auto count = juce::jmin (4, arr->size());
                for (int i = 0; i < count; ++i)
                    speakerCh[i] = static_cast<int> (arr->getReference (i));
            }
        }
    }

    micChannel = juce::jlimit (0, 7, micChannel);
    sweepSecs  = juce::jlimit (0.1f, 30.0f, sweepSecs);
    tailSecs   = juce::jlimit (0.0f, 10.0f, tailSecs);

    for (int& ch : speakerCh)
        ch = juce::jlimit (0, 7, ch);

    if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("cal_mic_channel")))
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (micChannel + 1)));

    calibrationEngine.startCalibration (toSignalType (testTypeIndex),
                                        levelDb,
                                        sweepSecs,
                                        tailSecs,
                                        speakerCh,
                                        micChannel);

    return calibrationEngine.getState() == CalibrationEngine::State::Playing;
}

void LocusQAudioProcessor::abortCalibrationFromUI()
{
    calibrationEngine.abortCalibration();
}

juce::var LocusQAudioProcessor::getCalibrationStatus() const
{
    auto progress = calibrationEngine.getProgress();
    const auto state = progress.state;
    const auto speakerIndex = juce::jlimit (0, 3, progress.currentSpeaker);

    int completedSpeakers = 0;
    float speakerPhasePercent = 0.0f;
    bool running = false;

    switch (state)
    {
        case CalibrationEngine::State::Idle:
            break;

        case CalibrationEngine::State::Playing:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = juce::jlimit (0.0f, 1.0f, progress.playPercent) * 0.5f;
            break;

        case CalibrationEngine::State::Recording:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = 0.5f + juce::jlimit (0.0f, 1.0f, progress.recordPercent) * 0.45f;
            break;

        case CalibrationEngine::State::Analyzing:
            running = true;
            completedSpeakers = speakerIndex;
            speakerPhasePercent = 0.95f;
            break;

        case CalibrationEngine::State::Complete:
            completedSpeakers = 4;
            speakerPhasePercent = 1.0f;
            break;

        case CalibrationEngine::State::Error:
            completedSpeakers = speakerIndex;
            break;
    }

    auto overallPercent = (state == CalibrationEngine::State::Complete)
                            ? 1.0f
                            : (static_cast<float> (completedSpeakers) + speakerPhasePercent) / 4.0f;
    overallPercent = juce::jlimit (0.0f, 1.0f, overallPercent);

    juce::var statusVar (new juce::DynamicObject());
    auto* status = statusVar.getDynamicObject();

    status->setProperty ("state", toCalibrationStateString (state));
    status->setProperty ("stateCode", static_cast<int> (state));
    status->setProperty ("running", running);
    status->setProperty ("complete", state == CalibrationEngine::State::Complete);
    status->setProperty ("currentSpeaker", speakerIndex + 1);
    status->setProperty ("completedSpeakers", completedSpeakers);
    status->setProperty ("playPercent", juce::jlimit (0.0f, 1.0f, progress.playPercent));
    status->setProperty ("recordPercent", juce::jlimit (0.0f, 1.0f, progress.recordPercent));
    status->setProperty ("overallPercent", overallPercent);
    status->setProperty ("message", progress.message);

    juce::Array<juce::var> speakerLevels;
    speakerLevels.ensureStorageAllocated (4);
    for (int i = 0; i < 4; ++i)
    {
        float level = 0.0f;

        if (state == CalibrationEngine::State::Complete || i < completedSpeakers)
        {
            level = 1.0f;
        }
        else if (running && i == speakerIndex)
        {
            if (state == CalibrationEngine::State::Playing)
                level = juce::jlimit (0.0f, 1.0f, progress.playPercent);
            else if (state == CalibrationEngine::State::Recording)
                level = juce::jlimit (0.0f, 1.0f, progress.recordPercent);
            else if (state == CalibrationEngine::State::Analyzing)
                level = 1.0f;
        }

        speakerLevels.add (juce::jlimit (0.0f, 1.0f, level));
    }
    status->setProperty ("speakerLevels", juce::var (speakerLevels));

    const auto roomProfile = sceneGraph.getRoomProfile();
    status->setProperty ("profileValid", roomProfile != nullptr && roomProfile->valid);

    if (state == CalibrationEngine::State::Complete)
        status->setProperty ("estimatedRT60", calibrationEngine.getResult().estimatedRT60);

    return statusVar;
}

juce::var LocusQAudioProcessor::serialiseKeyframeTimelineLocked() const
{
    juce::var timelineVar (new juce::DynamicObject());
    auto* timeline = timelineVar.getDynamicObject();

    timeline->setProperty ("durationSeconds", keyframeTimeline.getDurationSeconds());
    timeline->setProperty ("looping", keyframeTimeline.isLooping());
    timeline->setProperty ("playbackRate", keyframeTimeline.getPlaybackRate());
    timeline->setProperty ("currentTimeSeconds", keyframeTimeline.getCurrentTimeSeconds());

    juce::Array<juce::var> tracks;

    for (const auto& track : keyframeTimeline.getTracks())
    {
        juce::var trackVar (new juce::DynamicObject());
        auto* trackObject = trackVar.getDynamicObject();
        trackObject->setProperty ("parameterId", track.getParameterId());

        juce::Array<juce::var> keyframes;
        for (const auto& keyframe : track.getKeyframes())
        {
            juce::var keyframeVar (new juce::DynamicObject());
            auto* keyframeObject = keyframeVar.getDynamicObject();
            keyframeObject->setProperty ("timeSeconds", keyframe.timeSeconds);
            keyframeObject->setProperty ("value", keyframe.value);
            keyframeObject->setProperty ("curve", keyframeCurveToString (keyframe.curve));
            keyframes.add (keyframeVar);
        }

        trackObject->setProperty ("keyframes", juce::var (keyframes));
        tracks.add (trackVar);
    }

    timeline->setProperty ("tracks", juce::var (tracks));
    return timelineVar;
}

bool LocusQAudioProcessor::applyKeyframeTimelineLocked (const juce::var& timelineState)
{
    auto* timeline = timelineState.getDynamicObject();
    if (timeline == nullptr)
        return false;

    auto* trackArray = timeline->getProperty ("tracks").getArray();
    if (trackArray == nullptr)
        return false;

    keyframeTimeline.clearTracks();

    for (const auto& trackValue : *trackArray)
    {
        auto* trackObject = trackValue.getDynamicObject();
        if (trackObject == nullptr)
            continue;

        const auto parameterId = trackObject->getProperty ("parameterId").toString().trim();
        if (parameterId.isEmpty())
            continue;

        std::vector<Keyframe> keyframes;
        if (auto* keyframeArray = trackObject->getProperty ("keyframes").getArray())
        {
            keyframes.reserve (static_cast<size_t> (keyframeArray->size()));

            for (const auto& keyframeValue : *keyframeArray)
            {
                auto* keyframeObject = keyframeValue.getDynamicObject();
                if (keyframeObject == nullptr)
                    continue;

                Keyframe keyframe;
                keyframe.timeSeconds = static_cast<double> (keyframeObject->getProperty ("timeSeconds"));
                keyframe.value = static_cast<float> (double (keyframeObject->getProperty ("value")));
                keyframe.curve = keyframeCurveFromVar (keyframeObject->getProperty ("curve"));
                keyframes.push_back (keyframe);
            }
        }

        if (! keyframes.empty())
        {
            KeyframeTrack track { parameterId };
            track.setKeyframes (std::move (keyframes));
            keyframeTimeline.addOrReplaceTrack (std::move (track));
        }
    }

    if (timeline->hasProperty ("durationSeconds"))
        keyframeTimeline.setDurationSeconds (static_cast<double> (timeline->getProperty ("durationSeconds")));

    if (timeline->hasProperty ("looping"))
        keyframeTimeline.setLooping (static_cast<bool> (timeline->getProperty ("looping")));

    if (timeline->hasProperty ("playbackRate"))
        keyframeTimeline.setPlaybackRate (static_cast<float> (double (timeline->getProperty ("playbackRate"))));

    if (timeline->hasProperty ("currentTimeSeconds"))
        keyframeTimeline.setCurrentTimeSeconds (static_cast<double> (timeline->getProperty ("currentTimeSeconds")));

    if (! keyframeTimeline.hasAnyTrack())
        initialiseDefaultKeyframeTimeline();

    return true;
}

juce::var LocusQAudioProcessor::getKeyframeTimelineForUI() const
{
    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    return serialiseKeyframeTimelineLocked();
}

bool LocusQAudioProcessor::setKeyframeTimelineFromUI (const juce::var& timelineState)
{
    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    return applyKeyframeTimelineLocked (timelineState);
}

bool LocusQAudioProcessor::setTimelineCurrentTimeFromUI (double timeSeconds)
{
    if (! std::isfinite (timeSeconds))
        return false;

    const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
    const auto clamped = juce::jlimit (0.0,
                                       juce::jmax (0.0, keyframeTimeline.getDurationSeconds()),
                                       timeSeconds);
    keyframeTimeline.setCurrentTimeSeconds (clamped);
    return true;
}

juce::String LocusQAudioProcessor::sanitisePresetName (const juce::String& presetName)
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

juce::String LocusQAudioProcessor::sanitiseEmitterLabel (const juce::String& label)
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

    constexpr int maxChars = 31;
    if (filtered.length() > maxChars)
        filtered = filtered.substring (0, maxChars);

    return filtered.trim();
}

juce::File LocusQAudioProcessor::getPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile ("LocusQ")
        .getChildFile ("Presets");
}

juce::String LocusQAudioProcessor::getSnapshotOutputLayout() const
{
    return outputLayoutToString (getBusesLayout().getMainOutputChannelSet());
}

int LocusQAudioProcessor::getSnapshotOutputChannels() const
{
    const auto outputChannels = getMainBusNumOutputChannels();
    if (outputChannels > 0)
        return outputChannels;

    return juce::jmax (1, getTotalNumOutputChannels());
}

void LocusQAudioProcessor::setIntegerParameterValueNotifyingHost (const char* parameterId, int value)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (value)));
}

void LocusQAudioProcessor::migrateSnapshotLayoutIfNeeded (const juce::ValueTree& restoredState)
{
    int storedOutputChannels = 0;
    if (restoredState.hasProperty (kSnapshotOutputChannelsProperty))
    {
        storedOutputChannels = juce::jlimit (1,
                                             SpatialRenderer::NUM_SPEAKERS,
                                             static_cast<int> (restoredState.getProperty (kSnapshotOutputChannelsProperty)));
    }
    else if (restoredState.hasProperty (kSnapshotOutputLayoutProperty))
    {
        const auto storedLayout = restoredState.getProperty (kSnapshotOutputLayoutProperty).toString().trim().toLowerCase();
        if (storedLayout == "mono")
            storedOutputChannels = 1;
        else if (storedLayout == "stereo")
            storedOutputChannels = 2;
        else if (storedLayout == "quad" || storedLayout == "multichannel")
            storedOutputChannels = SpatialRenderer::NUM_SPEAKERS;
    }

    const auto currentOutputChannels = juce::jlimit (1,
                                                     SpatialRenderer::NUM_SPEAKERS,
                                                     getSnapshotOutputChannels());
    const auto isLegacySnapshot = ! restoredState.hasProperty (kSnapshotSchemaProperty);
    const auto hasLayoutMismatch = (storedOutputChannels > 0 && storedOutputChannels != currentOutputChannels);

    if (! isLegacySnapshot && ! hasLayoutMismatch)
        return;

    std::array<int, SpatialRenderer::NUM_SPEAKERS> migratedSpeakerMap { 1, 2, 3, 4 };

    if (currentOutputChannels == 1)
    {
        migratedSpeakerMap.fill (1);
    }
    else if (currentOutputChannels == 2)
    {
        migratedSpeakerMap = { 1, 2, 1, 2 };
    }

    setIntegerParameterValueNotifyingHost ("cal_spk1_out", migratedSpeakerMap[0]);
    setIntegerParameterValueNotifyingHost ("cal_spk2_out", migratedSpeakerMap[1]);
    setIntegerParameterValueNotifyingHost ("cal_spk3_out", migratedSpeakerMap[2]);
    setIntegerParameterValueNotifyingHost ("cal_spk4_out", migratedSpeakerMap[3]);
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
    if (! file.existsAsFile())
        return std::nullopt;

    const auto payload = juce::JSON::parse (file.loadFileAsString());
    if (payload.isVoid())
        return std::nullopt;

    return payload;
}

bool LocusQAudioProcessor::writeJsonToFile (const juce::File& file, const juce::var& payload)
{
    return file.replaceWithText (juce::JSON::toString (payload, true));
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

juce::var LocusQAudioProcessor::buildEmitterPresetLocked (const juce::String& presetName) const
{
    juce::var presetVar (new juce::DynamicObject());
    auto* preset = presetVar.getDynamicObject();

    preset->setProperty ("schema", kEmitterPresetSchemaV2);
    preset->setProperty ("name", presetName);
    preset->setProperty ("savedAtUtc", juce::Time::getCurrentTime().toISO8601 (true));

    juce::var layoutVar (new juce::DynamicObject());
    auto* layout = layoutVar.getDynamicObject();
    layout->setProperty ("outputLayout", getSnapshotOutputLayout());
    layout->setProperty ("outputChannels", getSnapshotOutputChannels());
    preset->setProperty (kEmitterPresetLayoutProperty, layoutVar);

    juce::var parametersVar (new juce::DynamicObject());
    auto* parameters = parametersVar.getDynamicObject();
    for (const auto* parameterId : kEmitterPresetParameterIds)
    {
        if (auto* parameter = apvts.getParameter (parameterId))
            parameters->setProperty (parameterId, parameter->getValue());
    }

    preset->setProperty ("parameters", parametersVar);
    preset->setProperty ("timeline", serialiseKeyframeTimelineLocked());
    return presetVar;
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

    if (auto* parameters = preset->getProperty ("parameters").getDynamicObject())
    {
        for (const auto* parameterId : kEmitterPresetParameterIds)
        {
            if (parameters->hasProperty (parameterId))
            {
                if (auto* parameter = apvts.getParameter (parameterId))
                {
                    const auto normalized = juce::jlimit (0.0f, 1.0f, static_cast<float> (double (parameters->getProperty (parameterId))));
                    parameter->setValueNotifyingHost (normalized);
                }
            }
        }
    }

    if (preset->hasProperty ("timeline"))
        applyKeyframeTimelineLocked (preset->getProperty ("timeline"));

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
        if (const auto payload = readJsonFromFile (file))
        {
            if (auto* preset = payload->getDynamicObject())
            {
                if (preset->hasProperty ("name"))
                    displayName = preset->getProperty ("name").toString();
            }
        }

        entry->setProperty ("name", displayName);
        entry->setProperty ("file", file.getFileName());
        entry->setProperty ("path", file.getFullPathName());
        entry->setProperty ("modifiedUtc", file.getLastModificationTime().toISO8601 (true));
        presets.add (entryVar);
    }

    return juce::var (presets);
}

juce::var LocusQAudioProcessor::saveEmitterPresetFromUI (const juce::var& options)
{
    juce::String requestedName = "Preset";
    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("name"))
            requestedName = optionsObject->getProperty ("name").toString();
    }

    requestedName = requestedName.trim();
    if (requestedName.isEmpty())
        requestedName = "Preset_" + juce::String (juce::Time::getCurrentTime().toMilliseconds());

    const auto safeName = sanitisePresetName (requestedName);
    auto presetDir = getPresetDirectory();
    presetDir.createDirectory();
    const auto presetFile = presetDir.getChildFile (safeName + ".json");

    juce::var presetPayload;
    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        presetPayload = buildEmitterPresetLocked (requestedName);
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! writeJsonToFile (presetFile, presetPayload))
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Failed to write preset file.");
        return response;
    }

    result->setProperty ("ok", true);
    result->setProperty ("name", requestedName);
    result->setProperty ("file", presetFile.getFileName());
    result->setProperty ("path", presetFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::loadEmitterPresetFromUI (const juce::var& options)
{
    juce::String presetPath;
    juce::String presetName;

    if (auto* optionsObject = options.getDynamicObject(); optionsObject != nullptr)
    {
        if (optionsObject->hasProperty ("path"))
            presetPath = optionsObject->getProperty ("path").toString();
        if (optionsObject->hasProperty ("name"))
            presetName = optionsObject->getProperty ("name").toString();
        if (optionsObject->hasProperty ("file") && presetName.isEmpty())
            presetName = juce::File (optionsObject->getProperty ("file").toString()).getFileNameWithoutExtension();
    }

    juce::File presetFile;
    if (presetPath.isNotEmpty())
    {
        presetFile = juce::File (presetPath);
    }
    else if (presetName.isNotEmpty())
    {
        const auto safeName = sanitisePresetName (presetName);
        presetFile = getPresetDirectory().getChildFile (safeName + ".json");
    }

    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    if (! presetFile.existsAsFile())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file not found.");
        return response;
    }

    const auto payload = readJsonFromFile (presetFile);
    if (! payload.has_value())
    {
        result->setProperty ("ok", false);
        result->setProperty ("message", "Preset file is invalid JSON.");
        return response;
    }

    {
        const juce::SpinLock::ScopedLockType timelineLock (keyframeTimelineLock);
        if (! applyEmitterPresetLocked (*payload))
        {
            result->setProperty ("ok", false);
            result->setProperty ("message", "Preset payload is not compatible.");
            return response;
        }
    }

    result->setProperty ("ok", true);
    result->setProperty ("file", presetFile.getFileName());
    result->setProperty ("path", presetFile.getFullPathName());
    return response;
}

juce::var LocusQAudioProcessor::getUIStateFromUI() const
{
    juce::var stateVar (new juce::DynamicObject());
    auto* state = stateVar.getDynamicObject();

    juce::String emitterLabelSnapshot;
    juce::String physicsPresetSnapshot;
    {
        const juce::SpinLock::ScopedLockType uiStateScopedLock (uiStateLock);
        emitterLabelSnapshot = emitterLabelState;
        physicsPresetSnapshot = physicsPresetState;
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

    state->setProperty ("emitterLabel", sanitiseEmitterLabel (emitterLabelSnapshot));
    state->setProperty ("physicsPreset", physicsPresetSnapshot);
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

    return changed;
}

void LocusQAudioProcessor::updatePerfEma (double& accumulator, double sampleMs) noexcept
{
    if (sampleMs <= 0.0)
        return;

    constexpr double alpha = 0.08;
    if (accumulator <= 0.0)
        accumulator = sampleMs;
    else
        accumulator += (sampleMs - accumulator) * alpha;
}

//==============================================================================
juce::AudioProcessorEditor* LocusQAudioProcessor::createEditor()
{
#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
    return nullptr;
#else
    return new LocusQAudioProcessorEditor (*this);
#endif
}

bool LocusQAudioProcessor::hasEditor() const
{
#if defined (LOCUSQ_TESTING) && LOCUSQ_TESTING
    return false;
#else
    return true;
#endif
}

//==============================================================================
const juce::String LocusQAudioProcessor::getName() const { return JucePlugin_Name; }
bool LocusQAudioProcessor::acceptsMidi() const { return false; }
bool LocusQAudioProcessor::producesMidi() const { return false; }
bool LocusQAudioProcessor::isMidiEffect() const { return false; }
double LocusQAudioProcessor::getTailLengthSeconds() const { return 2.0; }

//==============================================================================
int LocusQAudioProcessor::getNumPrograms() { return 1; }
int LocusQAudioProcessor::getCurrentProgram() { return 0; }
void LocusQAudioProcessor::setCurrentProgram (int) {}
const juce::String LocusQAudioProcessor::getProgramName (int) { return {}; }
void LocusQAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
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
            migrateSnapshotLayoutIfNeeded (state);

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

//==============================================================================
// PARAMETER LAYOUT - All 76 parameters
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout LocusQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ==================== GLOBAL ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Calibrate", "Emitter", "Renderer" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    // ==================== CALIBRATE ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_spk_config", 1 }, "Speaker Config",
        juce::StringArray { "4x Mono", "2x Stereo" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_mic_channel", 1 }, "Mic Channel", 1, 8, 1));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk1_out", 1 }, "SPK1 Output", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk2_out", 1 }, "SPK2 Output", 1, 8, 2));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk3_out", 1 }, "SPK3 Output", 1, 8, 3));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk4_out", 1 }, "SPK4 Output", 1, 8, 4));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cal_test_level", 1 }, "Test Level",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_test_type", 1 }, "Test Type",
        juce::StringArray { "Sweep", "Pink", "White", "Impulse" }, 0));

    // ==================== EMITTER: POSITION ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_azimuth", 1 }, "Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_elevation", 1 }, "Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f, 0.5f), 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_x", 1 }, "Position X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_y", 1 }, "Position Y",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_z", 1 }, "Position Z",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pos_coord_mode", 1 }, "Coord Mode",
        juce::StringArray { "Spherical", "Cartesian" }, 0));

    // ==================== EMITTER: SIZE ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_height", 1 }, "Height",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "size_link", 1 }, "Link Size", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_uniform", 1 }, "Uniform Scale",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    // ==================== EMITTER: AUDIO ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_gain", 1 }, "Emitter Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_mute", 1 }, "Mute", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_solo", 1 }, "Solo", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_spread", 1 }, "Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_directivity", 1 }, "Directivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_azimuth", 1 }, "Dir Aim Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_elevation", 1 }, "Dir Aim Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    // ==================== EMITTER: PHYSICS ====================
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_enable", 1 }, "Physics Enable", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_mass", 1 }, "Mass",
        juce::NormalisableRange<float> (0.01f, 100.0f, 0.01f, 0.4f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_drag", 1 }, "Drag",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_elasticity", 1 }, "Elasticity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_gravity", 1 }, "Gravity",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_gravity_dir", 1 }, "Gravity Direction",
        juce::StringArray { "Down", "Up", "To Center", "From Center", "Custom" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_friction", 1 }, "Friction",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_x", 1 }, "Init Vel X",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_y", 1 }, "Init Vel Y",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_z", 1 }, "Init Vel Z",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_throw", 1 }, "Throw", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_reset", 1 }, "Reset Position", false));

    // ==================== EMITTER: ANIMATION ====================
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_enable", 1 }, "Animation Enable", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "anim_mode", 1 }, "Animation Source",
        juce::StringArray { "DAW", "Internal" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_loop", 1 }, "Loop", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "anim_speed", 1 }, "Animation Speed",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_sync", 1 }, "Transport Sync", true));

    // ==================== EMITTER: IDENTITY ====================
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "emit_color", 1 }, "Color", 0, 15, 0));

    // ==================== RENDERER: MASTER ====================
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_master_gain", 1 }, "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_gain", 1 }, "SPK1 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_gain", 1 }, "SPK2 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_gain", 1 }, "SPK3 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_gain", 1 }, "SPK4 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_delay", 1 }, "SPK1 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_delay", 1 }, "SPK2 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_delay", 1 }, "SPK3 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_delay", 1 }, "SPK4 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));

    // ==================== RENDERER: SPATIALIZATION ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_quality", 1 }, "Quality",
        juce::StringArray { "Draft", "Final" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_distance_model", 1 }, "Distance Model",
        juce::StringArray { "Inverse Square", "Linear", "Logarithmic", "Custom" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_ref", 1 }, "Ref Distance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_max", 1 }, "Max Distance",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_doppler", 1 }, "Doppler", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_doppler_scale", 1 }, "Doppler Scale",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_air_absorb", 1 }, "Air Absorption", true));

    // ==================== RENDERER: ROOM ====================
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_enable", 1 }, "Room Enable", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_mix", 1 }, "Room Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_size", 1 }, "Room Size",
        juce::NormalisableRange<float> (0.5f, 5.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_damping", 1 }, "Room Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_er_only", 1 }, "ER Only", false));

    // ==================== RENDERER: PHYSICS GLOBAL ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_phys_rate", 1 }, "Physics Rate",
        juce::StringArray { "30 Hz", "60 Hz", "120 Hz", "240 Hz" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_walls", 1 }, "Wall Collision", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_interact", 1 }, "Object Interaction", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_pause", 1 }, "Pause Physics", false));

    // ==================== RENDERER: VISUALIZATION ====================
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_viz_mode", 1 }, "View Mode",
        juce::StringArray { "Perspective", "Top Down", "Front", "Side" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_trails", 1 }, "Show Trails", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_trail_len", 1 }, "Trail Length",
        juce::NormalisableRange<float> (0.5f, 30.0f, 0.1f), 5.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_vectors", 1 }, "Show Vectors", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_grid", 1 }, "Show Grid", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_labels", 1 }, "Show Labels", true));

    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LocusQAudioProcessor();
}
