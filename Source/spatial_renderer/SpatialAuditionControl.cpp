#include "../SpatialRenderer.h"

void SpatialRenderer::setAuditionEnabled (bool enabled) noexcept
{
    auditionEnabled = enabled;
}

void SpatialRenderer::setAuditionSignalType (int signalTypeIndex) noexcept
{
    const auto clamped = juce::jlimit (0, 12, signalTypeIndex);
    if (auditionSignalTypeIndex == clamped)
        return;

    auditionSignalTypeIndex = clamped;
    resetAuditionVoiceFieldStates();
}

void SpatialRenderer::setAuditionMotionType (int motionTypeIndex) noexcept
{
    auditionMotionTypeIndex = juce::jlimit (0, 5, motionTypeIndex);
}

void SpatialRenderer::setAuditionLevelPreset (int levelPresetIndex) noexcept
{
    auditionLevelPresetIndex = juce::jlimit (0, 4, levelPresetIndex);
}

void SpatialRenderer::setAuditionPhysicsReactiveInput (bool active,
                                                       float velocityNorm,
                                                       float collisionNorm,
                                                       float densityNorm) noexcept
{
    auditionPhysicsReactiveInputActive = active;
    auditionPhysicsReactiveVelocityTarget = sanitizeUnitScalar (velocityNorm);
    auditionPhysicsReactiveCollisionTarget = sanitizeUnitScalar (collisionNorm);
    auditionPhysicsReactiveDensityTarget = sanitizeUnitScalar (densityNorm);
}

const char* SpatialRenderer::auditionReactiveHeadphoneFallbackReasonToString (int reasonIndex) noexcept
{
    switch (static_cast<AuditionReactiveHeadphoneFallbackReason> (reasonIndex))
    {
        case AuditionReactiveHeadphoneFallbackReason::None: return "none";
        case AuditionReactiveHeadphoneFallbackReason::SteamUnavailable: return "steam_unavailable";
        case AuditionReactiveHeadphoneFallbackReason::SteamRenderFailed: return "steam_render_failed";
        case AuditionReactiveHeadphoneFallbackReason::OutputIncompatible: return "output_incompatible";
        default: break;
    }

    return "unknown";
}

bool SpatialRenderer::isAuditionVisualActive() const noexcept
{
    return auditionVisualActive.load (std::memory_order_relaxed);
}

float SpatialRenderer::getAuditionVisualX() const noexcept
{
    return auditionVisualX.load (std::memory_order_relaxed);
}

float SpatialRenderer::getAuditionVisualY() const noexcept
{
    return auditionVisualY.load (std::memory_order_relaxed);
}

float SpatialRenderer::getAuditionVisualZ() const noexcept
{
    return auditionVisualZ.load (std::memory_order_relaxed);
}

