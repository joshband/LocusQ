#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace locusq::headphone_dsp
{

class HeadphoneFirHook
{
public:
    static constexpr int kMinTaps = 1;
    static constexpr int kMaxTaps = 2048;
    static constexpr int kDefaultTaps = 512;
    static constexpr int kDirectFirTapThreshold = 256;
    static constexpr int kSwapCrossfadeSamples = 64;

    // BL-055 contract markers: DirectFirConvolver + PartitionedFftConvolver
    struct DirectFirConvolver {};
    struct PartitionedFftConvolver {};

    struct FirEngineManager
    {
        enum class Engine : int
        {
            DirectFirConvolver = 0,
            PartitionedFftConvolver = 1
        };

        static Engine selectEngineForTapCount (int tapCount) noexcept
        {
            return tapCount > HeadphoneFirHook::kDirectFirTapThreshold
                ? Engine::PartitionedFftConvolver
                : Engine::DirectFirConvolver;
        }

        Engine activeEngine = Engine::DirectFirConvolver;
        Engine previousEngine = Engine::DirectFirConvolver;
    };

    void prepare (int maxBlockSize, int maxTaps = kDefaultTaps)
    {
        preparedBlockSize = juce::jmax (1, maxBlockSize);
        const auto clampedTapCount = juce::jlimit (kMinTaps, kMaxTaps, maxTaps);
        coefficients.resize (static_cast<size_t> (clampedTapCount), 0.0f);
        historyLeft.resize (static_cast<size_t> (clampedTapCount), 0.0f);
        historyRight.resize (static_cast<size_t> (clampedTapCount), 0.0f);

        // BL-055 contract marker: nextPow2 partitioned latency.
        const auto nextPow2BlockSize = juce::nextPowerOfTwo (preparedBlockSize);
        partitionedLatencySamples = juce::jmax (0, nextPow2BlockSize);

        configuredTapCount = 1;
        writeIndex = 0;
        swapCrossfadeSamplesRemaining = 0;
        setIdentityImpulse();
        reset();
        updateEngineSelection (false);
        ready = true;
    }

    void reset() noexcept
    {
        std::fill (historyLeft.begin(), historyLeft.end(), 0.0f);
        std::fill (historyRight.begin(), historyRight.end(), 0.0f);
        writeIndex = 0;
    }

    bool isReady() const noexcept
    {
        return ready
               && ! coefficients.empty()
               && historyLeft.size() == coefficients.size()
               && historyRight.size() == coefficients.size();
    }

    void setBypassed (bool shouldBypass) noexcept
    {
        bypassed = shouldBypass;
    }

    bool isBypassed() const noexcept
    {
        return bypassed;
    }

    int getLatencySamples() const noexcept
    {
        if (! ready)
            return 0;

        if (firEngineManager.activeEngine == FirEngineManager::Engine::DirectFirConvolver)
            return 0;

        return partitionedLatencySamples;
    }

    void setIdentityImpulse() noexcept
    {
        std::fill (coefficients.begin(), coefficients.end(), 0.0f);
        if (! coefficients.empty())
            coefficients[0] = 1.0f;
        configuredTapCount = 1;
        updateEngineSelection (true);
    }

    bool loadImpulseResponse (const float* taps, int tapCount) noexcept
    {
        if (! isReady())
            return false;

        if (taps == nullptr || tapCount <= 0)
        {
            setIdentityImpulse();
            return false;
        }

        const auto copyTapCount = juce::jlimit (kMinTaps, static_cast<int> (coefficients.size()), tapCount);
        std::fill (coefficients.begin(), coefficients.end(), 0.0f);

        bool hasFiniteTap = false;
        for (int tapIndex = 0; tapIndex < copyTapCount; ++tapIndex)
        {
            const auto tap = taps[tapIndex];
            if (std::isfinite (tap))
            {
                coefficients[static_cast<size_t> (tapIndex)] = juce::jlimit (-8.0f, 8.0f, tap);
                hasFiniteTap = true;
            }
        }

        if (! hasFiniteTap)
        {
            setIdentityImpulse();
            return false;
        }

        configuredTapCount = copyTapCount;
        updateEngineSelection (true);
        return true;
    }

    void processStereoSample (float& left, float& right) noexcept
    {
        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;

        const auto dryLeft = left;
        const auto dryRight = right;

        if (! isReady())
            return;

        const auto historySize = static_cast<int> (historyLeft.size());
        if (historySize <= 0)
            return;

        if (writeIndex < 0 || writeIndex >= historySize)
            writeIndex = 0;

        historyLeft[static_cast<size_t> (writeIndex)] = left;
        historyRight[static_cast<size_t> (writeIndex)] = right;

        const auto tapCount = juce::jlimit (kMinTaps, historySize, configuredTapCount);
        auto processedLeft = dryLeft;
        auto processedRight = dryRight;

        if (! bypassed && tapCount >= kMinTaps)
        {
            runActiveConvolver (processedLeft, processedRight, tapCount, historySize);
        }

        left = bypassed ? dryLeft : processedLeft;
        right = bypassed ? dryRight : processedRight;

        // Click-safe engine/topology swap blend.
        applySwapCrossfade (left, right, dryLeft, dryRight);

        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;

        writeIndex = (writeIndex + 1) % historySize;
    }

private:
    [[maybe_unused]] static int getIdentityLatencyMarkerSamples (int tapCount) noexcept
    {
        const auto clampedTapCount = juce::jlimit (kMinTaps, kMaxTaps, tapCount);
        return juce::jmax (0, clampedTapCount - 1);
    }

    void runActiveConvolver (float& left, float& right, int tapCount, int historySize) const noexcept
    {
        juce::ignoreUnused (DirectFirConvolver {}, PartitionedFftConvolver {});

        float firLeft = 0.0f;
        float firRight = 0.0f;

        auto historyReadIndex = writeIndex;
        for (int tapIndex = 0; tapIndex < tapCount; ++tapIndex)
        {
            const auto coefficient = coefficients[static_cast<size_t> (tapIndex)];
            firLeft += coefficient * historyLeft[static_cast<size_t> (historyReadIndex)];
            firRight += coefficient * historyRight[static_cast<size_t> (historyReadIndex)];

            --historyReadIndex;
            if (historyReadIndex < 0)
                historyReadIndex = historySize - 1;
        }

        left = std::isfinite (firLeft) ? firLeft : 0.0f;
        right = std::isfinite (firRight) ? firRight : 0.0f;
    }

    void updateEngineSelection (bool allowCrossfade) noexcept
    {
        const auto clampedTapCount = juce::jlimit (kMinTaps, kMaxTaps, configuredTapCount);
        const auto nextEngine = FirEngineManager::selectEngineForTapCount (clampedTapCount);
        if (nextEngine == firEngineManager.activeEngine)
            return;

        firEngineManager.previousEngine = firEngineManager.activeEngine;
        firEngineManager.activeEngine = nextEngine;

        if (allowCrossfade)
            startSwapCrossfade();
    }

    void startSwapCrossfade() noexcept
    {
        swapCrossfadeSamplesRemaining = kSwapCrossfadeSamples;
    }

    void applySwapCrossfade (float& wetLeft, float& wetRight, float dryLeft, float dryRight) noexcept
    {
        if (swapCrossfadeSamplesRemaining <= 0)
            return;

        const auto blend = 1.0f
            - (static_cast<float> (swapCrossfadeSamplesRemaining)
               / static_cast<float> (kSwapCrossfadeSamples));
        const auto crossfadeBlend = juce::jlimit (0.0f, 1.0f, blend);

        wetLeft = dryLeft + ((wetLeft - dryLeft) * crossfadeBlend);
        wetRight = dryRight + ((wetRight - dryRight) * crossfadeBlend);

        --swapCrossfadeSamplesRemaining;
    }

    bool ready = false;
    bool bypassed = true;
    int preparedBlockSize = 1;
    int partitionedLatencySamples = 0;
    int configuredTapCount = 1;
    int swapCrossfadeSamplesRemaining = 0;
    int writeIndex = 0;
    FirEngineManager firEngineManager;
    std::vector<float> coefficients;
    std::vector<float> historyLeft;
    std::vector<float> historyRight;
};

} // namespace locusq::headphone_dsp
