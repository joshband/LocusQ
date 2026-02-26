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

    void prepare (int maxBlockSize, int maxTaps = kDefaultTaps)
    {
        juce::ignoreUnused (maxBlockSize);

        const auto clampedTapCount = juce::jlimit (kMinTaps, kMaxTaps, maxTaps);
        coefficients.resize (static_cast<size_t> (clampedTapCount), 0.0f);
        historyLeft.resize (static_cast<size_t> (clampedTapCount), 0.0f);
        historyRight.resize (static_cast<size_t> (clampedTapCount), 0.0f);

        configuredTapCount = 1;
        writeIndex = 0;
        setIdentityImpulse();
        reset();
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

        const auto clampedTapCount = juce::jlimit (kMinTaps, kMaxTaps, configuredTapCount);
        return juce::jmax (0, clampedTapCount - 1);
    }

    void setIdentityImpulse() noexcept
    {
        std::fill (coefficients.begin(), coefficients.end(), 0.0f);
        if (! coefficients.empty())
            coefficients[0] = 1.0f;
        configuredTapCount = 1;
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
        return true;
    }

    void processStereoSample (float& left, float& right) noexcept
    {
        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;

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
        if (! bypassed && tapCount >= kMinTaps)
        {
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

        writeIndex = (writeIndex + 1) % historySize;
    }

private:
    bool ready = false;
    bool bypassed = true;
    int configuredTapCount = 1;
    int writeIndex = 0;
    std::vector<float> coefficients;
    std::vector<float> historyLeft;
    std::vector<float> historyRight;
};

} // namespace locusq::headphone_dsp