SpatialRenderer::AuditionReactiveSnapshot SpatialRenderer::getAuditionReactiveSnapshot() const noexcept
{
    AuditionReactiveSnapshot snapshot;
    snapshot.rms = sanitizeUnitScalar (auditionReactiveRms.load (std::memory_order_relaxed));
    snapshot.peak = sanitizeUnitScalar (auditionReactivePeak.load (std::memory_order_relaxed));
    snapshot.envFast = sanitizeUnitScalar (auditionReactiveEnvFast.load (std::memory_order_relaxed));
    snapshot.envSlow = sanitizeUnitScalar (auditionReactiveEnvSlow.load (std::memory_order_relaxed));
    snapshot.onset = sanitizeUnitScalar (auditionReactiveOnset.load (std::memory_order_relaxed));
    snapshot.brightness = sanitizeUnitScalar (auditionReactiveBrightness.load (std::memory_order_relaxed));
    snapshot.rainFadeRate = sanitizeUnitScalar (auditionReactiveRainFadeRate.load (std::memory_order_relaxed));
    snapshot.snowFadeRate = sanitizeUnitScalar (auditionReactiveSnowFadeRate.load (std::memory_order_relaxed));
    snapshot.physicsVelocity = sanitizeUnitScalar (auditionReactivePhysicsVelocity.load (std::memory_order_relaxed));
    snapshot.physicsCollision = sanitizeUnitScalar (auditionReactivePhysicsCollision.load (std::memory_order_relaxed));
    snapshot.physicsDensity = sanitizeUnitScalar (auditionReactivePhysicsDensity.load (std::memory_order_relaxed));
    snapshot.physicsCoupling = sanitizeUnitScalar (auditionReactivePhysicsCoupling.load (std::memory_order_relaxed));
    snapshot.headphoneOutputRms = sanitizeUnitScalar (auditionReactiveHeadphoneOutputRms.load (std::memory_order_relaxed));
    snapshot.headphoneOutputPeak = sanitizeUnitScalar (auditionReactiveHeadphoneOutputPeak.load (std::memory_order_relaxed));
    snapshot.headphoneParity = sanitizeUnitScalar (auditionReactiveHeadphoneParity.load (std::memory_order_relaxed));
    snapshot.rmsNorm = snapshot.rms;
    snapshot.peakNorm = snapshot.peak;
    snapshot.envFastNorm = snapshot.envFast;
    snapshot.envSlowNorm = snapshot.envSlow;
    snapshot.headphoneOutputRmsNorm = snapshot.headphoneOutputRms;
    snapshot.headphoneOutputPeakNorm = snapshot.headphoneOutputPeak;
    snapshot.headphoneParityNorm = snapshot.headphoneParity;
    snapshot.headphoneFallbackReasonIndex = sanitizeHeadphoneFallbackReasonIndex (
        auditionReactiveHeadphoneFallbackReasonIndex.load (std::memory_order_relaxed));
    snapshot.sourceEnergyCount = sanitizeSourceCount (
        auditionReactiveSourceCount.load (std::memory_order_relaxed));

    for (int i = 0; i < MAX_AUDITION_REACTIVE_SOURCES; ++i)
    {
        snapshot.sourceEnergy[static_cast<size_t> (i)] = sanitizeUnitScalar (
            auditionReactiveSourceEnergy[static_cast<size_t> (i)].load (std::memory_order_relaxed));
    }

    const auto sourceDensity = sanitizeUnitScalar (
        static_cast<float> (snapshot.sourceEnergyCount)
            / static_cast<float> (juce::jmax (1, MAX_AUDITION_REACTIVE_SOURCES)));
    snapshot.geometryScale = sanitizeUnitScalar (
        0.30f * snapshot.envFast
            + 0.20f * snapshot.envSlow
            + 0.20f * snapshot.physicsCoupling
            + 0.20f * snapshot.headphoneParity
            + 0.10f * sourceDensity);
    snapshot.geometryWidth = sanitizeUnitScalar (
        0.40f * snapshot.physicsDensity
            + 0.25f * snapshot.physicsVelocity
            + 0.20f * snapshot.brightness
            + 0.15f * sourceDensity);
    snapshot.geometryDepth = sanitizeUnitScalar (
        0.35f * snapshot.envSlow
            + 0.30f * (1.0f - snapshot.brightness)
            + 0.20f * snapshot.physicsCoupling
            + 0.15f * sourceDensity);
    snapshot.geometryHeight = sanitizeUnitScalar (
        0.45f * snapshot.onset
            + 0.30f * snapshot.physicsCollision
            + 0.15f * snapshot.envFast
            + 0.10f * snapshot.headphoneOutputPeak);
    snapshot.precipitationFade = sanitizeUnitScalar (
        0.55f * snapshot.rainFadeRate
            + 0.45f * snapshot.snowFadeRate);
    snapshot.collisionBurst = sanitizeUnitScalar (
        snapshot.physicsCollision * (0.55f + 0.45f * snapshot.onset));
    snapshot.densitySpread = sanitizeUnitScalar (
        0.60f * snapshot.physicsDensity
            + 0.25f * sourceDensity
            + 0.15f * snapshot.physicsVelocity);

    return snapshot;
}

void SpatialRenderer::finalizeEmitterStageWithAuditionFallback (SpatialRenderer::EmitterStageResult& result,
                                                                int numSamples)
{
    if (result.processedEmitterCount == 0 && auditionEnabled)
    {
        renderInternalAuditionEmitter (numSamples);
        result.eligibleEmitterCount = juce::jmax (result.eligibleEmitterCount, 1);
        result.processedEmitterCount = juce::jmax (result.processedEmitterCount, 1);
        result.renderedAuditionEmitter = true;
        return;
    }

    resetAuditionReactiveTelemetry();
}

