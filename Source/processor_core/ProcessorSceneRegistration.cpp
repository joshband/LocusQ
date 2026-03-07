#include "../PluginProcessor.h"

#include <cmath>

namespace
{
using RegistrationContractOperation = locusq::shared_contracts::registration_lock_free::Operation;
using RegistrationContractOutcome = locusq::shared_contracts::registration_lock_free::Outcome;

RegistrationTransitionStage registrationStageFromContractOutcome (
    RegistrationContractOutcome outcome) noexcept
{
    switch (outcome)
    {
        case RegistrationContractOutcome::Success:
        case RegistrationContractOutcome::Noop:
            return RegistrationTransitionStage::Stable;
        case RegistrationContractOutcome::Contention:
            return RegistrationTransitionStage::ClaimConflict;
        case RegistrationContractOutcome::StateDrift:
            return RegistrationTransitionStage::Recovered;
        case RegistrationContractOutcome::ReleaseIncomplete:
            return RegistrationTransitionStage::Ambiguous;
        default:
            break;
    }

    return RegistrationTransitionStage::Stable;
}

RegistrationTransitionFallbackReason registrationFallbackFromContractStep (
    RegistrationContractOperation operation,
    RegistrationContractOutcome outcome) noexcept
{
    switch (outcome)
    {
        case RegistrationContractOutcome::Contention:
            if (operation == RegistrationContractOperation::ClaimRenderer)
                return RegistrationTransitionFallbackReason::RendererAlreadyClaimed;
            return RegistrationTransitionFallbackReason::EmitterSlotUnavailable;

        case RegistrationContractOutcome::StateDrift:
            if (operation == RegistrationContractOperation::ReleaseEmitter)
                return RegistrationTransitionFallbackReason::StaleEmitterOwner;
            if (operation == RegistrationContractOperation::ReleaseRenderer)
                return RegistrationTransitionFallbackReason::RendererStateDrift;
            return RegistrationTransitionFallbackReason::RendererStateDrift;

        case RegistrationContractOutcome::ReleaseIncomplete:
            return RegistrationTransitionFallbackReason::ReleaseIncomplete;

        case RegistrationContractOutcome::Success:
        case RegistrationContractOutcome::Noop:
        default:
            break;
    }

    return RegistrationTransitionFallbackReason::None;
}
} // namespace

