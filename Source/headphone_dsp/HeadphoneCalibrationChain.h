#pragma once

#include "../headphone_core/HeadphoneCalibrationChainState.h"
#include "HeadphoneFirHook.h"
#include "HeadphonePeqHook.h"

#include <cmath>

namespace locusq::headphone_dsp
{

class HeadphoneCalibrationChain
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        prepared = std::isfinite (sampleRate)
                   && sampleRate > 0.0
                   && maxBlockSize > 0;

        peqHook.prepare (sampleRate);
        firHook.prepare (maxBlockSize);
        peqHook.setIdentityCurve();
        firHook.setIdentityImpulse();
        reset();
        updateResolvedState();
    }

    void reset() noexcept
    {
        peqHook.reset();
        firHook.reset();
    }

    void setEnabled (bool enabled) noexcept
    {
        if (request.enabled == enabled)
            return;

        request.enabled = enabled;
        updateResolvedState();
    }

    void setRequestedEngineIndex (int engineIndex) noexcept
    {
        if (request.requestedEngineIndex == engineIndex)
            return;

        request.requestedEngineIndex = engineIndex;
        updateResolvedState();
    }

    int getRequestedEngineIndex() const noexcept
    {
        return resolved.requestedEngineIndex;
    }

    int getActiveEngineIndex() const noexcept
    {
        return resolved.activeEngineIndex;
    }

    int getFallbackReasonIndex() const noexcept
    {
        return resolved.fallbackReasonIndex;
    }

    int getActiveLatencySamples() const noexcept
    {
        return resolved.activeLatencySamples;
    }

    void processStereoSample (float& left, float& right) noexcept
    {
        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;

        switch (static_cast<headphone_core::CalibrationChainEngine> (resolved.activeEngineIndex))
        {
            case headphone_core::CalibrationChainEngine::ParametricEq:
                peqHook.processStereoSample (left, right);
                break;
            case headphone_core::CalibrationChainEngine::FirConvolution:
                firHook.processStereoSample (left, right);
                break;
            case headphone_core::CalibrationChainEngine::Disabled:
            default:
                break;
        }

        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;
    }

private:
    void updateResolvedState() noexcept
    {
        resolved = headphone_core::resolveCalibrationChainState (
            request,
            prepared,
            peqHook.isReady(),
            firHook.isReady(),
            peqHook.getLatencySamples(),
            firHook.getLatencySamples());

        const auto activeEngine = static_cast<headphone_core::CalibrationChainEngine> (resolved.activeEngineIndex);
        peqHook.setBypassed (activeEngine != headphone_core::CalibrationChainEngine::ParametricEq);
        firHook.setBypassed (activeEngine != headphone_core::CalibrationChainEngine::FirConvolution);
    }

    bool prepared = false;
    headphone_core::CalibrationChainRequest request {};
    headphone_core::CalibrationChainResolvedState resolved {};
    HeadphonePeqHook peqHook;
    HeadphoneFirHook firHook;
};

} // namespace locusq::headphone_dsp
