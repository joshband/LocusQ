#include "../SpatialRenderer.h"

float SpatialRenderer::auditionLevelDbForPreset (int presetIndex) noexcept
{
    return locusq::spatial_audition_primitives::auditionLevelDbForPreset (presetIndex);
}

float SpatialRenderer::advanceAuditionOscillator (double frequencyHz, double& phase) const noexcept
{
    return locusq::spatial_audition_primitives::advanceOscillator (
        frequencyHz,
        phase,
        currentSampleRate);
}

float SpatialRenderer::nextAuditionWhiteNoise() noexcept
{
    return locusq::spatial_audition_primitives::nextWhiteNoise (auditionNoiseState);
}

float SpatialRenderer::nextAuditionRand01() noexcept
{
    return locusq::spatial_audition_primitives::nextRand01 (auditionNoiseState);
}

float SpatialRenderer::wrapAuditionAzimuthDegrees (float azimuthDeg) noexcept
{
    return locusq::spatial_audition_primitives::wrapAzimuthDegrees (azimuthDeg);
}

float SpatialRenderer::auditionVoiceHashUnit (int voiceIndex, std::uint32_t salt) noexcept
{
    return locusq::spatial_audition_primitives::voiceHashUnit (voiceIndex, salt);
}

void SpatialRenderer::resetAuditionVoiceFieldStates() noexcept
{
    std::fill (auditionVoiceModPhase.begin(), auditionVoiceModPhase.end(), 0.0);
    std::fill (auditionVoiceExciterPhaseA.begin(), auditionVoiceExciterPhaseA.end(), 0.0);
    std::fill (auditionVoiceExciterPhaseB.begin(), auditionVoiceExciterPhaseB.end(), 0.0);
    std::fill (auditionVoiceExciterEnv.begin(), auditionVoiceExciterEnv.end(), 0.0f);
    std::fill (auditionVoiceExciterCooldownSamples.begin(), auditionVoiceExciterCooldownSamples.end(), 0);

    for (int voice = 0; voice < AUDITION_MAX_VOICES; ++voice)
    {
        auto seed = 0x13579BDFu ^ (0x9E3779B9u * static_cast<std::uint32_t> (voice + 1));
        seed ^= static_cast<std::uint32_t> (auditionSignalTypeIndex + 1) * 0x85EBCA6Bu;
        auditionVoiceNoiseState[static_cast<size_t> (voice)] = seed;
    }
}

bool SpatialRenderer::isAuditionCloudBoundModeAvailable() const noexcept
{
    if (currentSampleRate < 8000.0 || currentBlockSize <= 0)
        return false;

    return currentBlockSize <= 2048
        && currentBlockSize <= (AUDITION_HISTORY_BUFFER_SAMPLES / 2);
}

float SpatialRenderer::renderAuditionVoiceExcitation (int voiceIndex, int activeVoices, float delayedSample) noexcept
{
    const auto idx = static_cast<size_t> (juce::jlimit (0, AUDITION_MAX_VOICES - 1, voiceIndex));
    locusq::spatial_audition_engine::VoiceExcitationState state
    {
        auditionVoiceExciterPhaseA[idx],
        auditionVoiceExciterPhaseB[idx],
        auditionVoiceExciterEnv[idx],
        auditionVoiceExciterCooldownSamples[idx],
        auditionVoiceNoiseState[idx]
    };
    const locusq::spatial_audition_engine::VoiceExcitationInput input
    {
        voiceIndex,
        activeVoices,
        auditionSignalTypeIndex,
        qualityHigh,
        delayedSample,
        currentSampleRate
    };
    const auto excited = locusq::spatial_audition_engine::renderVoiceExcitation (state, input);
    auditionVoiceExciterPhaseA[idx] = state.phaseA;
    auditionVoiceExciterPhaseB[idx] = state.phaseB;
    auditionVoiceExciterEnv[idx] = state.env;
    auditionVoiceExciterCooldownSamples[idx] = state.cooldownSamples;
    auditionVoiceNoiseState[idx] = state.noiseState;
    return excited;
}

bool SpatialRenderer::isAuditionMultiSourceSignal (int signalIndex) const noexcept
{
    return locusq::spatial_audition_primitives::isMultiSourceSignal (signalIndex);
}

int SpatialRenderer::getAuditionVoiceCountForSignal() const noexcept
{
    return locusq::spatial_audition_primitives::voiceCountForSignal (
        auditionSignalTypeIndex,
        qualityHigh);
}

