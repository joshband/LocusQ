#pragma once

// Extracted non-RT scene snapshot bridge serialization logic from PluginProcessor.cpp.
juce::String LocusQAudioProcessor::getSceneStateJSON()
{
    const auto effectiveWritableChannels = resolveCalibrationWritableChannels (
        getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        getCurrentCalibrationSpeakerRouting());
    applyAutoDetectedCalibrationRoutingIfAppropriate (effectiveWritableChannels, false);

    const auto snapshotSeq = ++sceneSnapshotSequence;
    const auto snapshotPublishedAtUtcMs = juce::Time::getCurrentTime().toMilliseconds();

    // Build JSON scene snapshot for WebView
    juce::String json = "{\"snapshotSchema\":\"" + juce::String (kSceneSnapshotSchemaProperty) + "\""
                      + ",\"snapshotSeq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
                      + ",\"profileSyncSeq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
                      + ",\"snapshotPublishedAtUtcMs\":" + juce::String (snapshotPublishedAtUtcMs)
                      + ",\"snapshotCadenceHz\":" + juce::String (kSceneSnapshotCadenceHz)
                      + ",\"snapshotStaleAfterMs\":" + juce::String (kSceneSnapshotStaleAfterMs)
                      + ",\"emitters\":[";
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
    const auto rendererHeadphoneModeRequestedIndex = juce::jlimit (
        0,
        1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_mode")->load())));
    const auto rendererHeadphoneProfileRequestedIndex = juce::jlimit (
        0,
        3,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_headphone_profile")->load())));
    const bool rendererAuditionEnabled = apvts.getRawParameterValue ("rend_audition_enable")->load() > 0.5f;
    const int rendererAuditionSignalIndex = juce::jlimit (
        0,
        static_cast<int> (kRendererAuditionSignalIds.size()) - 1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_audition_signal")->load())));
    const int rendererAuditionMotionIndex = juce::jlimit (
        0,
        static_cast<int> (kRendererAuditionMotionIds.size()) - 1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_audition_motion")->load())));
    const int rendererAuditionLevelIndex = juce::jlimit (
        0,
        static_cast<int> (kRendererAuditionLevelDbValues.size()) - 1,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_audition_level")->load())));
    const juce::String rendererAuditionSignal { rendererAuditionSignalIdForIndex (rendererAuditionSignalIndex) };
    const juce::String rendererAuditionMotion { rendererAuditionMotionIdForIndex (rendererAuditionMotionIndex) };
    const float rendererAuditionLevelDb = rendererAuditionLevelDbForIndex (rendererAuditionLevelIndex);
    const bool rendererAuditionVisualReportedActive = spatialRenderer.isAuditionVisualActive();
    const float rendererAuditionVisualXRaw = spatialRenderer.getAuditionVisualX();
    const float rendererAuditionVisualYRaw = spatialRenderer.getAuditionVisualY();
    const float rendererAuditionVisualZRaw = spatialRenderer.getAuditionVisualZ();
    const bool rendererAuditionVisualFinite = isFiniteVector3 (
        rendererAuditionVisualXRaw,
        rendererAuditionVisualYRaw,
        rendererAuditionVisualZRaw);
    const bool rendererAuditionVisualInvalid = rendererAuditionVisualReportedActive && ! rendererAuditionVisualFinite;
    const bool rendererAuditionVisualActive = rendererAuditionVisualReportedActive && rendererAuditionVisualFinite;
    const float rendererAuditionVisualX = rendererAuditionVisualFinite ? rendererAuditionVisualXRaw : 0.0f;
    const float rendererAuditionVisualY = rendererAuditionVisualFinite ? rendererAuditionVisualYRaw : 1.2f;
    const float rendererAuditionVisualZ = rendererAuditionVisualFinite ? rendererAuditionVisualZRaw : -1.0f;
    const float rendererAuditionLevelNorm = static_cast<float> (rendererAuditionLevelIndex)
        / static_cast<float> (juce::jmax (1, static_cast<int> (kRendererAuditionLevelDbValues.size()) - 1));
    bool rendererAuditionCloudEnabled = rendererAuditionEnabled && rendererAuditionVisualActive;
    juce::String rendererAuditionCloudPattern { "tone_core" };
    int rendererAuditionCloudPointCountBase = 24;
    float rendererAuditionCloudSpreadBaseMeters = 0.45f;
    float rendererAuditionCloudPulseBaseHz = 1.9f;
    float rendererAuditionCloudCoherenceBase = 0.92f;
    juce::String rendererAuditionCloudMode { "single_core" };
    int rendererAuditionCloudEmitterCountBase = 1;
    float rendererAuditionCloudVerticalSpreadScale = 0.35f;
    switch (rendererAuditionSignalIndex)
    {
        case 0: // sine_440
            rendererAuditionCloudPattern = "tone_core";
            rendererAuditionCloudPointCountBase = 24;
            rendererAuditionCloudSpreadBaseMeters = 0.45f;
            rendererAuditionCloudPulseBaseHz = 1.9f;
            rendererAuditionCloudCoherenceBase = 0.92f;
            rendererAuditionCloudMode = "single_core";
            rendererAuditionCloudEmitterCountBase = 1;
            rendererAuditionCloudVerticalSpreadScale = 0.30f;
            break;
        case 1: // dual_tone
            rendererAuditionCloudPattern = "dual_orbit";
            rendererAuditionCloudPointCountBase = 28;
            rendererAuditionCloudSpreadBaseMeters = 0.55f;
            rendererAuditionCloudPulseBaseHz = 2.2f;
            rendererAuditionCloudCoherenceBase = 0.85f;
            rendererAuditionCloudMode = "dual_pair";
            rendererAuditionCloudEmitterCountBase = 2;
            rendererAuditionCloudVerticalSpreadScale = 0.34f;
            break;
        case 2: // pink_noise
            rendererAuditionCloudPattern = "noise_halo";
            rendererAuditionCloudPointCountBase = 42;
            rendererAuditionCloudSpreadBaseMeters = 0.70f;
            rendererAuditionCloudPulseBaseHz = 1.4f;
            rendererAuditionCloudCoherenceBase = 0.42f;
            rendererAuditionCloudMode = "noise_cluster";
            rendererAuditionCloudEmitterCountBase = 3;
            rendererAuditionCloudVerticalSpreadScale = 0.42f;
            break;
        case 3: // rain_field
            rendererAuditionCloudPattern = "rain_sheet";
            rendererAuditionCloudPointCountBase = 112;
            rendererAuditionCloudSpreadBaseMeters = 2.85f;
            rendererAuditionCloudPulseBaseHz = 3.2f;
            rendererAuditionCloudCoherenceBase = 0.24f;
            rendererAuditionCloudMode = "precipitation_rain";
            rendererAuditionCloudEmitterCountBase = 6;
            rendererAuditionCloudVerticalSpreadScale = 1.35f;
            break;
        case 4: // snow_drift
            rendererAuditionCloudPattern = "snow_cloud";
            rendererAuditionCloudPointCountBase = 104;
            rendererAuditionCloudSpreadBaseMeters = 3.10f;
            rendererAuditionCloudPulseBaseHz = 0.9f;
            rendererAuditionCloudCoherenceBase = 0.18f;
            rendererAuditionCloudMode = "precipitation_snow";
            rendererAuditionCloudEmitterCountBase = 7;
            rendererAuditionCloudVerticalSpreadScale = 1.05f;
            break;
        case 5: // bouncing_balls
            rendererAuditionCloudPattern = "bounce_cluster";
            rendererAuditionCloudPointCountBase = 74;
            rendererAuditionCloudSpreadBaseMeters = 2.45f;
            rendererAuditionCloudPulseBaseHz = 2.7f;
            rendererAuditionCloudCoherenceBase = 0.58f;
            rendererAuditionCloudMode = "impact_swarm";
            rendererAuditionCloudEmitterCountBase = 4;
            rendererAuditionCloudVerticalSpreadScale = 0.82f;
            break;
        case 6: // wind_chimes
            rendererAuditionCloudPattern = "chime_constellation";
            rendererAuditionCloudPointCountBase = 40;
            rendererAuditionCloudSpreadBaseMeters = 1.45f;
            rendererAuditionCloudPulseBaseHz = 1.2f;
            rendererAuditionCloudCoherenceBase = 0.58f;
            rendererAuditionCloudMode = "chime_cluster";
            rendererAuditionCloudEmitterCountBase = 5;
            rendererAuditionCloudVerticalSpreadScale = 0.72f;
            break;
        case 7: // crickets
            rendererAuditionCloudPattern = "cricket_field";
            rendererAuditionCloudPointCountBase = 76;
            rendererAuditionCloudSpreadBaseMeters = 2.60f;
            rendererAuditionCloudPulseBaseHz = 4.1f;
            rendererAuditionCloudCoherenceBase = 0.28f;
            rendererAuditionCloudMode = "bio_swarm";
            rendererAuditionCloudEmitterCountBase = 6;
            rendererAuditionCloudVerticalSpreadScale = 0.54f;
            break;
        case 8: // song_birds
            rendererAuditionCloudPattern = "songbird_canopy";
            rendererAuditionCloudPointCountBase = 68;
            rendererAuditionCloudSpreadBaseMeters = 2.95f;
            rendererAuditionCloudPulseBaseHz = 1.5f;
            rendererAuditionCloudCoherenceBase = 0.40f;
            rendererAuditionCloudMode = "bio_flock";
            rendererAuditionCloudEmitterCountBase = 6;
            rendererAuditionCloudVerticalSpreadScale = 1.10f;
            break;
        case 9: // karplus_plucks
            rendererAuditionCloudPattern = "pluck_strings";
            rendererAuditionCloudPointCountBase = 52;
            rendererAuditionCloudSpreadBaseMeters = 2.05f;
            rendererAuditionCloudPulseBaseHz = 1.9f;
            rendererAuditionCloudCoherenceBase = 0.52f;
            rendererAuditionCloudMode = "physical_modal";
            rendererAuditionCloudEmitterCountBase = 4;
            rendererAuditionCloudVerticalSpreadScale = 0.70f;
            break;
        case 10: // membrane_drops
            rendererAuditionCloudPattern = "membrane_impacts";
            rendererAuditionCloudPointCountBase = 60;
            rendererAuditionCloudSpreadBaseMeters = 2.30f;
            rendererAuditionCloudPulseBaseHz = 2.2f;
            rendererAuditionCloudCoherenceBase = 0.48f;
            rendererAuditionCloudMode = "physical_impacts";
            rendererAuditionCloudEmitterCountBase = 5;
            rendererAuditionCloudVerticalSpreadScale = 0.78f;
            break;
        case 11: // krell_patch
            rendererAuditionCloudPattern = "krell_glide";
            rendererAuditionCloudPointCountBase = 58;
            rendererAuditionCloudSpreadBaseMeters = 2.40f;
            rendererAuditionCloudPulseBaseHz = 1.3f;
            rendererAuditionCloudCoherenceBase = 0.44f;
            rendererAuditionCloudMode = "synth_generative";
            rendererAuditionCloudEmitterCountBase = 4;
            rendererAuditionCloudVerticalSpreadScale = 0.96f;
            break;
        case 12: // generative_arp
            rendererAuditionCloudPattern = "arp_lattice";
            rendererAuditionCloudPointCountBase = 62;
            rendererAuditionCloudSpreadBaseMeters = 2.55f;
            rendererAuditionCloudPulseBaseHz = 2.4f;
            rendererAuditionCloudCoherenceBase = 0.50f;
            rendererAuditionCloudMode = "synth_grid";
            rendererAuditionCloudEmitterCountBase = 5;
            rendererAuditionCloudVerticalSpreadScale = 0.88f;
            break;
        default:
            break;
    }
    float rendererAuditionCloudMotionSpreadScale = 0.85f;
    float rendererAuditionCloudMotionPulseScale = 1.0f;
    float rendererAuditionCloudMotionCoherenceScale = 1.0f;
    switch (rendererAuditionMotionIndex)
    {
        case 0: // center
            rendererAuditionCloudMotionSpreadScale = 0.85f;
            rendererAuditionCloudMotionPulseScale = 1.0f;
            rendererAuditionCloudMotionCoherenceScale = 1.0f;
            break;
        case 1: // orbit_slow
            rendererAuditionCloudMotionSpreadScale = 1.10f;
            rendererAuditionCloudMotionPulseScale = 1.15f;
            rendererAuditionCloudMotionCoherenceScale = 0.88f;
            break;
        case 2: // orbit_fast
            rendererAuditionCloudMotionSpreadScale = 1.35f;
            rendererAuditionCloudMotionPulseScale = 1.35f;
            rendererAuditionCloudMotionCoherenceScale = 0.72f;
            break;
        case 3: // figure8_flow
            rendererAuditionCloudMotionSpreadScale = 1.52f;
            rendererAuditionCloudMotionPulseScale = 1.22f;
            rendererAuditionCloudMotionCoherenceScale = 0.76f;
            break;
        case 4: // helix_rise
            rendererAuditionCloudMotionSpreadScale = 1.70f;
            rendererAuditionCloudMotionPulseScale = 1.42f;
            rendererAuditionCloudMotionCoherenceScale = 0.68f;
            break;
        case 5: // wall_ricochet
            rendererAuditionCloudMotionSpreadScale = 1.88f;
            rendererAuditionCloudMotionPulseScale = 1.56f;
            rendererAuditionCloudMotionCoherenceScale = 0.62f;
            break;
        default:
            break;
    }
    bool rendererAuditionCloudBoundsAdjusted = false;
    const int rendererAuditionCloudPointCountRaw = static_cast<int> (std::lround (
        rendererAuditionCloudPointCountBase * (1.0f + 0.25f * rendererAuditionLevelNorm)));
    int rendererAuditionCloudPointCount = rendererAuditionCloudEnabled
        ? sanitizeBoundedInt (
            rendererAuditionCloudPointCountRaw,
            8,
            kRendererAuditionCloudMaxPoints,
            &rendererAuditionCloudBoundsAdjusted)
        : 0;
    float rendererAuditionCloudSpreadMeters = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.20f,
            6.0f,
            rendererAuditionCloudSpreadBaseMeters * rendererAuditionCloudMotionSpreadScale
                * (0.90f + 0.20f * rendererAuditionLevelNorm))
        : 0.0f;
    float rendererAuditionCloudPulseHz = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.2f,
            6.0f,
            rendererAuditionCloudPulseBaseHz * rendererAuditionCloudMotionPulseScale
                * (0.95f + 0.15f * rendererAuditionLevelNorm))
        : 0.0f;
    float rendererAuditionCloudCoherence = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.05f,
            0.99f,
            rendererAuditionCloudCoherenceBase * rendererAuditionCloudMotionCoherenceScale
                + (rendererAuditionVisualActive ? 0.04f : -0.06f))
        : 0.0f;
    const bool rendererAuditionCloudGeometryInvalid = rendererAuditionCloudEnabled
        && (! std::isfinite (rendererAuditionCloudSpreadMeters)
            || ! std::isfinite (rendererAuditionCloudPulseHz)
            || ! std::isfinite (rendererAuditionCloudCoherence));
    if (rendererAuditionCloudGeometryInvalid)
    {
        rendererAuditionCloudSpreadMeters = 0.0f;
        rendererAuditionCloudPulseHz = 0.0f;
        rendererAuditionCloudCoherence = 0.0f;
        rendererAuditionCloudPointCount = 0;
    }
    juce::uint32 rendererAuditionCloudSeed = 0xA39F1C2Du;
    const auto rendererAuditionSignalOrdinal = static_cast<juce::uint32> (juce::jmax (0, rendererAuditionSignalIndex) + 1);
    const auto rendererAuditionMotionOrdinal = static_cast<juce::uint32> (juce::jmax (0, rendererAuditionMotionIndex) + 1);
    const auto rendererAuditionLevelOrdinal = static_cast<juce::uint32> (juce::jmax (0, rendererAuditionLevelIndex) + 1);
    rendererAuditionCloudSeed ^= rendererAuditionSignalOrdinal * 0x9E3779B9u;
    rendererAuditionCloudSeed = (rendererAuditionCloudSeed << 13u) | (rendererAuditionCloudSeed >> 19u);
    rendererAuditionCloudSeed ^= rendererAuditionMotionOrdinal * 0x85EBCA6Bu;
    rendererAuditionCloudSeed ^= rendererAuditionLevelOrdinal * 0xC2B2AE35u;
    if (rendererAuditionVisualActive)
        rendererAuditionCloudSeed ^= 0x1B873593u;
    int rendererAuditionCloudEmitterCount = 0;
    if (rendererAuditionCloudEnabled)
    {
        const int motionEmitterBoost = rendererAuditionMotionIndex == 2 ? 2 : (rendererAuditionMotionIndex == 1 ? 1 : 0);
        const int levelEmitterBoost = rendererAuditionLevelIndex >= 3 ? 1 : 0;
        const int visualEmitterBoost = rendererAuditionVisualActive ? 1 : 0;
        const auto emitterCountRaw =
            rendererAuditionCloudEmitterCountBase + motionEmitterBoost + levelEmitterBoost + visualEmitterBoost;
        rendererAuditionCloudEmitterCount = sanitizeBoundedInt (
            emitterCountRaw,
            1,
            kRendererAuditionCloudMaxEmitters,
            &rendererAuditionCloudBoundsAdjusted);
    }

    // BL-029 Slice B1: renderer-authoritative audition binding resolver.
    const auto currentMode = getCurrentMode();
    const juce::String rendererAuditionSourceMode {
        rendererAuditionCloudEnabled ? "cloud" : "single"
    };
    juce::String rendererAuditionRequestedMode { rendererAuditionSourceMode };
    juce::String rendererAuditionResolvedMode { rendererAuditionSourceMode };
    juce::String rendererAuditionBindingTarget { "none" };
    bool rendererAuditionBindingAvailable = false;
    int preferredEmitterBindingId = -1;
    int preferredPhysicsBindingId = -1;

    if (emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId))
    {
        const auto emitterData = sceneGraph.getSlot (emitterSlotId).read();
        if (emitterData.active)
        {
            preferredEmitterBindingId = emitterSlotId;
            if (emitterData.physicsEnabled)
                preferredPhysicsBindingId = emitterSlotId;
        }
    }

    for (int slot = 0; slot < SceneGraph::MAX_EMITTERS; ++slot)
    {
        if (! sceneGraph.isSlotActive (slot))
            continue;

        const auto slotData = sceneGraph.getSlot (slot).read();
        if (! slotData.active)
            continue;

        if (preferredEmitterBindingId < 0)
            preferredEmitterBindingId = slot;

        if (slotData.physicsEnabled && preferredPhysicsBindingId < 0)
            preferredPhysicsBindingId = slot;
    }

    const bool choreographyBindingRequested = apvts.getRawParameterValue ("anim_enable")->load() > 0.5f
        && static_cast<int> (std::lround (apvts.getRawParameterValue ("anim_mode")->load())) == 1;
    const bool physicsBindingRequested = apvts.getRawParameterValue ("rend_phys_interact")->load() > 0.5f;
    const bool emitterBindingRequested = sceneGraph.getActiveEmitterCount() > 0;

    if (rendererAuditionEnabled)
    {
        if (physicsBindingRequested)
            rendererAuditionRequestedMode = "bound_physics";
        else if (choreographyBindingRequested)
            rendererAuditionRequestedMode = "bound_choreography";
        else if (emitterBindingRequested)
            rendererAuditionRequestedMode = "bound_emitter";
    }

    float rendererAuditionDensity = rendererAuditionCloudEnabled
        ? juce::jlimit (0.0f, 1.0f, static_cast<float> (rendererAuditionCloudPointCount) / 160.0f)
        : 0.0f;
    float rendererAuditionReactivity = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.0f,
            1.0f,
            (0.58f * rendererAuditionLevelNorm) + (0.42f * (1.0f - rendererAuditionCloudCoherence)))
        : 0.0f;
    const bool rendererAuditionTransportSync = false;
    juce::String rendererAuditionFallbackReason { "none" };
    if (! rendererAuditionEnabled)
    {
        rendererAuditionFallbackReason = "audition_disabled";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (currentMode != LocusQMode::Renderer)
    {
        rendererAuditionFallbackReason = "renderer_mode_inactive";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (rendererAuditionVisualInvalid)
    {
        rendererAuditionFallbackReason = "visual_centroid_invalid";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (rendererAuditionRequestedMode == "bound_emitter")
    {
        if (preferredEmitterBindingId >= 0)
        {
            rendererAuditionResolvedMode = "bound_emitter";
            rendererAuditionBindingTarget = "emitter:" + juce::String (preferredEmitterBindingId);
            rendererAuditionBindingAvailable = true;
        }
        else
        {
            rendererAuditionFallbackReason = "bound_emitter_unavailable";
            rendererAuditionResolvedMode = rendererAuditionSourceMode;
        }
    }
    else if (rendererAuditionRequestedMode == "bound_choreography")
    {
        if (choreographyBindingRequested)
        {
            rendererAuditionResolvedMode = "bound_choreography";
            rendererAuditionBindingTarget = "timeline:global";
            rendererAuditionBindingAvailable = true;
        }
        else
        {
            rendererAuditionFallbackReason = "bound_choreography_unavailable";
            rendererAuditionResolvedMode = rendererAuditionSourceMode;
        }
    }
    else if (rendererAuditionRequestedMode == "bound_physics")
    {
        if (preferredPhysicsBindingId >= 0)
        {
            rendererAuditionResolvedMode = "bound_physics";
            rendererAuditionBindingTarget = "emitter:" + juce::String (preferredPhysicsBindingId);
            rendererAuditionBindingAvailable = true;
        }
        else
        {
            rendererAuditionFallbackReason = "bound_physics_unavailable";
            rendererAuditionResolvedMode = rendererAuditionSourceMode;
        }
    }
    else if (! rendererAuditionVisualActive)
    {
        rendererAuditionFallbackReason = "visual_centroid_unavailable";
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }
    else if (rendererAuditionSourceMode == "cloud" && rendererAuditionCloudGeometryInvalid)
    {
        rendererAuditionFallbackReason = "cloud_geometry_invalid";
        rendererAuditionResolvedMode = "single";
        rendererAuditionCloudEnabled = false;
        rendererAuditionCloudEmitterCount = 0;
        rendererAuditionCloudPointCount = 0;
    }
    else if (rendererAuditionSourceMode == "cloud" && rendererAuditionCloudEmitterCount <= 0)
    {
        rendererAuditionFallbackReason = "cloud_emitters_unavailable";
        rendererAuditionResolvedMode = "single";
        rendererAuditionCloudEnabled = false;
        rendererAuditionCloudPointCount = 0;
    }
    else
    {
        rendererAuditionResolvedMode = rendererAuditionSourceMode;
    }

    rendererAuditionDensity = rendererAuditionCloudEnabled
        ? juce::jlimit (0.0f, 1.0f, static_cast<float> (rendererAuditionCloudPointCount) / 160.0f)
        : 0.0f;
    rendererAuditionReactivity = rendererAuditionCloudEnabled
        ? juce::jlimit (
            0.0f,
            1.0f,
            (0.58f * rendererAuditionLevelNorm) + (0.42f * (1.0f - rendererAuditionCloudCoherence)))
        : 0.0f;
    if (rendererAuditionFallbackReason == "none"
        && rendererAuditionSourceMode == "cloud"
        && rendererAuditionCloudBoundsAdjusted)
    {
        rendererAuditionFallbackReason = "cloud_bounds_clamped";
    }

    auto auditionCloudHashUnit = [rendererAuditionCloudSeed] (int emitterIndex, juce::uint32 salt) -> float
    {
        juce::uint32 hash = rendererAuditionCloudSeed;
        const auto emitterOrdinal = static_cast<juce::uint32> (juce::jmax (0, emitterIndex) + 1);
        hash ^= emitterOrdinal * 0x9E3779B9u;
        hash ^= salt;
        hash ^= (hash >> 16u);
        hash *= 0x7FEB352Du;
        hash ^= (hash >> 15u);
        hash *= 0x846CA68Bu;
        hash ^= (hash >> 16u);
        return static_cast<float> (hash & 0x00FFFFFFu) / static_cast<float> (0x00FFFFFFu);
    };

    juce::String rendererAuditionCloudEmittersJson { "[" };
    if (rendererAuditionCloudEnabled && rendererAuditionCloudEmitterCount > 0)
    {
        constexpr float kTwoPi = 6.28318530717958647692f;
        const float baseAngleStep = kTwoPi / static_cast<float> (rendererAuditionCloudEmitterCount);
        const float motionPhaseBias = static_cast<float> (rendererAuditionMotionIndex) * 0.31f
                                      + rendererAuditionLevelNorm * 0.27f;
        const float spreadScaleFromMotion = juce::jlimit (
            0.5f,
            1.6f,
            0.65f + 0.35f * rendererAuditionCloudMotionSpreadScale);

        for (int emitterIndex = 0; emitterIndex < rendererAuditionCloudEmitterCount; ++emitterIndex)
        {
            const float unitRadius = auditionCloudHashUnit (emitterIndex, 0xA53C9E11u);
            const float unitAngleJitter = auditionCloudHashUnit (emitterIndex, 0x3C6EF372u);
            const float unitHeight = auditionCloudHashUnit (emitterIndex, 0xBB67AE85u);
            const float unitWeight = auditionCloudHashUnit (emitterIndex, 0xC2B2AE35u);
            const float unitActivity = auditionCloudHashUnit (emitterIndex, 0x27D4EB2Fu);

            const float angle = baseAngleStep * static_cast<float> (emitterIndex)
                                + motionPhaseBias
                                + (unitAngleJitter - 0.5f) * 0.75f;
            const float radialSpread = rendererAuditionCloudSpreadMeters * spreadScaleFromMotion
                                       * (0.25f + 0.75f * unitRadius);
            const float localOffsetX = std::cos (angle) * radialSpread;
            const float localOffsetZ = std::sin (angle) * radialSpread;
            const float localOffsetY = (unitHeight - 0.5f) * 2.0f
                                       * rendererAuditionCloudSpreadMeters
                                       * rendererAuditionCloudVerticalSpreadScale;
            const float weight = juce::jlimit (
                0.05f,
                1.0f,
                (1.0f / static_cast<float> (rendererAuditionCloudEmitterCount)) * (0.82f + 0.36f * unitWeight));
            const float phase = std::fmod (
                static_cast<float> (emitterIndex) / static_cast<float> (juce::jmax (1, rendererAuditionCloudEmitterCount))
                    + motionPhaseBias
                    + unitAngleJitter * 0.33f,
                1.0f);
            const float activity = juce::jlimit (
                0.0f,
                1.0f,
                0.32f
                    + 0.58f * rendererAuditionLevelNorm
                    + 0.22f * (1.0f - rendererAuditionCloudCoherence)
                    + 0.12f * unitActivity);

            if (emitterIndex > 0)
                rendererAuditionCloudEmittersJson << ",";

            rendererAuditionCloudEmittersJson << "{\"id\":" << juce::String (emitterIndex)
                                              << ",\"weight\":" << juce::String (weight, 4)
                                              << ",\"localOffsetX\":" << juce::String (localOffsetX, 3)
                                              << ",\"localOffsetY\":" << juce::String (localOffsetY, 3)
                                              << ",\"localOffsetZ\":" << juce::String (localOffsetZ, 3)
                                              << ",\"phase\":" << juce::String (phase, 4)
                                              << ",\"activity\":" << juce::String (activity, 4) << "}";
        }
    }
    rendererAuditionCloudEmittersJson << "]";
    const bool rendererAuditionReactiveActive =
        rendererAuditionEnabled
        && currentMode == LocusQMode::Renderer
        && rendererAuditionVisualActive;
    auto rendererAuditionReactive = makeNeutralAuditionReactiveSnapshot();
    bool rendererAuditionReactiveInvalid = false;
    bool rendererAuditionReactiveMissing = false;
    if (rendererAuditionReactiveActive)
    {
        const auto sanitizedReactivePayload = sanitizeAuditionReactivePayload (
            spatialRenderer.getAuditionReactiveSnapshot());
        rendererAuditionReactive = sanitizedReactivePayload.snapshot;
        rendererAuditionReactiveInvalid = sanitizedReactivePayload.invalidScalars
            || sanitizedReactivePayload.invalidBounds;
        rendererAuditionReactiveMissing = rendererAuditionCloudEnabled
            && rendererAuditionResolvedMode == "cloud"
            && rendererAuditionReactive.sourceEnergyCount <= 0;

        if (rendererAuditionReactiveInvalid || rendererAuditionReactiveMissing)
            rendererAuditionReactive = makeNeutralAuditionReactiveSnapshot();
    }

    if (rendererAuditionFallbackReason == "none" && rendererAuditionReactiveInvalid)
        rendererAuditionFallbackReason = "reactive_payload_invalid";
    else if (rendererAuditionFallbackReason == "none" && rendererAuditionReactiveMissing)
        rendererAuditionFallbackReason = "reactive_payload_missing";
    const bool rendererAuditionReactivePublishedActive = rendererAuditionReactiveActive
        && ! rendererAuditionReactiveInvalid
        && ! rendererAuditionReactiveMissing;

    const juce::String rendererAuditionReactiveHeadphoneFallbackReason {
        SpatialRenderer::auditionReactiveHeadphoneFallbackReasonToString (
            rendererAuditionReactive.headphoneFallbackReasonIndex)
    };
    const bool rendererAuditionReactiveHeadphoneFallback =
        rendererAuditionReactive.headphoneFallbackReasonIndex
            != static_cast<int> (SpatialRenderer::AuditionReactiveHeadphoneFallbackReason::None);
    bool rendererAuditionReactiveSourceBoundsAdjusted = false;
    const auto rendererAuditionSourceEnergyCount = sanitizeBoundedInt (
        rendererAuditionReactive.sourceEnergyCount,
        0,
        SpatialRenderer::MAX_AUDITION_REACTIVE_SOURCES,
        &rendererAuditionReactiveSourceBoundsAdjusted);
    if (rendererAuditionFallbackReason == "none" && rendererAuditionReactiveSourceBoundsAdjusted)
        rendererAuditionFallbackReason = "reactive_source_count_invalid";

    juce::String rendererAuditionSourceEnergyJson { "[" };
    juce::String rendererAuditionSourceEnergyNormJson { "[" };
    for (int sourceIndex = 0; sourceIndex < rendererAuditionSourceEnergyCount; ++sourceIndex)
    {
        if (sourceIndex > 0)
        {
            rendererAuditionSourceEnergyJson << ",";
            rendererAuditionSourceEnergyNormJson << ",";
        }
        const auto sourceEnergy = sanitizeUnitScalar (
            rendererAuditionReactive.sourceEnergy[static_cast<size_t> (sourceIndex)],
            0.0f);
        rendererAuditionSourceEnergyJson << juce::String (
            sourceEnergy,
            5);
        rendererAuditionSourceEnergyNormJson << juce::String (sourceEnergy, 5);
    }
    rendererAuditionSourceEnergyJson << "]";
    rendererAuditionSourceEnergyNormJson << "]";

    const auto escapeJsonString = [] (juce::String text)
    {
        return text.replace ("\\", "\\\\").replace ("\"", "\\\"");
    };
    const bool rendererSteamAudioCompiled = spatialRenderer.isSteamAudioCompiled();
    const bool rendererSteamAudioAvailable = spatialRenderer.isSteamAudioAvailable();
    const int rendererSteamAudioInitStageIndex = spatialRenderer.getSteamAudioInitStageIndex();
    const juce::String rendererSteamAudioInitStage {
        SpatialRenderer::steamAudioInitStageToString (rendererSteamAudioInitStageIndex)
    };
    const int rendererSteamAudioInitErrorCode = spatialRenderer.getSteamAudioInitErrorCode();
    const juce::String rendererSteamAudioRuntimeLib {
        escapeJsonString (spatialRenderer.getSteamAudioRuntimeLibraryPath())
    };
    const juce::String rendererSteamAudioMissingSymbol {
        escapeJsonString (spatialRenderer.getSteamAudioMissingSymbolName())
    };
    const int rendererSpatialProfileRequestedIndex = juce::jlimit (
        0,
        11,
        static_cast<int> (std::lround (apvts.getRawParameterValue ("rend_spatial_profile")->load())));
    const int rendererSpatialProfileActiveIndex = spatialRenderer.getSpatialOutputProfileActiveIndex();
    const int rendererSpatialProfileStageIndex = spatialRenderer.getSpatialProfileStageIndex();
    const juce::String rendererSpatialProfileRequested {
        SpatialRenderer::spatialOutputProfileToString (rendererSpatialProfileRequestedIndex)
    };
    const juce::String rendererSpatialProfileActive {
        SpatialRenderer::spatialOutputProfileToString (rendererSpatialProfileActiveIndex)
    };
    const juce::String rendererSpatialProfileStage {
        SpatialRenderer::spatialProfileStageToString (rendererSpatialProfileStageIndex)
    };
    const bool rendererAmbiCompiled = (rendererSpatialProfileRequestedIndex
                                       == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicFOA))
                                      || (rendererSpatialProfileRequestedIndex
                                          == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA));
    const bool rendererAmbiActive = (rendererSpatialProfileActiveIndex
                                     == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicFOA))
                                    || (rendererSpatialProfileActiveIndex
                                        == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA));
    const int rendererAmbiMaxOrder = rendererSpatialProfileActiveIndex
                                     == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA)
                                         ? 3
                                         : 1;
    const juce::String rendererAmbiNormalization { "sn3d" };
    const juce::String rendererAmbiChannelOrder { "acn" };
    const juce::String rendererAmbiDecodeLayout { rendererSpatialProfileActive };
    const juce::String rendererAmbiStage { rendererSpatialProfileStage };
    const auto rendererAmbiIrContract = spatialRenderer.getAmbisonicIrContractSnapshot();
    const juce::String rendererAmbiIrNormalization {
        SpatialRenderer::ambisonicNormalizationToString (rendererAmbiIrContract.normalizationIndex)
    };
    const juce::String rendererAmbiIrRequestedProfile {
        SpatialRenderer::spatialOutputProfileToString (rendererAmbiIrContract.requestedSpatialProfileIndex)
    };
    const juce::String rendererAmbiIrActiveProfile {
        SpatialRenderer::spatialOutputProfileToString (rendererAmbiIrContract.activeSpatialProfileIndex)
    };
    const juce::String rendererAmbiIrStage {
        SpatialRenderer::spatialProfileStageToString (rendererAmbiIrContract.activeSpatialStageIndex)
    };
    const juce::String rendererAmbiIrRequestedHeadphoneMode {
        SpatialRenderer::headphoneRenderModeToString (rendererAmbiIrContract.requestedHeadphoneModeIndex)
    };
    const juce::String rendererAmbiIrActiveHeadphoneMode {
        SpatialRenderer::headphoneRenderModeToString (rendererAmbiIrContract.activeHeadphoneModeIndex)
    };
    const bool rendererCompatProfileMatch =
        rendererAmbiIrContract.requestedSpatialProfileIndex == rendererAmbiIrContract.activeSpatialProfileIndex;
    const bool rendererCompatHeadphoneModeMatch =
        rendererAmbiIrContract.requestedHeadphoneModeIndex == rendererAmbiIrContract.activeHeadphoneModeIndex;
    const bool rendererCompatAmbisonicRequested =
        rendererAmbiIrContract.requestedSpatialProfileIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicFOA)
        || rendererAmbiIrContract.requestedSpatialProfileIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AmbisonicHOA);
    const bool rendererCompatAmbisonicOrderValid =
        ! rendererCompatAmbisonicRequested || rendererAmbiIrContract.order > 0;
    const bool rendererCompatSteamRequested =
        rendererAmbiIrContract.requestedHeadphoneModeIndex
            == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural);
    const bool rendererCompatSteamFallback =
        rendererCompatSteamRequested
        && ! rendererCompatHeadphoneModeMatch
        && ! rendererAmbiIrContract.steamAudioAvailable;
    juce::String rendererCompatGuardStatus { "pass" };
    juce::String rendererCompatGuardBlocker { "none" };
    juce::String rendererCompatGuardReason { "none" };
    if (! rendererCompatAmbisonicOrderValid)
    {
        rendererCompatGuardStatus = "fail";
        rendererCompatGuardBlocker = "BL063-B1";
        rendererCompatGuardReason = "ambisonic_order_invalid";
    }
    else if (rendererAmbiIrContract.fallbackActive)
    {
        rendererCompatGuardStatus = "warn";
        rendererCompatGuardBlocker = "BL063-B2";
        rendererCompatGuardReason = "profile_fallback_active";
    }
    else if (rendererCompatSteamFallback)
    {
        rendererCompatGuardStatus = "warn";
        rendererCompatGuardBlocker = "BL063-B3";
        rendererCompatGuardReason = "steam_fallback_active";
    }
    else if (! rendererCompatProfileMatch)
    {
        rendererCompatGuardStatus = "warn";
        rendererCompatGuardBlocker = "BL063-B4";
        rendererCompatGuardReason = "profile_mismatch";
    }
    const auto rendererCodecExecution = spatialRenderer.getCodecMappingExecutionSnapshot();
    const auto rendererAdmPayload = spatialRenderer.getCodecAdmRuntimePayloadSnapshot();
    const auto rendererIamfPayload = spatialRenderer.getCodecIamfRuntimePayloadSnapshot();
    const juce::String rendererCodecExecutionMode {
        SpatialRenderer::codecMappingModeToString (rendererCodecExecution.modeIndex)
    };
    constexpr int rendererCodecContractRequiredFields = 8;
    int rendererCodecContractCoveredFields = 0;
    if (rendererAmbiIrContract.frameId > 0)
        ++rendererCodecContractCoveredFields;
    ++rendererCodecContractCoveredFields; // timestampSamples is always defined in snapshot
    if (rendererAmbiIrContract.order >= 0)
        ++rendererCodecContractCoveredFields;
    if (rendererAmbiIrContract.channelCount >= 0)
        ++rendererCodecContractCoveredFields;
    if (rendererAmbiIrContract.normalizationIndex >= 0)
        ++rendererCodecContractCoveredFields;
    if (rendererCodecExecution.signature > 0)
        ++rendererCodecContractCoveredFields;
    if (rendererCodecExecution.mappedChannelCount >= 0)
        ++rendererCodecContractCoveredFields;
    if (rendererCodecExecution.frameId > 0)
        ++rendererCodecContractCoveredFields;
    const float rendererCodecContractCoverage =
        static_cast<float> (rendererCodecContractCoveredFields)
        / static_cast<float> (rendererCodecContractRequiredFields);
    const bool rendererAdmModeActive =
        rendererCodecExecution.modeIndex == static_cast<int> (SpatialRenderer::CodecMappingMode::ADM);
    const bool rendererIamfModeActive =
        rendererCodecExecution.modeIndex == static_cast<int> (SpatialRenderer::CodecMappingMode::IAMF);
    const bool rendererAdmMappingReady =
        ! rendererAdmModeActive
        || (rendererCodecExecution.mappingApplied
            && rendererCodecExecution.finite
            && rendererCodecExecution.objectCount > 0
            && rendererAdmPayload.active
            && rendererAdmPayload.objectCount > 0);
    const bool rendererIamfMappingReady =
        ! rendererIamfModeActive
        || (rendererCodecExecution.mappingApplied
            && rendererCodecExecution.finite
            && rendererCodecExecution.elementCount > 0
            && rendererIamfPayload.active
            && rendererIamfPayload.elementCount > 0);
    const juce::String rendererAdmMappingStatus { rendererAdmMappingReady ? "pass" : "fail" };
    const juce::String rendererIamfMappingStatus { rendererIamfMappingReady ? "pass" : "fail" };
    const juce::uint64 rendererCodecContractSignature =
        rendererCodecExecution.signature;
    const bool rendererPilotIntakeFailed = rendererCompatGuardStatus == "fail"
                                           || ! rendererAdmMappingReady
                                           || ! rendererIamfMappingReady;
    const bool rendererPilotIntakeConditional = ! rendererPilotIntakeFailed
                                                && rendererCompatGuardStatus == "warn";
    const juce::String rendererPilotIntakeStatus {
        rendererPilotIntakeFailed ? "blocked" : (rendererPilotIntakeConditional ? "defer" : "ready")
    };
    const juce::String rendererPilotIntakeBlocker {
        rendererPilotIntakeFailed
            ? (rendererCompatGuardStatus == "fail" ? rendererCompatGuardBlocker : "BL066-B1")
            : "none"
    };
    const juce::String rendererPilotIntakeReason {
        rendererPilotIntakeFailed
            ? (rendererCompatGuardStatus == "fail" ? rendererCompatGuardReason : "mapping_contract_incomplete")
            : (rendererPilotIntakeConditional ? rendererCompatGuardReason : "none")
    };
    const juce::String rendererPilotIntakeExecutionMode {
        rendererCodecExecutionMode
    };
    juce::String rendererAdmPayloadObjectsJson { "[" };
    for (int i = 0; i < rendererAdmPayload.objectCount && i < SpatialRenderer::NUM_SPEAKERS; ++i)
    {
        if (i > 0)
            rendererAdmPayloadObjectsJson << ",";
        rendererAdmPayloadObjectsJson
            << "{\"id\":\"adm_obj_" << juce::String (i + 1)
            << "\",\"gain\":" << juce::String (rendererAdmPayload.objectGain[static_cast<size_t> (i)], 5)
            << ",\"azimuthDeg\":" << juce::String (rendererAdmPayload.objectAzimuthDeg[static_cast<size_t> (i)], 3)
            << "}";
    }
    rendererAdmPayloadObjectsJson << "]";
    juce::String rendererIamfPayloadElementsJson { "[" };
    for (int i = 0; i < rendererIamfPayload.elementCount && i < 2; ++i)
    {
        if (i > 0)
            rendererIamfPayloadElementsJson << ",";
        rendererIamfPayloadElementsJson
            << "{\"id\":\"iamf_elem_" << juce::String (i + 1)
            << "\",\"gain\":" << juce::String (rendererIamfPayload.elementGain[static_cast<size_t> (i)], 5)
            << "}";
    }
    rendererIamfPayloadElementsJson << "]";
    const auto clapDiagnostics = getClapRuntimeDiagnostics();
    const juce::String clapWrapperType {
        escapeJsonString (clapDiagnostics.wrapperType)
    };
    const juce::String clapLifecycleStage {
        escapeJsonString (clapDiagnostics.lifecycleStage)
    };
    const juce::String clapRuntimeMode {
        escapeJsonString (clapDiagnostics.runtimeMode)
    };
    auto rendererHeadphoneModeActiveIndex = spatialRenderer.getHeadphoneRenderModeActiveIndex();
    if (outputChannels >= 2)
    {
        rendererHeadphoneModeActiveIndex =
            (rendererHeadphoneModeRequestedIndex == static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
             && rendererSteamAudioAvailable)
                ? static_cast<int> (SpatialRenderer::HeadphoneRenderMode::SteamBinaural)
                : static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    else
    {
        rendererHeadphoneModeActiveIndex = static_cast<int> (SpatialRenderer::HeadphoneRenderMode::StereoDownmix);
    }
    const juce::String rendererHeadphoneModeRequested {
        SpatialRenderer::headphoneRenderModeToString (rendererHeadphoneModeRequestedIndex)
    };
    const juce::String rendererHeadphoneModeActive {
        SpatialRenderer::headphoneRenderModeToString (rendererHeadphoneModeActiveIndex)
    };
    const auto rendererHeadphoneProfileActiveIndex = spatialRenderer.getHeadphoneDeviceProfileActiveIndex();
    const juce::String rendererHeadphoneProfileRequested {
        SpatialRenderer::headphoneDeviceProfileToString (rendererHeadphoneProfileRequestedIndex)
    };
    const juce::String rendererHeadphoneProfileActive {
        SpatialRenderer::headphoneDeviceProfileToString (rendererHeadphoneProfileActiveIndex)
    };
    const bool rendererHeadphoneCalibrationEnabledRequested =
        spatialRenderer.isHeadphoneCalibrationEnabledRequested();
    const int rendererHeadphoneCalibrationEngineRequestedIndex =
        spatialRenderer.getHeadphoneCalibrationEngineRequestedIndex();
    const int rendererHeadphoneCalibrationEngineActiveIndex =
        spatialRenderer.getHeadphoneCalibrationEngineActiveIndex();
    const int rendererHeadphoneCalibrationFallbackReasonIndex =
        spatialRenderer.getHeadphoneCalibrationFallbackReasonIndex();
    const int rendererHeadphoneCalibrationLatencySamples =
        spatialRenderer.getHeadphoneCalibrationLatencySamples();
    const juce::String rendererHeadphoneCalibrationEngineRequested {
        SpatialRenderer::headphoneCalibrationEngineToString (rendererHeadphoneCalibrationEngineRequestedIndex)
    };
    const juce::String rendererHeadphoneCalibrationEngineActive {
        SpatialRenderer::headphoneCalibrationEngineToString (rendererHeadphoneCalibrationEngineActiveIndex)
    };
    const juce::String rendererHeadphoneCalibrationFallbackReasonCode {
        SpatialRenderer::headphoneCalibrationFallbackReasonToString (rendererHeadphoneCalibrationFallbackReasonIndex)
    };
    const auto* rendererHeadTrackingPose = headTrackingBridge.currentPose();
    const bool rendererHeadPoseAvailable = rendererHeadTrackingPose != nullptr;
    const auto rendererHeadTrackingConsumers = headTrackingBridge.getConsumerCount();
    const auto rendererHeadTrackingSnapshot = buildRendererHeadTrackingSnapshot (
        rendererHeadTrackingPose,
        headTrackingBridge.getInvalidPacketCount(),
        static_cast<std::uint64_t> (juce::Time::currentTimeMillis()));
    const auto rendererMatrix = buildRendererMatrixSnapshot (
        rendererSpatialProfileRequestedIndex,
        rendererSpatialProfileActiveIndex,
        rendererSpatialProfileStageIndex,
        rendererHeadphoneModeRequestedIndex,
        rendererHeadphoneModeActiveIndex,
        outputChannels,
        rendererHeadPoseAvailable);
    const auto rendererMatrixEventSeq = static_cast<juce::uint64> (snapshotSeq);
    const bool rendererPhysicsLensEnabled = apvts.getRawParameterValue ("rend_viz_physics_lens")->load() > 0.5f;
    const float rendererPhysicsLensMix = juce::jlimit (
        0.0f,
        1.0f,
        apvts.getRawParameterValue ("rend_viz_diag_mix")->load());

    if (outputChannels >= 13
        && (rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::Surround742)
            || rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::AtmosBed)))
    {
        outputChannelLabelsJson = "[\"L\",\"R\",\"C\",\"LFE1\",\"LFE2\",\"Ls\",\"Rs\",\"Lrs\",\"Rrs\",\"TopFL\",\"TopFR\",\"TopRL\",\"TopRR\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= 10
             && rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::Surround721))
    {
        outputChannelLabelsJson = "[\"L\",\"R\",\"C\",\"LFE1\",\"LFE2\",\"Ls\",\"Rs\",\"Lrs\",\"Rrs\",\"TopC\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= 8
             && rendererSpatialProfileActiveIndex == static_cast<int> (SpatialRenderer::SpatialOutputProfile::Surround521))
    {
        outputChannelLabelsJson = "[\"L\",\"R\",\"C\",\"LFE1\",\"LFE2\",\"Ls\",\"Rs\",\"TopC\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= SpatialRenderer::NUM_SPEAKERS
             && rendererAmbiActive)
    {
        outputChannelLabelsJson = "[\"W\",\"X\",\"Y\",\"Z\"]";
        rendererOutputMode = rendererSpatialProfileActive;
    }
    else if (outputChannels >= SpatialRenderer::NUM_SPEAKERS)
    {
        outputChannelLabelsJson = "[\"FL\",\"FR\",\"RL\",\"RR\"]";
        rendererOutputMode = "quad_map_first4";
    }
    else if (outputChannels >= 2)
    {
        outputChannelLabelsJson = "[\"L\",\"R\"]";
        rendererOutputMode = rendererSpatialProfileActive;
        if (rendererOutputMode.isEmpty() || rendererOutputMode == "auto")
            rendererOutputMode = rendererHeadphoneModeActive;
    }

    const auto currentCalSpeakerConfig = getCurrentCalibrationSpeakerConfigIndex();
    const auto currentCalSpeakerRouting = getCurrentCalibrationSpeakerRouting();
    const auto currentCalTopologyProfile = getCurrentCalibrationTopologyProfileIndex();
    const auto currentCalMonitoringPath = getCurrentCalibrationMonitoringPathIndex();
    const auto currentCalDeviceProfile = getCurrentCalibrationDeviceProfileIndex();
    const auto currentCalTopologyId = calibrationTopologyIdForIndex (currentCalTopologyProfile);
    const auto currentCalMonitoringPathId = calibrationMonitoringPathIdForIndex (currentCalMonitoringPath);
    const auto currentCalDeviceProfileId = calibrationDeviceProfileIdForIndex (currentCalDeviceProfile);
    const auto currentCalRequiredChannels = getRequiredCalibrationChannelsForTopologyIndex (currentCalTopologyProfile);
    const auto currentCalWritableChannels = resolveCalibrationWritableChannels (
        outputChannels > 0 ? outputChannels : getSnapshotOutputChannels(),
        static_cast<int> (getBusesLayout().getMainOutputChannelSet().size()),
        lastAutoDetectedOutputChannels,
        currentCalSpeakerRouting);
    const bool currentCalMappingLimitedToFirst4 = currentCalRequiredChannels > currentCalWritableChannels;
    const auto toRoutingJson = [] (const std::array<int, SpatialRenderer::NUM_SPEAKERS>& routing)
    {
        juce::String jsonArray { "[" };
        for (size_t i = 0; i < routing.size(); ++i)
        {
            if (i > 0)
                jsonArray << ",";
            jsonArray << juce::String (juce::jlimit (1, 8, routing[i]));
        }
        jsonArray << "]";
        return jsonArray;
    };

    const auto currentCalSpeakerRoutingJson = toRoutingJson (currentCalSpeakerRouting);
    const auto autoDetectedRoutingJson = toRoutingJson (lastAutoDetectedSpeakerRouting);
    const auto autoDetectedTopologyId = calibrationTopologyIdForIndex (lastAutoDetectedTopologyProfile);
    const auto rendererHeadphoneCalibration = buildHeadphoneCalibrationDiagnosticsSnapshot (
        currentCalMonitoringPath,
        rendererHeadphoneModeRequestedIndex,
        rendererHeadphoneModeActiveIndex,
        outputChannels,
        rendererSteamAudioAvailable,
        rendererSteamAudioInitStage);
    const auto rendererHeadphoneVerification = buildHeadphoneVerificationSnapshot (
        rendererHeadphoneProfileRequestedIndex,
        rendererHeadphoneProfileActiveIndex,
        rendererHeadphoneCalibrationEnabledRequested,
        rendererHeadphoneCalibrationEngineRequestedIndex,
        rendererHeadphoneCalibrationEngineActiveIndex,
        rendererHeadphoneCalibrationFallbackReasonIndex,
        rendererHeadphoneCalibrationLatencySamples);

    {
        const juce::SpinLock::ScopedLockType publishedCalibrationLock (publishedHeadphoneCalibrationLock);
        publishedHeadphoneCalibrationDiagnostics.profileSyncSeq = snapshotSeq;
        publishedHeadphoneCalibrationDiagnostics.requested = rendererHeadphoneCalibration.requested;
        publishedHeadphoneCalibrationDiagnostics.active = rendererHeadphoneCalibration.active;
        publishedHeadphoneCalibrationDiagnostics.stage = rendererHeadphoneCalibration.stage;
        publishedHeadphoneCalibrationDiagnostics.fallbackReady = rendererHeadphoneCalibration.fallbackReady;
        publishedHeadphoneCalibrationDiagnostics.fallbackReason = rendererHeadphoneCalibration.fallbackReason;
        publishedHeadphoneCalibrationDiagnostics.valid = true;

        publishedHeadphoneVerificationDiagnostics.profileSyncSeq = snapshotSeq;
        publishedHeadphoneVerificationDiagnostics.profileId = rendererHeadphoneVerification.profileId;
        publishedHeadphoneVerificationDiagnostics.requestedProfileId =
            rendererHeadphoneVerification.requestedProfileId;
        publishedHeadphoneVerificationDiagnostics.activeProfileId =
            rendererHeadphoneVerification.activeProfileId;
        publishedHeadphoneVerificationDiagnostics.requestedEngineId =
            rendererHeadphoneVerification.requestedEngineId;
        publishedHeadphoneVerificationDiagnostics.activeEngineId =
            rendererHeadphoneVerification.activeEngineId;
        publishedHeadphoneVerificationDiagnostics.fallbackReasonCode =
            rendererHeadphoneVerification.fallbackReasonCode;
        publishedHeadphoneVerificationDiagnostics.fallbackTarget =
            rendererHeadphoneVerification.fallbackTarget;
        publishedHeadphoneVerificationDiagnostics.fallbackReasonText =
            rendererHeadphoneVerification.fallbackReasonText;
        publishedHeadphoneVerificationDiagnostics.frontBackScore =
            locusq::shared_contracts::headphone_verification::sanitizeScore (
                rendererHeadphoneVerification.frontBackScore,
                0.0f);
        publishedHeadphoneVerificationDiagnostics.elevationScore =
            locusq::shared_contracts::headphone_verification::sanitizeScore (
                rendererHeadphoneVerification.elevationScore,
                0.0f);
        publishedHeadphoneVerificationDiagnostics.externalizationScore =
            locusq::shared_contracts::headphone_verification::sanitizeScore (
                rendererHeadphoneVerification.externalizationScore,
                0.0f);
        publishedHeadphoneVerificationDiagnostics.confidence =
            locusq::shared_contracts::headphone_verification::sanitizeScore (
                rendererHeadphoneVerification.confidence,
                0.0f);
        publishedHeadphoneVerificationDiagnostics.verificationStage =
            rendererHeadphoneVerification.verificationStage;
        publishedHeadphoneVerificationDiagnostics.verificationScoreStatus =
            rendererHeadphoneVerification.verificationScoreStatus;
        publishedHeadphoneVerificationDiagnostics.chainLatencySamples =
            locusq::shared_contracts::headphone_verification::sanitizeLatencySamples (
                rendererHeadphoneVerification.chainLatencySamples);
        publishedHeadphoneVerificationDiagnostics.valid = true;
    }

    Vec3 listenerPosition { 0.0f, 1.2f, 0.0f };
    Vec3 roomDimensions { 6.0f, 4.0f, 3.0f };
    std::array<Vec3, SpatialRenderer::NUM_SPEAKERS> speakerPositions = kViewportFallbackSpeakerPositions;
    std::array<float, SpatialRenderer::NUM_SPEAKERS> speakerGainTrims {};
    std::array<float, SpatialRenderer::NUM_SPEAKERS> speakerDelayCompMs {};
    bool roomProfileValid = false;

    if (auto roomProfile = sceneGraph.getRoomProfile(); roomProfile != nullptr && roomProfile->valid)
    {
        roomProfileValid = true;
        listenerPosition = roomProfile->listenerPos;
        roomDimensions = roomProfile->dimensions;

        for (size_t i = 0; i < speakerPositions.size(); ++i)
        {
            speakerPositions[i] = roomProfile->speakers[i].position;
            speakerGainTrims[i] = roomProfile->speakers[i].gainTrim;
            speakerDelayCompMs[i] = roomProfile->speakers[i].delayComp;
        }
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

        const auto* emitterAudio = sceneGraph.getSlot (i).getAudioMono();
        const auto emitterAudioSamples = sceneGraph.getSlot (i).getAudioNumSamples();
        const auto emitterRmsLinear = computeMonoRmsLinear (emitterAudio, emitterAudioSamples);
        const auto emitterRmsDb = juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, emitterRmsLinear), -120.0f);

        json += "{\"id\":" + juce::String (i)
              + ",\"x\":" + juce::String (data.position.x, 3)
              + ",\"y\":" + juce::String (data.position.y, 3)
              + ",\"z\":" + juce::String (data.position.z, 3)
              + ",\"sx\":" + juce::String (data.size.x, 2)
              + ",\"sy\":" + juce::String (data.size.y, 2)
              + ",\"sz\":" + juce::String (data.size.z, 2)
              + ",\"gain\":" + juce::String (data.gain, 1)
              + ",\"spread\":" + juce::String (data.spread, 2)
              + ",\"directivity\":" + juce::String (data.directivity, 2)
              + ",\"aimX\":" + juce::String (data.directivityAim.x, 3)
              + ",\"aimY\":" + juce::String (data.directivityAim.y, 3)
              + ",\"aimZ\":" + juce::String (data.directivityAim.z, 3)
              + ",\"color\":" + juce::String (data.colorIndex)
              + ",\"muted\":" + juce::String (data.muted ? "true" : "false")
              + ",\"soloed\":" + juce::String (data.soloed ? "true" : "false")
              + ",\"physics\":" + juce::String (data.physicsEnabled ? "true" : "false")
              + ",\"vx\":" + juce::String (data.velocity.x, 3)
              + ",\"vy\":" + juce::String (data.velocity.y, 3)
              + ",\"vz\":" + juce::String (data.velocity.z, 3)
              + ",\"fx\":" + juce::String (data.force.x, 3)
              + ",\"fy\":" + juce::String (data.force.y, 3)
              + ",\"fz\":" + juce::String (data.force.z, 3)
              + ",\"collisionMask\":" + juce::String (static_cast<int> (data.collisionMask))
              + ",\"collisionEnergy\":" + juce::String (data.collisionEnergy, 4)
              + ",\"rms\":" + juce::String (emitterRmsLinear, 5)
              + ",\"rmsDb\":" + juce::String (emitterRmsDb, 2)
              + ",\"label\":\"" + juce::String (data.label) + "\""
              + "}";
    }

    juce::String speakerRmsJson { "[" };
    juce::String speakersJson { "[" };
    for (size_t i = 0; i < sceneSpeakerRms.size(); ++i)
    {
        if (i > 0)
        {
            speakerRmsJson << ",";
            speakersJson << ",";
        }

        const auto speakerRms = juce::jlimit (0.0f, 4.0f, sceneSpeakerRms[i]);
        speakerRmsJson << juce::String (speakerRms, 5);

        speakersJson << "{\"id\":" << juce::String (static_cast<int> (i))
                     << ",\"label\":\"" << juce::String (kInternalSpeakerLabels[i]) << "\""
                     << ",\"x\":" << juce::String (speakerPositions[i].x, 3)
                     << ",\"y\":" << juce::String (speakerPositions[i].y, 3)
                     << ",\"z\":" << juce::String (speakerPositions[i].z, 3)
                     << ",\"gainTrimDb\":" << juce::String (speakerGainTrims[i], 3)
                     << ",\"delayCompMs\":" << juce::String (speakerDelayCompMs[i], 3)
                     << ",\"rms\":" << juce::String (speakerRms, 5)
                     << "}";
    }
    speakerRmsJson << "]";
    speakersJson << "]";

    const auto registrationTransitionSeq = registrationTransitionDiagnostics.seq.load (std::memory_order_acquire);
    const auto registrationRequestedModeCode = juce::jlimit (
        0,
        2,
        registrationTransitionDiagnostics.requestedMode.load (std::memory_order_relaxed));
    const auto registrationRequestedMode = static_cast<LocusQMode> (registrationRequestedModeCode);
    const auto registrationStage = registrationTransitionStageFromCode (
        registrationTransitionDiagnostics.stageCode.load (std::memory_order_relaxed));
    const auto registrationFallback = registrationTransitionFallbackReasonFromCode (
        registrationTransitionDiagnostics.fallbackCode.load (std::memory_order_relaxed));
    const auto registrationEmitterSlot = registrationTransitionDiagnostics.emitterSlot.load (std::memory_order_relaxed);
    const auto registrationEmitterActive = registrationTransitionDiagnostics.emitterActive.load (std::memory_order_relaxed);
    const auto registrationRendererOwned = registrationTransitionDiagnostics.rendererOwned.load (std::memory_order_relaxed);
    const auto registrationAmbiguityCount = registrationTransitionDiagnostics.ambiguityCount.load (std::memory_order_relaxed);
    const auto registrationStaleOwnerCount = registrationTransitionDiagnostics.staleOwnerCount.load (std::memory_order_relaxed);

    json += "],\"emitterCount\":" + juce::String (sceneGraph.getActiveEmitterCount())
          + ",\"localEmitterId\":" + juce::String (emitterSlotId)
          + ",\"registrationTransitionSeq\":"
              + juce::String (static_cast<juce::int64> (registrationTransitionSeq))
          + ",\"registrationRequestedMode\":\""
              + escapeJsonString (locusQModeToString (registrationRequestedMode)) + "\""
          + ",\"registrationStage\":\""
              + escapeJsonString (registrationTransitionStageToString (registrationStage)) + "\""
          + ",\"registrationFallbackReason\":\""
              + escapeJsonString (registrationTransitionFallbackReasonToString (registrationFallback)) + "\""
          + ",\"registrationEmitterSlot\":" + juce::String (registrationEmitterSlot)
          + ",\"registrationEmitterActive\":" + juce::String (registrationEmitterActive ? "true" : "false")
          + ",\"registrationRendererOwned\":" + juce::String (registrationRendererOwned ? "true" : "false")
          + ",\"registrationAmbiguityCount\":" + juce::String (registrationAmbiguityCount)
          + ",\"registrationStaleOwnerCount\":" + juce::String (registrationStaleOwnerCount)
          + ",\"registrationTransition\":{\"requestedMode\":\""
              + escapeJsonString (locusQModeToString (registrationRequestedMode)) + "\""
              + ",\"stage\":\"" + escapeJsonString (registrationTransitionStageToString (registrationStage)) + "\""
              + ",\"fallbackReason\":\""
              + escapeJsonString (registrationTransitionFallbackReasonToString (registrationFallback)) + "\""
              + ",\"emitterSlot\":" + juce::String (registrationEmitterSlot)
              + ",\"emitterActive\":" + juce::String (registrationEmitterActive ? "true" : "false")
              + ",\"rendererOwned\":" + juce::String (registrationRendererOwned ? "true" : "false")
              + ",\"ambiguityCount\":" + juce::String (registrationAmbiguityCount)
              + ",\"staleOwnerCount\":" + juce::String (registrationStaleOwnerCount)
              + ",\"seq\":" + juce::String (static_cast<juce::int64> (registrationTransitionSeq))
              + "}"
          + ",\"rendererActive\":" + juce::String (sceneGraph.isRendererRegistered() ? "true" : "false")
          + ",\"rendererEligibleEmitters\":" + juce::String (spatialRenderer.getLastEligibleEmitterCount())
          + ",\"rendererProcessedEmitters\":" + juce::String (spatialRenderer.getLastProcessedEmitterCount())
          + ",\"rendererCulledBudget\":" + juce::String (spatialRenderer.getLastBudgetCulledEmitterCount())
          + ",\"rendererCulledActivity\":" + juce::String (spatialRenderer.getLastActivityCulledEmitterCount())
          + ",\"rendererGuardrailActive\":" + juce::String (spatialRenderer.wasGuardrailActiveLastBlock() ? "true" : "false")
          + ",\"outputChannels\":" + juce::String (outputChannels)
          + ",\"outputLayout\":\"" + outputLayout + "\""
          + ",\"rendererOutputMode\":\"" + rendererOutputMode + "\""
          + ",\"rendererSpatialProfileRequested\":\"" + rendererSpatialProfileRequested + "\""
          + ",\"rendererSpatialProfileActive\":\"" + rendererSpatialProfileActive + "\""
          + ",\"rendererSpatialProfileStage\":\"" + rendererSpatialProfileStage + "\""
          + ",\"rendererMatrixRequestedDomain\":\"" + escapeJsonString (rendererMatrix.requestedDomain) + "\""
          + ",\"rendererMatrixActiveDomain\":\"" + escapeJsonString (rendererMatrix.activeDomain) + "\""
          + ",\"rendererMatrixRequestedLayout\":\"" + escapeJsonString (rendererMatrix.requestedLayout) + "\""
          + ",\"rendererMatrixActiveLayout\":\"" + escapeJsonString (rendererMatrix.activeLayout) + "\""
          + ",\"rendererMatrixRuleId\":\"" + escapeJsonString (rendererMatrix.ruleId) + "\""
          + ",\"rendererMatrixRuleState\":\"" + escapeJsonString (rendererMatrix.ruleState) + "\""
          + ",\"rendererMatrixReasonCode\":\"" + escapeJsonString (rendererMatrix.reasonCode) + "\""
          + ",\"rendererMatrixFallbackMode\":\"" + escapeJsonString (rendererMatrix.fallbackMode) + "\""
          + ",\"rendererMatrixFailSafeRoute\":\"" + escapeJsonString (rendererMatrix.failSafeRoute) + "\""
          + ",\"rendererMatrixStatusText\":\"" + escapeJsonString (rendererMatrix.statusText) + "\""
          + ",\"rendererMatrixEventSeq\":" + juce::String (static_cast<juce::int64> (rendererMatrixEventSeq))
          + ",\"rendererMatrix\":{\"requestedDomain\":\"" + escapeJsonString (rendererMatrix.requestedDomain) + "\""
              + ",\"activeDomain\":\"" + escapeJsonString (rendererMatrix.activeDomain) + "\""
              + ",\"requestedLayout\":\"" + escapeJsonString (rendererMatrix.requestedLayout) + "\""
              + ",\"activeLayout\":\"" + escapeJsonString (rendererMatrix.activeLayout) + "\""
              + ",\"ruleId\":\"" + escapeJsonString (rendererMatrix.ruleId) + "\""
              + ",\"ruleState\":\"" + escapeJsonString (rendererMatrix.ruleState) + "\""
              + ",\"fallbackMode\":\"" + escapeJsonString (rendererMatrix.fallbackMode) + "\""
              + ",\"reasonCode\":\"" + escapeJsonString (rendererMatrix.reasonCode) + "\""
              + ",\"statusText\":\"" + escapeJsonString (rendererMatrix.statusText) + "\"}"
          + ",\"rendererHeadphoneModeRequested\":\"" + rendererHeadphoneModeRequested + "\""
          + ",\"rendererHeadphoneModeActive\":\"" + rendererHeadphoneModeActive + "\""
          + ",\"rendererHeadphoneProfileRequested\":\"" + rendererHeadphoneProfileRequested + "\""
          + ",\"rendererHeadphoneProfileActive\":\"" + rendererHeadphoneProfileActive + "\""
          + ",\"rendererHeadTrackingEnabled\":"
              + juce::String (rendererHeadTrackingSnapshot.bridgeEnabled ? "true" : "false")
          + ",\"rendererHeadTrackingSource\":\""
              + escapeJsonString (rendererHeadTrackingSnapshot.source) + "\""
          + ",\"rendererHeadTrackingPoseAvailable\":"
              + juce::String (rendererHeadTrackingSnapshot.poseAvailable ? "true" : "false")
          + ",\"rendererHeadTrackingPoseStale\":"
              + juce::String (rendererHeadTrackingSnapshot.poseStale ? "true" : "false")
          + ",\"rendererHeadTrackingOrientationValid\":"
              + juce::String (rendererHeadTrackingSnapshot.orientationValid ? "true" : "false")
          + ",\"rendererHeadTrackingInvalidPackets\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingSnapshot.invalidPacketCount))
          + ",\"rendererHeadTrackingConsumers\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingConsumers))
          + ",\"rendererHeadTrackingSeq\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingSnapshot.seq))
          + ",\"rendererHeadTrackingTimestampMs\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingSnapshot.timestampMs))
          + ",\"rendererHeadTrackingAgeMs\":"
              + juce::String (rendererHeadTrackingSnapshot.ageMs, 3)
          + ",\"rendererHeadTrackingQx\":" + juce::String (rendererHeadTrackingSnapshot.qx, 6)
          + ",\"rendererHeadTrackingQy\":" + juce::String (rendererHeadTrackingSnapshot.qy, 6)
          + ",\"rendererHeadTrackingQz\":" + juce::String (rendererHeadTrackingSnapshot.qz, 6)
          + ",\"rendererHeadTrackingQw\":" + juce::String (rendererHeadTrackingSnapshot.qw, 6)
          + ",\"rendererHeadTrackingYawDeg\":" + juce::String (rendererHeadTrackingSnapshot.yawDeg, 3)
          + ",\"rendererHeadTrackingPitchDeg\":" + juce::String (rendererHeadTrackingSnapshot.pitchDeg, 3)
          + ",\"rendererHeadTrackingRollDeg\":" + juce::String (rendererHeadTrackingSnapshot.rollDeg, 3)
          + ",\"rendererHeadTracking\":{\"enabled\":"
              + juce::String (rendererHeadTrackingSnapshot.bridgeEnabled ? "true" : "false")
              + ",\"source\":\"" + escapeJsonString (rendererHeadTrackingSnapshot.source) + "\""
              + ",\"poseAvailable\":"
              + juce::String (rendererHeadTrackingSnapshot.poseAvailable ? "true" : "false")
              + ",\"poseStale\":"
              + juce::String (rendererHeadTrackingSnapshot.poseStale ? "true" : "false")
              + ",\"orientationValid\":"
              + juce::String (rendererHeadTrackingSnapshot.orientationValid ? "true" : "false")
              + ",\"invalidPackets\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingSnapshot.invalidPacketCount))
              + ",\"consumers\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingConsumers))
              + ",\"seq\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingSnapshot.seq))
              + ",\"timestampMs\":"
              + juce::String (static_cast<juce::uint64> (rendererHeadTrackingSnapshot.timestampMs))
              + ",\"ageMs\":" + juce::String (rendererHeadTrackingSnapshot.ageMs, 3)
              + ",\"qx\":" + juce::String (rendererHeadTrackingSnapshot.qx, 6)
              + ",\"qy\":" + juce::String (rendererHeadTrackingSnapshot.qy, 6)
              + ",\"qz\":" + juce::String (rendererHeadTrackingSnapshot.qz, 6)
              + ",\"qw\":" + juce::String (rendererHeadTrackingSnapshot.qw, 6)
              + ",\"yawDeg\":" + juce::String (rendererHeadTrackingSnapshot.yawDeg, 3)
              + ",\"pitchDeg\":" + juce::String (rendererHeadTrackingSnapshot.pitchDeg, 3)
              + ",\"rollDeg\":" + juce::String (rendererHeadTrackingSnapshot.rollDeg, 3) + "}"
          + ",\"rendererHeadphoneCalibrationSchema\":\""
              + escapeJsonString (locusq::shared_contracts::headphone_calibration::kSchemaV1) + "\""
          + ",\"rendererHeadphoneCalibrationRequested\":\""
              + escapeJsonString (rendererHeadphoneCalibration.requested) + "\""
          + ",\"rendererHeadphoneCalibrationActive\":\""
              + escapeJsonString (rendererHeadphoneCalibration.active) + "\""
          + ",\"rendererHeadphoneCalibrationStage\":\""
              + escapeJsonString (rendererHeadphoneCalibration.stage) + "\""
          + ",\"rendererHeadphoneCalibrationFallbackReady\":"
              + juce::String (rendererHeadphoneCalibration.fallbackReady ? "true" : "false")
          + ",\"rendererHeadphoneCalibrationFallbackReason\":\""
              + escapeJsonString (rendererHeadphoneCalibration.fallbackReason) + "\""
          + ",\"rendererHeadphoneCalibration\":{\"schema\":\""
              + escapeJsonString (locusq::shared_contracts::headphone_calibration::kSchemaV1) + "\""
              + ",\"requested\":\"" + escapeJsonString (rendererHeadphoneCalibration.requested) + "\""
              + ",\"active\":\"" + escapeJsonString (rendererHeadphoneCalibration.active) + "\""
              + ",\"stage\":\"" + escapeJsonString (rendererHeadphoneCalibration.stage) + "\""
              + ",\"fallbackReady\":"
              + juce::String (rendererHeadphoneCalibration.fallbackReady ? "true" : "false")
              + ",\"fallbackReason\":\"" + escapeJsonString (rendererHeadphoneCalibration.fallbackReason) + "\"}"
          + ",\"rendererHeadphoneCalibrationEnabledRequested\":"
              + juce::String (rendererHeadphoneCalibrationEnabledRequested ? "true" : "false")
          + ",\"rendererHeadphoneCalibrationEngineRequested\":\""
              + escapeJsonString (rendererHeadphoneCalibrationEngineRequested) + "\""
          + ",\"rendererHeadphoneCalibrationEngineActive\":\""
              + escapeJsonString (rendererHeadphoneCalibrationEngineActive) + "\""
          + ",\"rendererHeadphoneCalibrationFallbackReasonCode\":\""
              + escapeJsonString (rendererHeadphoneCalibrationFallbackReasonCode) + "\""
          + ",\"rendererHeadphoneCalibrationLatencySamples\":"
              + juce::String (rendererHeadphoneVerification.chainLatencySamples)
          + ",\"rendererHeadphoneVerificationSchema\":\""
              + escapeJsonString (locusq::shared_contracts::headphone_verification::kSchemaV1) + "\""
          + ",\"rendererHeadphoneVerificationProfileId\":\""
              + escapeJsonString (rendererHeadphoneVerification.profileId) + "\""
          + ",\"rendererHeadphoneVerificationRequestedProfileId\":\""
              + escapeJsonString (rendererHeadphoneVerification.requestedProfileId) + "\""
          + ",\"rendererHeadphoneVerificationActiveProfileId\":\""
              + escapeJsonString (rendererHeadphoneVerification.activeProfileId) + "\""
          + ",\"rendererHeadphoneVerificationFallbackReasonCode\":\""
              + escapeJsonString (rendererHeadphoneVerification.fallbackReasonCode) + "\""
          + ",\"rendererHeadphoneVerificationFallbackTarget\":\""
              + escapeJsonString (rendererHeadphoneVerification.fallbackTarget) + "\""
          + ",\"rendererHeadphoneVerificationFallbackReasonText\":\""
              + escapeJsonString (rendererHeadphoneVerification.fallbackReasonText) + "\""
          + ",\"rendererHeadphoneVerificationFrontBackScore\":"
              + juce::String (rendererHeadphoneVerification.frontBackScore, 5)
          + ",\"rendererHeadphoneVerificationElevationScore\":"
              + juce::String (rendererHeadphoneVerification.elevationScore, 5)
          + ",\"rendererHeadphoneVerificationExternalizationScore\":"
              + juce::String (rendererHeadphoneVerification.externalizationScore, 5)
          + ",\"rendererHeadphoneVerificationConfidence\":"
              + juce::String (rendererHeadphoneVerification.confidence, 5)
          + ",\"rendererHeadphoneVerificationStage\":\""
              + escapeJsonString (rendererHeadphoneVerification.verificationStage) + "\""
          + ",\"rendererHeadphoneVerificationScoreStatus\":\""
              + escapeJsonString (rendererHeadphoneVerification.verificationScoreStatus) + "\""
          + ",\"rendererHeadphoneVerificationLatencySamples\":"
              + juce::String (rendererHeadphoneVerification.chainLatencySamples)
          + ",\"headphoneVerification\":{\"schema\":\""
              + escapeJsonString (locusq::shared_contracts::headphone_verification::kSchemaV1) + "\""
              + ",\"profileId\":\"" + escapeJsonString (rendererHeadphoneVerification.profileId) + "\""
              + ",\"requestedProfileId\":\""
              + escapeJsonString (rendererHeadphoneVerification.requestedProfileId) + "\""
              + ",\"activeProfileId\":\""
              + escapeJsonString (rendererHeadphoneVerification.activeProfileId) + "\""
              + ",\"requestedEngineId\":\""
              + escapeJsonString (rendererHeadphoneVerification.requestedEngineId) + "\""
              + ",\"activeEngineId\":\""
              + escapeJsonString (rendererHeadphoneVerification.activeEngineId) + "\""
              + ",\"fallbackReasonCode\":\""
              + escapeJsonString (rendererHeadphoneVerification.fallbackReasonCode) + "\""
              + ",\"fallbackTarget\":\""
              + escapeJsonString (rendererHeadphoneVerification.fallbackTarget) + "\""
              + ",\"fallbackReasonText\":\""
              + escapeJsonString (rendererHeadphoneVerification.fallbackReasonText) + "\""
              + ",\"frontBackScore\":" + juce::String (rendererHeadphoneVerification.frontBackScore, 5)
              + ",\"elevationScore\":" + juce::String (rendererHeadphoneVerification.elevationScore, 5)
              + ",\"externalizationScore\":"
              + juce::String (rendererHeadphoneVerification.externalizationScore, 5)
              + ",\"confidence\":" + juce::String (rendererHeadphoneVerification.confidence, 5)
              + ",\"verificationStage\":\""
              + escapeJsonString (rendererHeadphoneVerification.verificationStage) + "\""
              + ",\"verificationScoreStatus\":\""
              + escapeJsonString (rendererHeadphoneVerification.verificationScoreStatus) + "\""
              + ",\"latencySamples\":" + juce::String (rendererHeadphoneVerification.chainLatencySamples)
              + "}"
          + ",\"rendererAuditionEnabled\":" + juce::String (rendererAuditionEnabled ? "true" : "false")
          + ",\"rendererAuditionSignal\":\"" + escapeJsonString (rendererAuditionSignal) + "\""
          + ",\"rendererAuditionMotion\":\"" + escapeJsonString (rendererAuditionMotion) + "\""
          + ",\"rendererAuditionLevelDb\":" + juce::String (rendererAuditionLevelDb, 1)
          + ",\"rendererAuditionSourceMode\":\"" + escapeJsonString (rendererAuditionSourceMode) + "\""
          + ",\"rendererAuditionRequestedMode\":\"" + escapeJsonString (rendererAuditionRequestedMode) + "\""
          + ",\"rendererAuditionResolvedMode\":\"" + escapeJsonString (rendererAuditionResolvedMode) + "\""
          + ",\"rendererAuditionBindingTarget\":\"" + escapeJsonString (rendererAuditionBindingTarget) + "\""
          + ",\"rendererAuditionBindingAvailable\":" + juce::String (rendererAuditionBindingAvailable ? "true" : "false")
          + ",\"rendererAuditionSeed\":" + juce::String (static_cast<juce::uint64> (rendererAuditionCloudSeed))
          + ",\"rendererAuditionTransportSync\":" + juce::String (rendererAuditionTransportSync ? "true" : "false")
          + ",\"rendererAuditionDensity\":" + juce::String (rendererAuditionDensity, 4)
          + ",\"rendererAuditionReactivity\":" + juce::String (rendererAuditionReactivity, 4)
          + ",\"rendererAuditionFallbackReason\":\"" + escapeJsonString (rendererAuditionFallbackReason) + "\""
          + ",\"rendererAuditionVisualActive\":" + juce::String (rendererAuditionVisualActive ? "true" : "false")
          + ",\"rendererAuditionVisual\":{\"x\":" + juce::String (rendererAuditionVisualX, 3)
              + ",\"y\":" + juce::String (rendererAuditionVisualY, 3)
              + ",\"z\":" + juce::String (rendererAuditionVisualZ, 3) + "}"
          + ",\"rendererAuditionCloud\":{\"enabled\":" + juce::String (rendererAuditionCloudEnabled ? "true" : "false")
              + ",\"pattern\":\"" + escapeJsonString (rendererAuditionCloudPattern) + "\""
              + ",\"mode\":\"" + escapeJsonString (rendererAuditionCloudMode) + "\""
              + ",\"emitterCount\":" + juce::String (rendererAuditionCloudEmitterCount)
              + ",\"pointCount\":" + juce::String (rendererAuditionCloudPointCount)
              + ",\"spreadMeters\":" + juce::String (rendererAuditionCloudSpreadMeters, 3)
              + ",\"seed\":" + juce::String (static_cast<juce::uint64> (rendererAuditionCloudSeed))
              + ",\"pulseHz\":" + juce::String (rendererAuditionCloudPulseHz, 3)
              + ",\"coherence\":" + juce::String (rendererAuditionCloudCoherence, 3)
              + ",\"emitters\":" + rendererAuditionCloudEmittersJson + "}"
          + ",\"rendererAuditionReactive\":{\"rms\":" + juce::String (rendererAuditionReactive.rms, 5)
              + ",\"peak\":" + juce::String (rendererAuditionReactive.peak, 5)
              + ",\"envFast\":" + juce::String (rendererAuditionReactive.envFast, 5)
              + ",\"envSlow\":" + juce::String (rendererAuditionReactive.envSlow, 5)
              + ",\"onset\":" + juce::String (rendererAuditionReactive.onset, 5)
              + ",\"brightness\":" + juce::String (rendererAuditionReactive.brightness, 5)
              + ",\"rainFadeRate\":" + juce::String (rendererAuditionReactive.rainFadeRate, 5)
              + ",\"snowFadeRate\":" + juce::String (rendererAuditionReactive.snowFadeRate, 5)
              + ",\"physicsVelocity\":" + juce::String (rendererAuditionReactive.physicsVelocity, 5)
              + ",\"physicsCollision\":" + juce::String (rendererAuditionReactive.physicsCollision, 5)
              + ",\"physicsDensity\":" + juce::String (rendererAuditionReactive.physicsDensity, 5)
              + ",\"physicsCoupling\":" + juce::String (rendererAuditionReactive.physicsCoupling, 5)
              + ",\"geometryScale\":" + juce::String (rendererAuditionReactive.geometryScale, 5)
              + ",\"geometryWidth\":" + juce::String (rendererAuditionReactive.geometryWidth, 5)
              + ",\"geometryDepth\":" + juce::String (rendererAuditionReactive.geometryDepth, 5)
              + ",\"geometryHeight\":" + juce::String (rendererAuditionReactive.geometryHeight, 5)
              + ",\"precipitationFade\":" + juce::String (rendererAuditionReactive.precipitationFade, 5)
              + ",\"collisionBurst\":" + juce::String (rendererAuditionReactive.collisionBurst, 5)
              + ",\"densitySpread\":" + juce::String (rendererAuditionReactive.densitySpread, 5)
              + ",\"headphoneOutputRms\":" + juce::String (rendererAuditionReactive.headphoneOutputRms, 5)
              + ",\"headphoneOutputPeak\":" + juce::String (rendererAuditionReactive.headphoneOutputPeak, 5)
              + ",\"headphoneParity\":" + juce::String (rendererAuditionReactive.headphoneParity, 5)
              + ",\"headphoneFallback\":" + juce::String (rendererAuditionReactiveHeadphoneFallback ? "true" : "false")
              + ",\"headphoneFallbackReason\":\"" + escapeJsonString (rendererAuditionReactiveHeadphoneFallbackReason) + "\""
              + ",\"sourceEnergy\":" + rendererAuditionSourceEnergyJson
              + ",\"reactiveActive\":" + juce::String (rendererAuditionReactivePublishedActive ? "true" : "false")
              + ",\"rmsNorm\":" + juce::String (rendererAuditionReactive.rmsNorm, 5)
              + ",\"peakNorm\":" + juce::String (rendererAuditionReactive.peakNorm, 5)
              + ",\"envFastNorm\":" + juce::String (rendererAuditionReactive.envFastNorm, 5)
              + ",\"envSlowNorm\":" + juce::String (rendererAuditionReactive.envSlowNorm, 5)
              + ",\"onsetNorm\":" + juce::String (rendererAuditionReactive.onset, 5)
              + ",\"brightnessNorm\":" + juce::String (rendererAuditionReactive.brightness, 5)
              + ",\"rainFadeRateNorm\":" + juce::String (rendererAuditionReactive.rainFadeRate, 5)
              + ",\"snowFadeRateNorm\":" + juce::String (rendererAuditionReactive.snowFadeRate, 5)
              + ",\"physicsVelocityNorm\":" + juce::String (rendererAuditionReactive.physicsVelocity, 5)
              + ",\"physicsCollisionNorm\":" + juce::String (rendererAuditionReactive.physicsCollision, 5)
              + ",\"physicsDensityNorm\":" + juce::String (rendererAuditionReactive.physicsDensity, 5)
              + ",\"physicsCouplingNorm\":" + juce::String (rendererAuditionReactive.physicsCoupling, 5)
              + ",\"headphoneOutputRmsNorm\":" + juce::String (rendererAuditionReactive.headphoneOutputRmsNorm, 5)
              + ",\"headphoneOutputPeakNorm\":" + juce::String (rendererAuditionReactive.headphoneOutputPeakNorm, 5)
              + ",\"headphoneParityNorm\":" + juce::String (rendererAuditionReactive.headphoneParityNorm, 5)
              + ",\"sourceEnergyNorm\":" + rendererAuditionSourceEnergyNormJson + "}"
          + ",\"rendererPhysicsLensEnabled\":" + juce::String (rendererPhysicsLensEnabled ? "true" : "false")
          + ",\"rendererPhysicsLensMix\":" + juce::String (rendererPhysicsLensMix, 3)
          + ",\"rendererSteamAudioCompiled\":" + juce::String (rendererSteamAudioCompiled ? "true" : "false")
          + ",\"rendererSteamAudioAvailable\":" + juce::String (rendererSteamAudioAvailable ? "true" : "false")
          + ",\"rendererSteamAudioInitStage\":\"" + rendererSteamAudioInitStage + "\""
          + ",\"rendererSteamAudioInitErrorCode\":" + juce::String (rendererSteamAudioInitErrorCode)
          + ",\"rendererSteamAudioRuntimeLib\":\"" + rendererSteamAudioRuntimeLib + "\""
          + ",\"rendererSteamAudioMissingSymbol\":\"" + rendererSteamAudioMissingSymbol + "\""
          + ",\"rendererAmbiCompiled\":" + juce::String (rendererAmbiCompiled ? "true" : "false")
          + ",\"rendererAmbiActive\":" + juce::String (rendererAmbiActive ? "true" : "false")
          + ",\"rendererAmbiMaxOrder\":" + juce::String (rendererAmbiMaxOrder)
          + ",\"rendererAmbiNormalization\":\"" + rendererAmbiNormalization + "\""
          + ",\"rendererAmbiChannelOrder\":\"" + rendererAmbiChannelOrder + "\""
          + ",\"rendererAmbiDecodeLayout\":\"" + rendererAmbiDecodeLayout + "\""
          + ",\"rendererAmbiStage\":\"" + rendererAmbiStage + "\""
          + ",\"rendererAmbiIrFrameId\":"
              + juce::String (static_cast<juce::int64> (rendererAmbiIrContract.frameId))
          + ",\"rendererAmbiIrTimestampSamples\":"
              + juce::String (static_cast<juce::int64> (rendererAmbiIrContract.timestampSamples))
          + ",\"rendererAmbiIrOrder\":" + juce::String (rendererAmbiIrContract.order)
          + ",\"rendererAmbiIrNormalization\":\"" + rendererAmbiIrNormalization + "\""
          + ",\"rendererAmbiIrChannelCount\":" + juce::String (rendererAmbiIrContract.channelCount)
          + ",\"rendererAmbiIrRequestedProfile\":\"" + rendererAmbiIrRequestedProfile + "\""
          + ",\"rendererAmbiIrActiveProfile\":\"" + rendererAmbiIrActiveProfile + "\""
          + ",\"rendererAmbiIrStage\":\"" + rendererAmbiIrStage + "\""
          + ",\"rendererAmbiIrRequestedHeadphoneMode\":\"" + rendererAmbiIrRequestedHeadphoneMode + "\""
          + ",\"rendererAmbiIrActiveHeadphoneMode\":\"" + rendererAmbiIrActiveHeadphoneMode + "\""
          + ",\"rendererAmbiIrSteamAudioAvailable\":"
              + juce::String (rendererAmbiIrContract.steamAudioAvailable ? "true" : "false")
          + ",\"rendererAmbiIrHeadphoneRenderAllowed\":"
              + juce::String (rendererAmbiIrContract.headphoneRenderAllowed ? "true" : "false")
          + ",\"rendererAmbiIrFallbackActive\":"
              + juce::String (rendererAmbiIrContract.fallbackActive ? "true" : "false")
          + ",\"rendererAmbiIrContract\":{\"frameId\":"
              + juce::String (static_cast<juce::int64> (rendererAmbiIrContract.frameId))
              + ",\"timestampSamples\":"
              + juce::String (static_cast<juce::int64> (rendererAmbiIrContract.timestampSamples))
              + ",\"order\":" + juce::String (rendererAmbiIrContract.order)
              + ",\"normalization\":\"" + rendererAmbiIrNormalization + "\""
              + ",\"channelCount\":" + juce::String (rendererAmbiIrContract.channelCount)
              + ",\"requestedProfile\":\"" + rendererAmbiIrRequestedProfile + "\""
              + ",\"activeProfile\":\"" + rendererAmbiIrActiveProfile + "\""
              + ",\"stage\":\"" + rendererAmbiIrStage + "\""
              + ",\"requestedHeadphoneMode\":\"" + rendererAmbiIrRequestedHeadphoneMode + "\""
              + ",\"activeHeadphoneMode\":\"" + rendererAmbiIrActiveHeadphoneMode + "\""
              + ",\"steamAudioAvailable\":"
              + juce::String (rendererAmbiIrContract.steamAudioAvailable ? "true" : "false")
              + ",\"headphoneRenderAllowed\":"
              + juce::String (rendererAmbiIrContract.headphoneRenderAllowed ? "true" : "false")
              + ",\"fallbackActive\":"
              + juce::String (rendererAmbiIrContract.fallbackActive ? "true" : "false")
              + "}"
          + ",\"rendererCompatGuardStatus\":\"" + escapeJsonString (rendererCompatGuardStatus) + "\""
          + ",\"rendererCompatGuardBlocker\":\"" + escapeJsonString (rendererCompatGuardBlocker) + "\""
          + ",\"rendererCompatGuardReason\":\"" + escapeJsonString (rendererCompatGuardReason) + "\""
          + ",\"rendererCompatGuardrails\":{\"status\":\""
              + escapeJsonString (rendererCompatGuardStatus) + "\""
              + ",\"blocker\":\"" + escapeJsonString (rendererCompatGuardBlocker) + "\""
              + ",\"reason\":\"" + escapeJsonString (rendererCompatGuardReason) + "\""
              + ",\"profileMatch\":" + juce::String (rendererCompatProfileMatch ? "true" : "false")
              + ",\"headphoneModeMatch\":" + juce::String (rendererCompatHeadphoneModeMatch ? "true" : "false")
              + ",\"ambisonicRequested\":" + juce::String (rendererCompatAmbisonicRequested ? "true" : "false")
              + ",\"ambisonicOrderValid\":" + juce::String (rendererCompatAmbisonicOrderValid ? "true" : "false")
              + ",\"steamFallback\":" + juce::String (rendererCompatSteamFallback ? "true" : "false")
              + ",\"fallbackActive\":" + juce::String (rendererAmbiIrContract.fallbackActive ? "true" : "false")
              + ",\"seq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
              + "}"
          + ",\"rendererAdmMappingStatus\":\"" + escapeJsonString (rendererAdmMappingStatus) + "\""
          + ",\"rendererIamfMappingStatus\":\"" + escapeJsonString (rendererIamfMappingStatus) + "\""
          + ",\"rendererCodecMappingContract\":{\"admStatus\":\""
              + escapeJsonString (rendererAdmMappingStatus) + "\""
              + ",\"iamfStatus\":\"" + escapeJsonString (rendererIamfMappingStatus) + "\""
              + ",\"requiredFields\":" + juce::String (rendererCodecContractRequiredFields)
              + ",\"coveredFields\":" + juce::String (rendererCodecContractCoveredFields)
              + ",\"coveragePct\":" + juce::String (rendererCodecContractCoverage, 4)
              + ",\"signature\":" + juce::String (static_cast<juce::int64> (rendererCodecContractSignature))
              + ",\"mode\":\"" + rendererCodecExecutionMode + "\""
              + ",\"mappingApplied\":" + juce::String (rendererCodecExecution.mappingApplied ? "true" : "false")
              + ",\"finite\":" + juce::String (rendererCodecExecution.finite ? "true" : "false")
              + ",\"fallbackActive\":" + juce::String (rendererCodecExecution.fallbackActive ? "true" : "false")
              + ",\"mappedChannelCount\":" + juce::String (rendererCodecExecution.mappedChannelCount)
              + ",\"objectCount\":" + juce::String (rendererCodecExecution.objectCount)
              + ",\"elementCount\":" + juce::String (rendererCodecExecution.elementCount)
              + ",\"executionFrameId\":"
              + juce::String (static_cast<juce::int64> (rendererCodecExecution.frameId))
              + ",\"executionTimestampSamples\":"
              + juce::String (static_cast<juce::int64> (rendererCodecExecution.timestampSamples))
              + ",\"order\":" + juce::String (rendererAmbiIrContract.order)
              + ",\"normalization\":\"" + rendererAmbiIrNormalization + "\""
              + ",\"channelCount\":" + juce::String (rendererAmbiIrContract.channelCount)
              + ",\"seq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
              + "}"
          + ",\"rendererPilotIntakeStatus\":\"" + escapeJsonString (rendererPilotIntakeStatus) + "\""
          + ",\"rendererPilotIntakeBlocker\":\"" + escapeJsonString (rendererPilotIntakeBlocker) + "\""
          + ",\"rendererPilotIntakeReason\":\"" + escapeJsonString (rendererPilotIntakeReason) + "\""
          + ",\"rendererPilotIntakeGate\":{\"status\":\""
              + escapeJsonString (rendererPilotIntakeStatus) + "\""
              + ",\"blocker\":\"" + escapeJsonString (rendererPilotIntakeBlocker) + "\""
              + ",\"reason\":\"" + escapeJsonString (rendererPilotIntakeReason) + "\""
              + ",\"compatGuardStatus\":\"" + escapeJsonString (rendererCompatGuardStatus) + "\""
              + ",\"admMappingStatus\":\"" + escapeJsonString (rendererAdmMappingStatus) + "\""
              + ",\"iamfMappingStatus\":\"" + escapeJsonString (rendererIamfMappingStatus) + "\""
              + ",\"executionMode\":\"" + escapeJsonString (rendererPilotIntakeExecutionMode) + "\""
              + ",\"executionFinite\":" + juce::String (rendererCodecExecution.finite ? "true" : "false")
              + ",\"executionFallbackActive\":"
              + juce::String (rendererCodecExecution.fallbackActive ? "true" : "false")
              + ",\"seq\":" + juce::String (static_cast<juce::int64> (snapshotSeq))
              + "}"
          + ",\"rendererAdmRuntimePayload\":{\"active\":"
              + juce::String (rendererAdmPayload.active ? "true" : "false")
              + ",\"frameId\":" + juce::String (static_cast<juce::int64> (rendererAdmPayload.frameId))
              + ",\"timestampSamples\":"
              + juce::String (static_cast<juce::int64> (rendererAdmPayload.timestampSamples))
              + ",\"channelCount\":" + juce::String (rendererAdmPayload.channelCount)
              + ",\"objectCount\":" + juce::String (rendererAdmPayload.objectCount)
              + ",\"objects\":" + rendererAdmPayloadObjectsJson
              + "}"
          + ",\"rendererIamfRuntimePayload\":{\"active\":"
              + juce::String (rendererIamfPayload.active ? "true" : "false")
              + ",\"frameId\":" + juce::String (static_cast<juce::int64> (rendererIamfPayload.frameId))
              + ",\"timestampSamples\":"
              + juce::String (static_cast<juce::int64> (rendererIamfPayload.timestampSamples))
              + ",\"channelCount\":" + juce::String (rendererIamfPayload.channelCount)
              + ",\"elementCount\":" + juce::String (rendererIamfPayload.elementCount)
              + ",\"sceneGain\":" + juce::String (rendererIamfPayload.sceneGain, 5)
              + ",\"elements\":" + rendererIamfPayloadElementsJson
              + "}"
          + ",\"clapBuildEnabled\":" + juce::String (clapDiagnostics.buildEnabled ? "true" : "false")
          + ",\"clapPropertiesAvailable\":" + juce::String (clapDiagnostics.propertiesAvailable ? "true" : "false")
          + ",\"clapIsPluginFormat\":" + juce::String (clapDiagnostics.isClapInstance ? "true" : "false")
          + ",\"clapIsActive\":" + juce::String (clapDiagnostics.isActive ? "true" : "false")
          + ",\"clapIsProcessing\":" + juce::String (clapDiagnostics.isProcessing ? "true" : "false")
          + ",\"clapHasTransport\":" + juce::String (clapDiagnostics.hasTransport ? "true" : "false")
          + ",\"clapWrapperType\":\"" + clapWrapperType + "\""
          + ",\"clapLifecycleStage\":\"" + clapLifecycleStage + "\""
          + ",\"clapRuntimeMode\":\"" + clapRuntimeMode + "\""
          + ",\"clapVersion\":{\"major\":" + juce::String (static_cast<int> (clapDiagnostics.versionMajor))
              + ",\"minor\":" + juce::String (static_cast<int> (clapDiagnostics.versionMinor))
              + ",\"revision\":" + juce::String (static_cast<int> (clapDiagnostics.versionRevision))
              + "}"
          + ",\"rendererOutputChannels\":" + outputChannelLabelsJson
          + ",\"rendererInternalSpeakers\":" + internalSpeakerLabelsJson
          + ",\"rendererQuadMap\":" + quadOutputMapJson
          + ",\"calCurrentTopologyProfile\":" + juce::String (currentCalTopologyProfile)
          + ",\"calCurrentTopologyId\":\"" + escapeJsonString (currentCalTopologyId) + "\""
          + ",\"calCurrentMonitoringPath\":" + juce::String (currentCalMonitoringPath)
          + ",\"calCurrentMonitoringPathId\":\"" + escapeJsonString (currentCalMonitoringPathId) + "\""
          + ",\"calCurrentDeviceProfile\":" + juce::String (currentCalDeviceProfile)
          + ",\"calCurrentDeviceProfileId\":\"" + escapeJsonString (currentCalDeviceProfileId) + "\""
          + ",\"calRequiredChannels\":" + juce::String (currentCalRequiredChannels)
          + ",\"calWritableChannels\":" + juce::String (currentCalWritableChannels)
          + ",\"calMappingLimitedToFirst4\":" + juce::String (currentCalMappingLimitedToFirst4 ? "true" : "false")
          + ",\"calTopologyAliasLegacySpeakerConfig\":" + juce::String (legacySpeakerConfigForTopologyIndex (currentCalTopologyProfile))
          + ",\"calCurrentSpeakerConfig\":" + juce::String (currentCalSpeakerConfig)
          + ",\"calCurrentSpeakerMap\":" + currentCalSpeakerRoutingJson
          + ",\"calAutoRoutingApplied\":" + juce::String (hasAppliedAutoDetectedCalibrationRouting ? "true" : "false")
          + ",\"calAutoRoutingOutputChannels\":" + juce::String (lastAutoDetectedOutputChannels)
          + ",\"calAutoRoutingTopologyProfile\":" + juce::String (lastAutoDetectedTopologyProfile)
          + ",\"calAutoRoutingTopologyId\":\"" + escapeJsonString (autoDetectedTopologyId) + "\""
          + ",\"calAutoRoutingSpeakerConfig\":" + juce::String (lastAutoDetectedSpeakerConfig)
          + ",\"calAutoRoutingMap\":" + autoDetectedRoutingJson
          + ",\"roomProfileValid\":" + juce::String (roomProfileValid ? "true" : "false")
          + ",\"roomDimensions\":{\"width\":" + juce::String (roomDimensions.x, 3)
              + ",\"depth\":" + juce::String (roomDimensions.y, 3)
              + ",\"height\":" + juce::String (roomDimensions.z, 3) + "}"
          + ",\"listener\":{\"x\":" + juce::String (listenerPosition.x, 3)
              + ",\"y\":" + juce::String (listenerPosition.y, 3)
              + ",\"z\":" + juce::String (listenerPosition.z, 3) + "}"
          + ",\"speakerRms\":" + speakerRmsJson
          + ",\"speakers\":" + speakersJson
          + ",\"physicsInteraction\":" + juce::String (sceneGraph.isPhysicsInteractionEnabled() ? "true" : "false")
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
