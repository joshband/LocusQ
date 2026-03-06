#include "../SpatialRenderer.h"

void SpatialRenderer::renderInternalAuditionEmitter (int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    const auto levelDb = auditionLevelDbForPreset (auditionLevelPresetIndex);
    const auto signalGain = juce::Decibels::decibelsToGain (levelDb);

    float azimuth = 0.0f;
    float elevation = 0.0f;
    float auditionDistanceMeters = 1.0f;
    double orbitHz = 0.0;
    const auto phaseRadians = juce::MathConstants<double>::twoPi * auditionOrbitPhase;

    switch (auditionMotionTypeIndex)
    {
        case 1:
            orbitHz = 0.08;
            azimuth = static_cast<float> (auditionOrbitPhase * 360.0 - 180.0);
            elevation = static_cast<float> (14.0 * std::sin (phaseRadians * 0.75));
            auditionDistanceMeters = 1.05f + 0.12f * static_cast<float> (0.5 + 0.5 * std::cos (phaseRadians * 0.5));
            break;

        case 2:
            // Orbit Fast is an explicit 3D path, not just a flat azimuth sweep.
            orbitHz = 0.20;
            azimuth = static_cast<float> (auditionOrbitPhase * 360.0 - 180.0);
            elevation = static_cast<float> (60.0 * std::sin (phaseRadians * 1.15));
            auditionDistanceMeters = 0.95f + 0.55f * static_cast<float> (0.5 + 0.5 * std::cos (phaseRadians * 0.8));
            break;
        case 3: // figure8_flow
            orbitHz = 0.13;
            azimuth = 168.0f * static_cast<float> (std::sin (phaseRadians) * std::cos (phaseRadians * 0.5));
            elevation = 30.0f * static_cast<float> (std::sin (phaseRadians * 2.0));
            auditionDistanceMeters = 0.85f + 0.92f * static_cast<float> (0.5 + 0.5 * std::cos (phaseRadians * 1.1));
            break;
        case 4: // helix_rise
            orbitHz = 0.17;
            azimuth = static_cast<float> (auditionOrbitPhase * 360.0 - 180.0);
            elevation = -46.0f + 92.0f * static_cast<float> (0.5 + 0.5 * std::sin (phaseRadians * 0.45));
            auditionDistanceMeters = 0.72f + 1.12f * static_cast<float> (0.5 + 0.5 * std::sin (phaseRadians * 1.7 + 0.7));
            break;
        case 5: // wall_ricochet
        {
            const auto sampleRate = juce::jmax (1.0, currentSampleRate);
            const float dt = static_cast<float> (numSamples / sampleRate);
            const float bounds = qualityHigh ? 2.20f : 1.90f;
            auditionWallPosX += auditionWallVelX * dt;
            auditionWallPosZ += auditionWallVelZ * dt;

            bool collision = false;
            auto reflectAxis = [bounds, &collision, this] (float& pos, float& vel) noexcept
            {
                if (pos > bounds)
                {
                    pos = bounds - (pos - bounds);
                    vel = -std::abs (vel) * (0.93f + 0.04f * nextAuditionRand01());
                    collision = true;
                }
                else if (pos < -bounds)
                {
                    pos = -bounds + (-bounds - pos);
                    vel = std::abs (vel) * (0.93f + 0.04f * nextAuditionRand01());
                    collision = true;
                }
            };
            reflectAxis (auditionWallPosX, auditionWallVelX);
            reflectAxis (auditionWallPosZ, auditionWallVelZ);

            if (collision)
                auditionBounceEnv = juce::jmax (auditionBounceEnv, 0.58f + 0.30f * nextAuditionRand01());

            const float planarDistance = std::sqrt (
                auditionWallPosX * auditionWallPosX + auditionWallPosZ * auditionWallPosZ);
            azimuth = juce::radiansToDegrees (std::atan2 (auditionWallPosX, -auditionWallPosZ));
            elevation = -22.0f + 56.0f * juce::jlimit (0.0f, 1.0f, auditionBounceEnv)
                + 12.0f * static_cast<float> (std::sin (phaseRadians * 1.6));
            auditionDistanceMeters = 0.58f + 0.62f * planarDistance;
            break;
        }

        default:
            break;
    }

    if (auditionMotionTypeIndex != 0)
    {
        const auto phase = static_cast<float> (phaseRadians);
        const auto motionTier = (auditionMotionTypeIndex == 1) ? 0.62f
            : (auditionMotionTypeIndex == 2) ? 1.0f
            : (auditionMotionTypeIndex == 3) ? 1.18f
            : (auditionMotionTypeIndex == 4) ? 1.32f
            : 1.50f;
        const auto qualitySpread = qualityHigh ? 1.0f : 0.72f;

        switch (auditionSignalTypeIndex)
        {
            case 3: // rain_sheet
            {
                const auto sweep = static_cast<float> (std::sin (phase * (0.90f + 0.35f * motionTier)));
                const auto billow = static_cast<float> (std::sin (phase * (1.85f + 0.20f * motionTier)));
                azimuth = 158.0f * qualitySpread * sweep + 26.0f * qualitySpread * billow;
                elevation = -10.0f + 20.0f * qualitySpread
                    * static_cast<float> (std::sin (phase * (1.20f + 0.25f * motionTier)));
                auditionDistanceMeters = 1.08f + (0.42f + 0.24f * qualitySpread)
                    * static_cast<float> (0.5 + 0.5 * std::cos (phase * (1.35f + 0.25f * motionTier)));
                break;
            }
            case 4: // snow_cloud
            {
                const auto cloudA = static_cast<float> (std::sin (phase * (0.34f + 0.12f * motionTier) + 0.6f));
                const auto cloudB = static_cast<float> (std::sin (phase * (0.58f + 0.07f * motionTier) - 1.1f));
                const auto drift = 0.55f * cloudA + 0.45f * cloudB;
                azimuth = 128.0f * qualitySpread * drift;
                elevation = 16.0f + 30.0f * qualitySpread
                    * (0.45f * cloudB
                       + 0.55f * static_cast<float> (std::sin (phase * (0.42f + 0.09f * motionTier) + 0.9f)));
                auditionDistanceMeters = 1.28f + (0.42f + 0.20f * qualitySpread)
                    * static_cast<float> (0.5 + 0.5 * std::sin (phase * (0.39f + 0.05f * motionTier) + 0.35f));
                break;
            }
            case 5: // bounce_cluster
            {
                if (auditionMotionTypeIndex == 5)
                {
                    const auto impact = juce::jlimit (0.0f, 1.0f, auditionBounceEnv);
                    elevation = -18.0f + 62.0f * impact
                        + 10.0f * static_cast<float> (std::sin (phase * 2.1f));
                    auditionDistanceMeters += 0.18f * impact;
                    break;
                }
                const auto impact = juce::jlimit (0.0f, 1.0f, auditionBounceEnv);
                const auto cluster = juce::jlimit (0.0f, 1.0f, static_cast<float> (auditionBounceClusterRemaining) / 6.0f);
                const auto rebound = std::abs (static_cast<float> (std::sin (phase * (1.65f + 0.65f * motionTier))));
                azimuth = 136.0f * qualitySpread * static_cast<float> (std::sin (phase * (0.95f + 0.45f * motionTier)))
                    + 38.0f * cluster * static_cast<float> (std::sin (phase * (2.60f + 0.35f * motionTier)));
                elevation = -24.0f + (38.0f * impact + 16.0f * cluster) * rebound;
                auditionDistanceMeters = 0.96f
                    + 0.72f * (1.0f - impact)
                    + 0.34f * cluster * std::abs (static_cast<float> (std::cos (phase * (1.55f + 0.25f * motionTier))));
                if (qualityHigh)
                    auditionDistanceMeters += 0.14f * cluster * rebound;
                break;
            }
            case 6: // chime_constellation
            {
                const auto chimeA = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseA);
                const auto chimeB = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseB);
                const auto shimmer = juce::jlimit (0.0f, 1.0f, auditionChimeShimmer * 2.2f);
                const auto constellation = static_cast<float> (
                    std::sin (phase * (1.10f + 0.30f * motionTier) + 0.35f * std::sin (chimeA)));
                azimuth = 138.0f * qualitySpread * constellation
                    + 18.0f * qualitySpread * static_cast<float> (std::sin (chimeB * 0.5f));
                elevation = 18.0f + 34.0f * qualitySpread * std::abs (static_cast<float> (
                    std::sin (chimeB * 0.45f + phase * (0.60f + 0.18f * motionTier))));
                auditionDistanceMeters = 0.82f + (0.30f + 0.12f * qualitySpread)
                    * (0.45f + 0.55f * std::abs (static_cast<float> (std::sin (chimeA * 0.5f))));
                auditionDistanceMeters += 0.12f * shimmer * qualitySpread;
                break;
            }
            case 7: // crickets
            {
                const auto chatter = static_cast<float> (std::sin (phase * (1.85f + 0.55f * motionTier)));
                azimuth = 172.0f * qualitySpread * chatter;
                elevation = -12.0f + 10.0f * static_cast<float> (std::sin (phase * 2.7f));
                auditionDistanceMeters = 1.18f + 0.82f * static_cast<float> (
                    0.5f + 0.5f * std::sin (phase * (1.35f + 0.22f * motionTier)));
                break;
            }
            case 8: // song_birds
            {
                const auto swirl = static_cast<float> (std::sin (phase * (0.86f + 0.30f * motionTier)));
                azimuth = 160.0f * qualitySpread * swirl;
                elevation = 26.0f + 34.0f * qualitySpread * std::abs (static_cast<float> (
                    std::sin (phase * (1.40f + 0.35f * motionTier))));
                auditionDistanceMeters = 1.05f + 0.96f * static_cast<float> (
                    0.5f + 0.5f * std::cos (phase * (1.05f + 0.18f * motionTier)));
                break;
            }
            case 9: // karplus_plucks
            {
                const auto pluckWave = static_cast<float> (std::sin (phase * (1.20f + 0.28f * motionTier)));
                azimuth = 148.0f * qualitySpread * pluckWave;
                elevation = -6.0f + 18.0f * static_cast<float> (std::sin (phase * 1.9f));
                auditionDistanceMeters = 0.92f + 0.84f * static_cast<float> (
                    0.5f + 0.5f * std::cos (phase * (1.25f + 0.24f * motionTier)));
                break;
            }
            case 10: // membrane_drops
            {
                const auto throb = std::abs (static_cast<float> (std::sin (phase * (1.45f + 0.35f * motionTier))));
                azimuth = 164.0f * qualitySpread * static_cast<float> (std::sin (phase * 0.9f));
                elevation = -18.0f + 32.0f * throb;
                auditionDistanceMeters = 1.04f + 0.92f * static_cast<float> (
                    0.5f + 0.5f * std::cos (phase * (1.55f + 0.20f * motionTier)));
                break;
            }
            case 11: // krell_patch
            {
                const auto glide = static_cast<float> (std::sin (phase * (0.66f + 0.24f * motionTier)));
                azimuth = 170.0f * qualitySpread * glide;
                elevation = -4.0f + 40.0f * static_cast<float> (std::sin (phase * 1.25f + 0.6f));
                auditionDistanceMeters = 0.80f + 1.10f * static_cast<float> (
                    0.5f + 0.5f * std::sin (phase * (1.15f + 0.20f * motionTier) + 0.35f));
                break;
            }
            case 12: // generative_arp
            {
                const auto lattice = static_cast<float> (std::sin (phase * (1.45f + 0.34f * motionTier)));
                azimuth = 158.0f * qualitySpread * lattice;
                elevation = 4.0f + 28.0f * std::abs (static_cast<float> (
                    std::sin (phase * (2.05f + 0.18f * motionTier))));
                auditionDistanceMeters = 0.88f + 1.04f * static_cast<float> (
                    0.5f + 0.5f * std::cos (phase * (1.32f + 0.25f * motionTier)));
                break;
            }
            default:
                break;
        }

        azimuth = juce::jlimit (-170.0f, 170.0f, azimuth);
        elevation = juce::jlimit (-65.0f, 65.0f, elevation);
        auditionDistanceMeters = juce::jlimit (0.55f, 2.20f, auditionDistanceMeters);
    }

    if (orbitHz > 0.0)
    {
        const auto sampleRate = juce::jmax (1.0, currentSampleRate);
        auditionOrbitPhase += (orbitHz * static_cast<double> (numSamples)) / sampleRate;
        auditionOrbitPhase -= std::floor (auditionOrbitPhase);
    }

    const auto azimuthRadians = juce::degreesToRadians (azimuth);
    const auto elevationRadians = juce::degreesToRadians (elevation);
    const auto cosElevation = std::cos (elevationRadians);
    auditionVisualX.store (std::sin (azimuthRadians) * cosElevation * auditionDistanceMeters, std::memory_order_relaxed);
    auditionVisualY.store (1.2f + std::sin (elevationRadians) * auditionDistanceMeters, std::memory_order_relaxed);
    auditionVisualZ.store (-std::cos (azimuthRadians) * cosElevation * auditionDistanceMeters, std::memory_order_relaxed);

    const auto cloudBoundAvailable = isAuditionCloudBoundModeAvailable();
    const auto requestedVoiceCount = juce::jlimit (1, AUDITION_MAX_VOICES, getAuditionVoiceCountForSignal());
    const auto activeVoices = cloudBoundAvailable ? requestedVoiceCount : 1;
    const auto multiSourceSignal = cloudBoundAvailable && activeVoices > 1;
    const auto spreadDegrees = getAuditionVoiceSpreadDegrees();
    const auto motionSpreadBlend = auditionMotionTypeIndex == 0 ? 1.0f
        : auditionMotionTypeIndex == 1 ? 0.72f
        : auditionMotionTypeIndex == 2 ? 0.62f
        : auditionMotionTypeIndex == 3 ? 0.56f
        : auditionMotionTypeIndex == 4 ? 0.50f
        : 0.42f;
    const auto motionEnergy = auditionMotionTypeIndex == 0 ? 0.0f
        : auditionMotionTypeIndex == 1 ? 0.28f
        : auditionMotionTypeIndex == 2 ? 0.55f
        : auditionMotionTypeIndex == 3 ? 0.72f
        : auditionMotionTypeIndex == 4 ? 0.88f
        : 1.0f;
    const auto physicsVelocityTarget = auditionPhysicsReactiveInputActive ? auditionPhysicsReactiveVelocityTarget : 0.0f;
    const auto physicsCollisionTarget = auditionPhysicsReactiveInputActive ? auditionPhysicsReactiveCollisionTarget : 0.0f;
    const auto physicsDensityTarget = auditionPhysicsReactiveInputActive ? auditionPhysicsReactiveDensityTarget : 0.0f;
    auditionPhysicsReactiveVelocityState += (physicsVelocityTarget - auditionPhysicsReactiveVelocityState) * 0.24f;
    auditionPhysicsReactiveCollisionState += (physicsCollisionTarget - auditionPhysicsReactiveCollisionState) * 0.30f;
    auditionPhysicsReactiveDensityState += (physicsDensityTarget - auditionPhysicsReactiveDensityState) * 0.18f;
    const auto physicsVelocityNorm = juce::jlimit (0.0f, 1.0f, auditionPhysicsReactiveVelocityState);
    const auto physicsCollisionNorm = juce::jlimit (0.0f, 1.0f, auditionPhysicsReactiveCollisionState);
    const auto physicsDensityNorm = juce::jlimit (0.0f, 1.0f, auditionPhysicsReactiveDensityState);
    const auto physicsCouplingNorm = juce::jlimit (
        0.0f,
        1.0f,
        0.44f * physicsVelocityNorm + 0.36f * physicsCollisionNorm + 0.20f * physicsDensityNorm);
    const auto phase = static_cast<float> (phaseRadians);

    std::array<int, AUDITION_MAX_VOICES> voiceDelaySamples {};
    std::array<float, AUDITION_MAX_VOICES> voiceLevelWeights {};
    std::array<double, AUDITION_MAX_VOICES> voiceSquareSum {};
    float voiceWeightSum = 0.0f;

    for (int voice = 0; voice < AUDITION_MAX_VOICES; ++voice)
    {
        if (voice >= activeVoices)
        {
            for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
                auditionSmoothedSpeakerGains[static_cast<size_t> (voice)][static_cast<size_t> (spk)].setTargetValue (0.0f);
            continue;
        }

        auto voiceAzimuth = azimuth;
        auto voiceElevation = elevation;
        auto voiceDistanceMeters = auditionDistanceMeters;
        const auto hashA = auditionVoiceHashUnit (voice, 0xA53C9E11u);
        const auto hashB = auditionVoiceHashUnit (voice, 0x3C6EF372u);
        const auto hashC = auditionVoiceHashUnit (voice, 0xBB67AE85u);
        const auto voiceNorm = activeVoices > 1
            ? static_cast<float> (voice) / static_cast<float> (activeVoices - 1)
            : 0.0f;
        const auto ringAzimuth = -180.0f + (360.0f * voiceNorm) + (hashA - 0.5f) * 18.0f;

        if (multiSourceSignal)
        {
            const auto ringRadians = juce::degreesToRadians (ringAzimuth);
            const auto azimuthWobble = spreadDegrees
                * static_cast<float> (std::sin (phase * (0.65f + 0.22f * hashB)
                                                  + ringRadians * (1.0f + 0.35f * motionEnergy)));
            const auto mixedAzimuth = voiceAzimuth * (1.0f - motionSpreadBlend)
                + (ringAzimuth + azimuthWobble * (0.28f + 0.42f * motionEnergy)) * motionSpreadBlend;
            voiceAzimuth = wrapAuditionAzimuthDegrees (mixedAzimuth);

            const auto elevationSpread = (auditionSignalTypeIndex == 4 || auditionSignalTypeIndex == 8)
                ? 44.0f : 30.0f;
            const auto elevationWobble = elevationSpread
                * static_cast<float> (std::sin (phase * (0.85f + 0.26f * hashC)
                                                  + ringRadians * (0.65f + 0.22f * hashA)));
            voiceElevation = juce::jlimit (
                -65.0f,
                65.0f,
                voiceElevation + (hashB - 0.5f) * elevationSpread * 0.6f
                    + elevationWobble * (0.24f + 0.40f * motionEnergy));

            const auto distanceSpread = 0.26f + 0.40f * hashC;
            const auto distanceWobble = static_cast<float> (std::sin (
                phase * (0.52f + 0.28f * hashA) + ringRadians * (0.75f + 0.30f * hashB)));
            voiceDistanceMeters = juce::jlimit (
                0.55f,
                2.20f,
                voiceDistanceMeters + distanceSpread * distanceWobble);
        }

        const auto panGains = vbapPanner.calculateGains (voiceAzimuth, voiceElevation);
        const auto distanceGain = distanceAttenuator.calculateGain (voiceDistanceMeters);
        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            auditionSmoothedSpeakerGains[static_cast<size_t> (voice)][static_cast<size_t> (spk)].setTargetValue (
                panGains.gains[static_cast<size_t> (spk)] * distanceGain);
        }

        voiceDelaySamples[static_cast<size_t> (voice)] = getAuditionVoiceDelaySamples (voice, activeVoices);
        voiceLevelWeights[static_cast<size_t> (voice)] = multiSourceSignal
            ? (0.62f + 0.38f * hashB)
            : 1.0f;
        voiceWeightSum += voiceLevelWeights[static_cast<size_t> (voice)];
    }

    if (voiceWeightSum > 0.0f)
    {
        const auto invVoiceWeightSum = 1.0f / voiceWeightSum;
        for (int voice = 0; voice < activeVoices; ++voice)
            voiceLevelWeights[static_cast<size_t> (voice)] *= invVoiceWeightSum;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        auto generated = generateAuditionSignalSample();
        generated = applyAuditionPhysicsReactiveTimbre (
            generated,
            physicsVelocityNorm,
            physicsCollisionNorm,
            physicsDensityNorm,
            motionEnergy);
        tempMonoBuffer[static_cast<size_t> (i)] = generated * signalGain;
    }

    double mixedSquareSum = 0.0;
    double mixedHighSquareSum = 0.0;
    float mixedPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto drySample = tempMonoBuffer[static_cast<size_t> (i)];
        auditionHistoryBuffer[static_cast<size_t> (auditionHistoryWritePos)] = drySample;
        auditionHistoryWritePos = (auditionHistoryWritePos + 1) % AUDITION_HISTORY_BUFFER_SAMPLES;
        float mixedVoiceSample = 0.0f;

        for (int voice = 0; voice < AUDITION_MAX_VOICES; ++voice)
        {
            const auto delayedSample = readAuditionHistoryDelayed (voiceDelaySamples[static_cast<size_t> (voice)]);
            const auto voiceBaseLevel = voiceLevelWeights[static_cast<size_t> (voice)];
            auto& voiceModPhase = auditionVoiceModPhase[static_cast<size_t> (voice)];
            const auto voiceLfoHz = 0.22 + 0.31 * auditionVoiceHashUnit (voice, 0xC2B2AE35u);
            voiceModPhase += voiceLfoHz / juce::jmax (1.0, currentSampleRate);
            voiceModPhase -= std::floor (voiceModPhase);
            const auto modulation = 0.90f + 0.10f * static_cast<float> (
                std::sin (juce::MathConstants<double>::twoPi * voiceModPhase));
            const auto voiceExcitedSample = (voice < activeVoices && multiSourceSignal)
                ? renderAuditionVoiceExcitation (voice, activeVoices, delayedSample)
                : delayedSample;
            const auto voiceSample = voiceExcitedSample * voiceBaseLevel * modulation;
            if (voice < activeVoices)
            {
                mixedVoiceSample += voiceSample;
                voiceSquareSum[static_cast<size_t> (voice)] +=
                    static_cast<double> (voiceSample) * static_cast<double> (voiceSample);
            }

            for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
            {
                const auto gain = auditionSmoothedSpeakerGains[static_cast<size_t> (voice)][static_cast<size_t> (spk)].getNextValue();
                accumBuffer.addSample (spk, i, voiceSample * gain);
            }
        }

        const auto mixedAbs = std::abs (mixedVoiceSample);
        mixedPeak = juce::jmax (mixedPeak, mixedAbs);
        mixedSquareSum += static_cast<double> (mixedVoiceSample) * static_cast<double> (mixedVoiceSample);
        auditionReactiveBrightnessLowpassState += (mixedVoiceSample - auditionReactiveBrightnessLowpassState) * 0.08f;
        const auto highComponent = mixedVoiceSample - auditionReactiveBrightnessLowpassState;
        mixedHighSquareSum += static_cast<double> (highComponent) * static_cast<double> (highComponent);
    }

    const auto invNumSamples = 1.0f / static_cast<float> (numSamples);
    const auto blockRms = juce::jlimit (
        0.0f,
        2.0f,
        std::sqrt (static_cast<float> (mixedSquareSum * static_cast<double> (invNumSamples))));
    const auto blockPeak = juce::jlimit (0.0f, 2.0f, mixedPeak);
    const auto blockHighRms = juce::jlimit (
        0.0f,
        2.0f,
        std::sqrt (static_cast<float> (mixedHighSquareSum * static_cast<double> (invNumSamples))));

    const auto fastAlpha = qualityHigh ? 0.27f : 0.20f;
    const auto slowAlpha = qualityHigh ? 0.08f : 0.06f;
    auditionReactiveEnvFastState += (blockRms - auditionReactiveEnvFastState) * fastAlpha;
    auditionReactiveEnvSlowState += (blockRms - auditionReactiveEnvSlowState) * slowAlpha;
    auditionReactiveEnvFastState = juce::jlimit (0.0f, 2.0f, auditionReactiveEnvFastState);
    auditionReactiveEnvSlowState = juce::jlimit (0.0f, 2.0f, auditionReactiveEnvSlowState);

    auto onset = juce::jlimit (
        0.0f,
        1.0f,
        (auditionReactiveEnvFastState - auditionReactiveEnvSlowState) * 5.0f);
    auto brightness = juce::jlimit (
        0.0f,
        1.0f,
        blockHighRms / juce::jmax (0.001f, blockRms * 1.8f + 0.05f));
    const auto sourceDensityNorm = juce::jlimit (
        0.0f,
        1.0f,
        static_cast<float> (activeVoices) / static_cast<float> (AUDITION_MAX_VOICES));
    const auto coupledDensityNorm = juce::jlimit (
        0.0f,
        1.0f,
        0.70f * sourceDensityNorm + 0.30f * physicsDensityNorm);

    onset = juce::jlimit (
        0.0f,
        1.0f,
        onset + 0.34f * physicsCollisionNorm * (0.40f + 0.60f * physicsVelocityNorm));
    brightness = juce::jlimit (
        0.0f,
        1.0f,
        brightness + 0.28f * physicsVelocityNorm + 0.10f * physicsCollisionNorm);

    auto rainFadeRate = 0.10f
        + 0.45f * auditionReactiveEnvFastState
        + 0.25f * onset
        + 0.10f * brightness
        + 0.10f * motionEnergy
        + 0.16f * physicsVelocityNorm
        + 0.22f * physicsCollisionNorm
        + 0.08f * coupledDensityNorm;
    auto snowFadeRate = 0.12f
        + 0.42f * auditionReactiveEnvSlowState
        + 0.18f * (1.0f - brightness)
        + 0.10f * (1.0f - onset)
        + 0.12f * coupledDensityNorm
        + 0.16f * physicsDensityNorm
        + 0.08f * (1.0f - physicsVelocityNorm)
        + 0.08f * physicsCollisionNorm;

    if (auditionSignalTypeIndex == 3) // rain
    {
        rainFadeRate += 0.20f;
        snowFadeRate *= 0.74f;
    }
    else if (auditionSignalTypeIndex == 4) // snow
    {
        snowFadeRate += 0.20f;
        rainFadeRate *= 0.78f;
    }

    rainFadeRate = juce::jlimit (0.0f, 1.0f, rainFadeRate);
    snowFadeRate = juce::jlimit (0.0f, 1.0f, snowFadeRate);

    std::array<float, AUDITION_MAX_VOICES> sourceEnergy {};
    float maxVoiceRms = 0.0f;
    for (int voice = 0; voice < activeVoices; ++voice)
    {
        sourceEnergy[static_cast<size_t> (voice)] = juce::jlimit (
            0.0f,
            2.0f,
            std::sqrt (static_cast<float> (voiceSquareSum[static_cast<size_t> (voice)] * static_cast<double> (invNumSamples))));
        sourceEnergy[static_cast<size_t> (voice)] = juce::jlimit (
            0.0f,
            2.0f,
            sourceEnergy[static_cast<size_t> (voice)] * (0.88f + 0.24f * physicsCouplingNorm));
        maxVoiceRms = juce::jmax (maxVoiceRms, sourceEnergy[static_cast<size_t> (voice)]);
    }

    if (maxVoiceRms > 1.0e-6f)
    {
        const auto invMaxVoice = 1.0f / maxVoiceRms;
        for (int voice = 0; voice < activeVoices; ++voice)
        {
            sourceEnergy[static_cast<size_t> (voice)] = juce::jlimit (
                0.0f,
                1.0f,
                sourceEnergy[static_cast<size_t> (voice)] * invMaxVoice);
        }
    }

    publishAuditionReactiveTelemetry (
        blockRms,
        blockPeak,
        auditionReactiveEnvFastState,
        auditionReactiveEnvSlowState,
        onset,
        brightness,
        rainFadeRate,
        snowFadeRate,
        physicsVelocityNorm,
        physicsCollisionNorm,
        coupledDensityNorm,
        physicsCouplingNorm,
        0.0f,
        0.0f,
        1.0f,
        static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None),
        sourceEnergy,
        activeVoices);
}