float SpatialRenderer::getAuditionVoiceSpreadDegrees() const noexcept
{
    return locusq::spatial_audition_primitives::voiceSpreadDegreesForSignal (auditionSignalTypeIndex);
}

int SpatialRenderer::getAuditionVoiceDelaySamples (int voiceIndex, int voiceCount) const noexcept
{
    return locusq::spatial_audition_primitives::voiceDelaySamplesForSignal (
        auditionSignalTypeIndex,
        qualityHigh,
        voiceIndex,
        voiceCount,
        currentSampleRate,
        AUDITION_HISTORY_BUFFER_SAMPLES);
}

float SpatialRenderer::readAuditionHistoryDelayed (int delaySamples) const noexcept
{
    const auto boundedDelay = juce::jlimit (0, AUDITION_HISTORY_BUFFER_SAMPLES - 1, delaySamples);
    auto readIndex = auditionHistoryWritePos - 1 - boundedDelay;
    while (readIndex < 0)
        readIndex += AUDITION_HISTORY_BUFFER_SAMPLES;
    return auditionHistoryBuffer[static_cast<size_t> (readIndex)];
}

void SpatialRenderer::publishAuditionReactiveTelemetry (float rms,
                                                        float peak,
                                                        float envFast,
                                                        float envSlow,
                                                        float onset,
                                                        float brightness,
                                                        float rainFadeRate,
                                                        float snowFadeRate,
                                                        float physicsVelocity,
                                                        float physicsCollision,
                                                        float physicsDensity,
                                                        float physicsCoupling,
                                                        float headphoneOutputRms,
                                                        float headphoneOutputPeak,
                                                        float headphoneParity,
                                                        int headphoneFallbackReasonIndex,
                                                        const std::array<float, AUDITION_MAX_VOICES>& sourceEnergy,
                                                        int sourceCount) noexcept
{
    auditionReactiveRms.store (sanitizeUnitScalar (rms), std::memory_order_relaxed);
    auditionReactivePeak.store (sanitizeUnitScalar (peak), std::memory_order_relaxed);
    auditionReactiveEnvFast.store (sanitizeUnitScalar (envFast), std::memory_order_relaxed);
    auditionReactiveEnvSlow.store (sanitizeUnitScalar (envSlow), std::memory_order_relaxed);
    auditionReactiveOnset.store (sanitizeUnitScalar (onset), std::memory_order_relaxed);
    auditionReactiveBrightness.store (sanitizeUnitScalar (brightness), std::memory_order_relaxed);
    auditionReactiveRainFadeRate.store (sanitizeUnitScalar (rainFadeRate), std::memory_order_relaxed);
    auditionReactiveSnowFadeRate.store (sanitizeUnitScalar (snowFadeRate), std::memory_order_relaxed);
    auditionReactivePhysicsVelocity.store (sanitizeUnitScalar (physicsVelocity), std::memory_order_relaxed);
    auditionReactivePhysicsCollision.store (sanitizeUnitScalar (physicsCollision), std::memory_order_relaxed);
    auditionReactivePhysicsDensity.store (sanitizeUnitScalar (physicsDensity), std::memory_order_relaxed);
    auditionReactivePhysicsCoupling.store (sanitizeUnitScalar (physicsCoupling), std::memory_order_relaxed);
    auditionReactiveHeadphoneOutputRms.store (
        sanitizeUnitScalar (headphoneOutputRms),
        std::memory_order_relaxed);
    auditionReactiveHeadphoneOutputPeak.store (
        sanitizeUnitScalar (headphoneOutputPeak),
        std::memory_order_relaxed);
    auditionReactiveHeadphoneParity.store (
        sanitizeUnitScalar (headphoneParity),
        std::memory_order_relaxed);
    auditionReactiveHeadphoneFallbackReasonIndex.store (
        sanitizeHeadphoneFallbackReasonIndex (headphoneFallbackReasonIndex),
        std::memory_order_relaxed);
    auditionReactiveSourceCount.store (
        sanitizeSourceCount (sourceCount),
        std::memory_order_relaxed);

    for (int i = 0; i < AUDITION_MAX_VOICES; ++i)
    {
        auditionReactiveSourceEnergy[static_cast<size_t> (i)].store (
            sanitizeUnitScalar (sourceEnergy[static_cast<size_t> (i)]),
            std::memory_order_relaxed);
    }
}