void LocusQAudioProcessor::syncSceneGraphRegistrationForMode (LocusQMode mode)
{
    auto stage = RegistrationTransitionStage::Stable;
    auto fallback = RegistrationTransitionFallbackReason::None;
    bool transitionAmbiguityObserved = false;
    bool staleOwnerRecovered = false;
    bool releaseIncomplete = false;

    auto applyContractStep = [&] (RegistrationContractOperation operation,
                                  RegistrationContractOutcome outcome)
    {
        registrationClaimReleaseDiagnostics.lastOperationCode.store (
            static_cast<int> (operation),
            std::memory_order_relaxed);
        registrationClaimReleaseDiagnostics.lastOutcomeCode.store (
            static_cast<int> (outcome),
            std::memory_order_relaxed);
        registrationClaimReleaseDiagnostics.seq.fetch_add (1, std::memory_order_release);

        if (locusq::shared_contracts::registration_lock_free::isContention (outcome))
        {
            registrationClaimReleaseDiagnostics.contentionCount.fetch_add (1, std::memory_order_relaxed);
        }
        if (outcome == RegistrationContractOutcome::ReleaseIncomplete)
        {
            registrationClaimReleaseDiagnostics.releaseIncompleteCount.fetch_add (1, std::memory_order_relaxed);
        }

        if (outcome == RegistrationContractOutcome::Success
            || outcome == RegistrationContractOutcome::Noop)
            return;

        stage = registrationStageFromContractOutcome (outcome);
        fallback = registrationFallbackFromContractStep (operation, outcome);

        if (stage == RegistrationTransitionStage::Recovered
            || outcome == RegistrationContractOutcome::StateDrift)
            staleOwnerRecovered = true;

        if (stage != RegistrationTransitionStage::Stable)
            transitionAmbiguityObserved = true;

        if (stage == RegistrationTransitionStage::Ambiguous
            || outcome == RegistrationContractOutcome::ReleaseIncomplete)
            releaseIncomplete = true;
    };

    auto releaseEmitter = [&]() -> RegistrationContractOutcome
    {
        if (emitterSlotId < 0)
            return RegistrationContractOutcome::Noop;

        const int slotToRelease = emitterSlotId;
        sceneGraph.unregisterEmitter (slotToRelease);
        const bool stillActive = sceneGraph.isSlotActive (slotToRelease);
        emitterSlotId = -1;
        lastPhysThrowGate = false;
        lastPhysResetGate = false;
        return stillActive ? RegistrationContractOutcome::ReleaseIncomplete
                           : RegistrationContractOutcome::Success;
    };

    auto releaseRenderer = [&]() -> RegistrationContractOutcome
    {
        if (! rendererRegistered)
            return RegistrationContractOutcome::Noop;

        sceneGraph.unregisterRenderer();
        sceneGraph.setPhysicsInteractionEnabled (false);
        const bool stillRegistered = sceneGraph.isRendererRegistered();
        rendererRegistered = false;
        return stillRegistered ? RegistrationContractOutcome::ReleaseIncomplete
                               : RegistrationContractOutcome::Success;
    };

    auto claimEmitter = [&]() -> RegistrationContractOutcome
    {
        if (emitterSlotId >= 0)
            return RegistrationContractOutcome::Noop;

        const int claimedSlot = sceneGraph.registerEmitter (sceneGraphAudioReservationId);
        if (claimedSlot < 0)
            return RegistrationContractOutcome::Contention;

        emitterSlotId = claimedSlot;
        DBG ("LocusQ: Registered emitter, slot " + juce::String (emitterSlotId));

        const auto seededColor = static_cast<int> (sceneGraph.getSlot (emitterSlotId).read().colorIndex);
        const auto currentColor = juce::jlimit (
            0,
            15,
            static_cast<int> (std::lround (apvts.getRawParameterValue ("emit_color")->load())));

        bool shouldSeedInitialColor = true;
#if LOCUSQ_CLAP_PROPERTIES_AVAILABLE
        // CLAP validator compares parameter values before init vs after first process.
        // Avoid host-visible parameter mutation during CLAP activation.
        shouldSeedInitialColor = ! is_clap;
#endif

        if (shouldSeedInitialColor
            && ! hasSeededInitialEmitterColor
            && ! hasRestoredSnapshotState
            && currentColor == 0)
        {
            setIntegerParameterValueNotifyingHost ("emit_color", seededColor);
        }

        hasSeededInitialEmitterColor = true;

        juce::String restoredLabel { "Emitter" };
        if (const auto labelSnapshot = emitterLabelRtState.load())
            restoredLabel = sanitiseEmitterLabel (*labelSnapshot);
        applyEmitterLabelToSceneSlotIfAvailable (restoredLabel);
        return RegistrationContractOutcome::Success;
    };

    auto claimRenderer = [&]() -> RegistrationContractOutcome
    {
        if (rendererRegistered)
            return RegistrationContractOutcome::Noop;

        rendererRegistered = sceneGraph.registerRenderer();
        DBG ("LocusQ: Registered renderer: " + juce::String (rendererRegistered ? "OK" : "FAILED (already exists)"));
        return rendererRegistered ? RegistrationContractOutcome::Success
                                  : RegistrationContractOutcome::Contention;
    };

    if (mode != LocusQMode::Emitter)
        applyContractStep (RegistrationContractOperation::ReleaseEmitter, releaseEmitter());
    if (mode != LocusQMode::Renderer)
        applyContractStep (RegistrationContractOperation::ReleaseRenderer, releaseRenderer());

    if (mode == LocusQMode::Emitter)
        applyContractStep (RegistrationContractOperation::ClaimEmitter, claimEmitter());
    else if (mode == LocusQMode::Renderer)
        applyContractStep (RegistrationContractOperation::ClaimRenderer, claimRenderer());

    bool emitterOwned = emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId);
    bool rendererOwned = rendererRegistered && sceneGraph.isRendererRegistered();

    if (mode == LocusQMode::Emitter && emitterSlotId >= 0 && ! emitterOwned)
    {
        const auto releaseOutcome = releaseEmitter();
        applyContractStep (RegistrationContractOperation::ReleaseEmitter,
                           releaseOutcome == RegistrationContractOutcome::ReleaseIncomplete
                               ? RegistrationContractOutcome::ReleaseIncomplete
                               : RegistrationContractOutcome::StateDrift);
        emitterOwned = false;
    }

    if (mode == LocusQMode::Renderer && rendererRegistered && ! rendererOwned)
    {
        rendererRegistered = false;
        const auto reclaimOutcome = claimRenderer();
        applyContractStep (RegistrationContractOperation::ClaimRenderer,
                           reclaimOutcome == RegistrationContractOutcome::Success
                               ? RegistrationContractOutcome::StateDrift
                               : reclaimOutcome);
        rendererOwned = rendererRegistered && sceneGraph.isRendererRegistered();
    }

    if (emitterOwned && rendererOwned)
    {
        transitionAmbiguityObserved = true;
        stage = RegistrationTransitionStage::Recovered;
        fallback = RegistrationTransitionFallbackReason::DualOwnershipResolved;

        if (mode == LocusQMode::Emitter)
        {
            const auto releaseOutcome = releaseRenderer();
            applyContractStep (RegistrationContractOperation::ReleaseRenderer, releaseOutcome);
            rendererOwned = rendererRegistered && sceneGraph.isRendererRegistered();
        }
        else
        {
            const auto releaseOutcome = releaseEmitter();
            applyContractStep (RegistrationContractOperation::ReleaseEmitter, releaseOutcome);
            emitterOwned = emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId);
        }
    }

    if (mode == LocusQMode::Calibrate && (emitterOwned || rendererOwned))
    {
        stage = RegistrationTransitionStage::Ambiguous;
        fallback = RegistrationTransitionFallbackReason::ReleaseIncomplete;
        transitionAmbiguityObserved = true;
        releaseIncomplete = true;
    }

    if (staleOwnerRecovered)
        registrationTransitionDiagnostics.staleOwnerCount.fetch_add (1, std::memory_order_relaxed);

    if (transitionAmbiguityObserved || releaseIncomplete || stage == RegistrationTransitionStage::Ambiguous)
        registrationTransitionDiagnostics.ambiguityCount.fetch_add (1, std::memory_order_relaxed);

    registrationTransitionDiagnostics.requestedMode.store (static_cast<int> (mode), std::memory_order_relaxed);
    registrationTransitionDiagnostics.stageCode.store (static_cast<int> (stage), std::memory_order_relaxed);
    registrationTransitionDiagnostics.fallbackCode.store (static_cast<int> (fallback), std::memory_order_relaxed);
    registrationTransitionDiagnostics.emitterSlot.store (emitterSlotId, std::memory_order_relaxed);
    registrationTransitionDiagnostics.emitterActive.store (
        emitterSlotId >= 0 && sceneGraph.isSlotActive (emitterSlotId),
        std::memory_order_relaxed);
    registrationTransitionDiagnostics.rendererOwned.store (
        rendererRegistered && sceneGraph.isRendererRegistered(),
        std::memory_order_relaxed);
    registrationTransitionDiagnostics.seq.fetch_add (1, std::memory_order_release);
}