int SpatialRenderer::determineAuditionHeadphoneFallbackReason (bool renderedAuditionEmitter,
                                                               SpatialRenderer::HeadphoneRenderMode requestedHeadphoneMode,
                                                               int numOutputChannels,
                                                               bool profileAllowsHeadphoneRender,
                                                               bool steamBackendAvailable,
                                                               bool steamRenderedThisBlock,
                                                               SpatialRenderer::HeadphoneRenderMode activeHeadphoneMode) const noexcept
{
    int reason = static_cast<int> (AuditionReactiveHeadphoneFallbackReason::None);
    if (renderedAuditionEmitter && requestedHeadphoneMode == HeadphoneRenderMode::SteamBinaural)
    {
        if (numOutputChannels < 2 || ! profileAllowsHeadphoneRender)
        {
            reason = static_cast<int> (AuditionReactiveHeadphoneFallbackReason::OutputIncompatible);
        }
        else if (! steamBackendAvailable)
        {
            reason = static_cast<int> (AuditionReactiveHeadphoneFallbackReason::SteamUnavailable);
        }
        else if (! steamRenderedThisBlock || activeHeadphoneMode != HeadphoneRenderMode::SteamBinaural)
        {
            reason = static_cast<int> (AuditionReactiveHeadphoneFallbackReason::SteamRenderFailed);
        }
    }

    return reason;
}

void SpatialRenderer::accumulateAuditionHeadphoneParitySample (SpatialRenderer::AuditionHeadphoneParityAccumulator& parity,
                                                               float left,
                                                               float right,
                                                               bool referenceCaptured,
                                                               float referenceLeft,
                                                               float referenceRight) noexcept
{
    const auto mono = 0.5f * (left + right);
    parity.outputEnergy += static_cast<double> (mono * mono);
    parity.peak = juce::jmax (parity.peak, juce::jmax (std::abs (left), std::abs (right)));

    if (referenceCaptured)
    {
        const auto referenceMono = 0.5f * (referenceLeft + referenceRight);
        parity.referenceEnergy += static_cast<double> (referenceMono * referenceMono);
    }
    else
    {
        parity.referenceEnergy += static_cast<double> (mono * mono);
    }

    parity.samplesCaptured = true;
}

void SpatialRenderer::finalizeAuditionHeadphoneParity (bool renderedAuditionEmitter,
                                                       int numSamples,
                                                       const SpatialRenderer::AuditionHeadphoneParityAccumulator& parity) noexcept
{
    if (renderedAuditionEmitter && parity.samplesCaptured && numSamples > 0)
    {
        const auto invNumSamples = 1.0f / static_cast<float> (numSamples);
        const auto headphoneOutputRms = juce::jlimit (
            0.0f,
            2.0f,
            std::sqrt (static_cast<float> (parity.outputEnergy * static_cast<double> (invNumSamples))));
        const auto headphoneReferenceRms = juce::jlimit (
            0.0f,
            2.0f,
            std::sqrt (static_cast<float> (parity.referenceEnergy * static_cast<double> (invNumSamples))));
        const auto headphoneParity = headphoneOutputRms > 1.0e-6f
            ? juce::jlimit (0.5f, 2.0f, headphoneReferenceRms / headphoneOutputRms)
            : 1.0f;

        applyAuditionReactiveHeadphoneParity (
            headphoneOutputRms,
            parity.peak,
            headphoneParity,
            parity.fallbackReasonIndex);
    }
    else if (renderedAuditionEmitter)
    {
        applyAuditionReactiveHeadphoneParity (
            0.0f,
            0.0f,
            1.0f,
            parity.fallbackReasonIndex);
    }
}

SpatialRenderer::AuditionHeadphoneParityAccumulator SpatialRenderer::prepareAuditionHeadphoneParityAccumulator (
    bool renderedAuditionEmitter,
    int numOutputChannels,
    const SpatialRenderer::HeadphoneRuntimeState& headphoneState) const noexcept
{
    AuditionHeadphoneParityAccumulator parity {};
    parity.fallbackReasonIndex = determineAuditionHeadphoneFallbackReason (
        renderedAuditionEmitter,
        headphoneState.requestedMode,
        numOutputChannels,
        headphoneState.profileAllowsHeadphoneRender,
        headphoneState.steamBackendAvailable,
        headphoneState.steamRenderedThisBlock,
        headphoneState.activeMode);
    return parity;
}

void SpatialRenderer::publishAuditionHeadphoneParityForBlock (bool renderedAuditionEmitter,
                                                              int numSamples,
                                                              const SpatialRenderer::AuditionHeadphoneParityAccumulator& parity) noexcept
{
    finalizeAuditionHeadphoneParity (renderedAuditionEmitter, numSamples, parity);
}