void SpatialRenderer::resetAuditionReactiveTelemetry() noexcept
{
    auditionReactiveEnvFastState = 0.0f;
    auditionReactiveEnvSlowState = 0.0f;
    auditionReactiveBrightnessLowpassState = 0.0f;
    auditionPhysicsReactiveInputActive = false;
    auditionPhysicsReactiveVelocityTarget = 0.0f;
    auditionPhysicsReactiveCollisionTarget = 0.0f;
    auditionPhysicsReactiveDensityTarget = 0.0f;
    auditionPhysicsReactiveVelocityState = 0.0f;
    auditionPhysicsReactiveCollisionState = 0.0f;
    auditionPhysicsReactiveDensityState = 0.0f;
    auditionPhysicsReactiveTimbreLowpassState = 0.0f;
    std::array<float, AUDITION_MAX_VOICES> sourceEnergy {};
    publishAuditionReactiveTelemetry (
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None),
        sourceEnergy,
        0);
}

void SpatialRenderer::applyAuditionReactiveHeadphoneParity (float headphoneOutputRms,
                                                            float headphoneOutputPeak,
                                                            float headphoneParity,
                                                            int headphoneFallbackReasonIndex) noexcept
{
    const auto parity = sanitizeUnitScalar (headphoneParity, 1.0f);
    const auto scaledRms = juce::jlimit (
        0.0f,
        1.0f,
        auditionReactiveRms.load (std::memory_order_relaxed) * parity);
    const auto scaledPeak = juce::jlimit (
        0.0f,
        1.0f,
        auditionReactivePeak.load (std::memory_order_relaxed) * parity);
    const auto scaledEnvFast = juce::jlimit (
        0.0f,
        1.0f,
        auditionReactiveEnvFast.load (std::memory_order_relaxed) * parity);
    const auto scaledEnvSlow = juce::jlimit (
        0.0f,
        1.0f,
        auditionReactiveEnvSlow.load (std::memory_order_relaxed) * parity);
    const auto onset = auditionReactiveOnset.load (std::memory_order_relaxed);
    const auto brightness = auditionReactiveBrightness.load (std::memory_order_relaxed);
    const auto parityBlend = 0.72f + 0.28f * parity;
    const auto scaledRainFadeRate = juce::jlimit (
        0.0f,
        1.0f,
        auditionReactiveRainFadeRate.load (std::memory_order_relaxed) * parityBlend);
    const auto scaledSnowFadeRate = juce::jlimit (
        0.0f,
        1.0f,
        auditionReactiveSnowFadeRate.load (std::memory_order_relaxed) * parityBlend);

    auditionReactiveRms.store (scaledRms, std::memory_order_relaxed);
    auditionReactivePeak.store (scaledPeak, std::memory_order_relaxed);
    auditionReactiveEnvFast.store (scaledEnvFast, std::memory_order_relaxed);
    auditionReactiveEnvSlow.store (scaledEnvSlow, std::memory_order_relaxed);
    auditionReactiveOnset.store (juce::jlimit (0.0f, 1.0f, onset), std::memory_order_relaxed);
    auditionReactiveBrightness.store (juce::jlimit (0.0f, 1.0f, brightness), std::memory_order_relaxed);
    auditionReactiveRainFadeRate.store (scaledRainFadeRate, std::memory_order_relaxed);
    auditionReactiveSnowFadeRate.store (scaledSnowFadeRate, std::memory_order_relaxed);
    auditionReactiveHeadphoneOutputRms.store (
        sanitizeUnitScalar (headphoneOutputRms),
        std::memory_order_relaxed);
    auditionReactiveHeadphoneOutputPeak.store (
        sanitizeUnitScalar (headphoneOutputPeak),
        std::memory_order_relaxed);
    auditionReactiveHeadphoneParity.store (parity, std::memory_order_relaxed);
    auditionReactiveHeadphoneFallbackReasonIndex.store (
        sanitizeHeadphoneFallbackReasonIndex (headphoneFallbackReasonIndex),
        std::memory_order_relaxed);
}

float SpatialRenderer::applyAuditionPhysicsReactiveTimbre (float sample,
                                                           float physicsVelocity,
                                                           float physicsCollision,
                                                           float physicsDensity,
                                                           float motionEnergy) noexcept
{
    locusq::spatial_audition_engine::PhysicsReactiveState state
    {
        auditionPhysicsReactiveTimbreLowpassState
    };
    const locusq::spatial_audition_engine::PhysicsReactiveInput input
    {
        auditionSignalTypeIndex,
        sample,
        physicsVelocity,
        physicsCollision,
        physicsDensity,
        motionEnergy
    };
    const auto shaped = locusq::spatial_audition_engine::applyPhysicsReactiveTimbre (state, input);
    auditionPhysicsReactiveTimbreLowpassState = state.timbreLowpassState;
    return shaped;
}
