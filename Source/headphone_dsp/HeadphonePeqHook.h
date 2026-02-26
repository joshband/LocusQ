#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>

namespace locusq::headphone_dsp
{

class HeadphonePeqHook
{
public:
    static constexpr int kMaxStages = 4;

    struct Coefficients
    {
        float b0 = 1.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
        bool active = false;
    };

    void prepare (double sampleRate) noexcept
    {
        ready = std::isfinite (sampleRate) && sampleRate > 0.0;
        setIdentityCurve();
    }

    void reset() noexcept
    {
        for (auto& stage : stages)
        {
            stage.z1Left = 0.0f;
            stage.z2Left = 0.0f;
            stage.z1Right = 0.0f;
            stage.z2Right = 0.0f;
        }
    }

    bool isReady() const noexcept
    {
        return ready;
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
        return 0;
    }

    void setIdentityCurve() noexcept
    {
        for (auto& stage : stages)
        {
            stage.coefficients = Coefficients {};
            stage.z1Left = 0.0f;
            stage.z2Left = 0.0f;
            stage.z1Right = 0.0f;
            stage.z2Right = 0.0f;
        }
    }

    void setStageCoefficients (int stageIndex, const Coefficients& coefficients) noexcept
    {
        if (stageIndex < 0 || stageIndex >= kMaxStages)
            return;

        stages[static_cast<size_t> (stageIndex)].coefficients = sanitizeCoefficients (coefficients);
    }

    void processStereoSample (float& left, float& right) noexcept
    {
        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;

        if (! ready || bypassed)
            return;

        for (auto& stage : stages)
        {
            if (! stage.coefficients.active)
                continue;

            left = processBiquadSample (left, stage.coefficients, stage.z1Left, stage.z2Left);
            right = processBiquadSample (right, stage.coefficients, stage.z1Right, stage.z2Right);
        }

        if (! std::isfinite (left))
            left = 0.0f;
        if (! std::isfinite (right))
            right = 0.0f;
    }

private:
    struct StageState
    {
        Coefficients coefficients {};
        float z1Left = 0.0f;
        float z2Left = 0.0f;
        float z1Right = 0.0f;
        float z2Right = 0.0f;
    };

    static float sanitizeFinite (float value, float fallback = 0.0f) noexcept
    {
        return std::isfinite (value) ? value : fallback;
    }

    static Coefficients sanitizeCoefficients (const Coefficients& coefficients) noexcept
    {
        Coefficients sanitized {};
        sanitized.b0 = juce::jlimit (-8.0f, 8.0f, sanitizeFinite (coefficients.b0, 1.0f));
        sanitized.b1 = juce::jlimit (-8.0f, 8.0f, sanitizeFinite (coefficients.b1));
        sanitized.b2 = juce::jlimit (-8.0f, 8.0f, sanitizeFinite (coefficients.b2));
        sanitized.a1 = juce::jlimit (-1.9995f, 1.9995f, sanitizeFinite (coefficients.a1));
        sanitized.a2 = juce::jlimit (-1.9995f, 1.9995f, sanitizeFinite (coefficients.a2));
        sanitized.active = coefficients.active;
        return sanitized;
    }

    static float processBiquadSample (
        float input,
        const Coefficients& coefficients,
        float& z1,
        float& z2) noexcept
    {
        const auto output = (coefficients.b0 * input) + z1;
        z1 = (coefficients.b1 * input) - (coefficients.a1 * output) + z2;
        z2 = (coefficients.b2 * input) - (coefficients.a2 * output);

        if (! std::isfinite (output)
            || ! std::isfinite (z1)
            || ! std::isfinite (z2))
        {
            z1 = 0.0f;
            z2 = 0.0f;
            return 0.0f;
        }

        return output;
    }

    bool ready = false;
    bool bypassed = true;
    std::array<StageState, kMaxStages> stages {};
};

} // namespace locusq::headphone_dsp
