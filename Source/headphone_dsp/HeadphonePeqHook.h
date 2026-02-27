#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>

namespace locusq::headphone_dsp
{

class HeadphonePeqHook
{
public:
    static constexpr int kMaxStages = 8;

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

    void setPreampDb (float db) noexcept
    {
        preampLinear = std::pow (10.0f, db / 20.0f);
    }

    void setIdentityCurve() noexcept
    {
        preampLinear = 1.0f;

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

        left  *= preampLinear;
        right *= preampLinear;

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

    static Coefficients makePeakEQ (float fc, float gainDb, float q, float sampleRate) noexcept
    {
        const float A     = std::sqrt (std::pow (10.0f, gainDb / 40.0f));
        const float w0    = 2.0f * 3.14159265f * fc / sampleRate;
        const float alpha = std::sin (w0) / (2.0f * q);
        const float a0inv = 1.0f / (1.0f + alpha / A);
        Coefficients c;
        c.b0 = (1.0f + alpha * A) * a0inv;
        c.b1 = -2.0f * std::cos (w0) * a0inv;
        c.b2 = (1.0f - alpha * A) * a0inv;
        c.a1 = c.b1;
        c.a2 = (1.0f - alpha / A) * a0inv;
        c.active = true;
        return c;
    }

    static Coefficients makeLowShelf (float fc, float gainDb, float q, float sampleRate) noexcept
    {
        const float A    = std::pow (10.0f, gainDb / 40.0f);
        const float w0   = 2.0f * 3.14159265f * fc / sampleRate;
        const float cosw = std::cos (w0);
        const float sinw = std::sin (w0);
        const float alpha = sinw / (2.0f * q);
        const float sqA  = std::sqrt (A);
        const float a0inv = 1.0f / ((A + 1.0f) + (A - 1.0f) * cosw + 2.0f * sqA * alpha);
        Coefficients c;
        c.b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + 2.0f * sqA * alpha) * a0inv;
        c.b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw) * a0inv;
        c.b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - 2.0f * sqA * alpha) * a0inv;
        c.a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw) * a0inv;
        c.a2 = ((A + 1.0f) + (A - 1.0f) * cosw - 2.0f * sqA * alpha) * a0inv;
        c.active = true;
        return c;
    }

    static Coefficients makeHighShelf (float fc, float gainDb, float q, float sampleRate) noexcept
    {
        const float A    = std::pow (10.0f, gainDb / 40.0f);
        const float w0   = 2.0f * 3.14159265f * fc / sampleRate;
        const float cosw = std::cos (w0);
        const float sinw = std::sin (w0);
        const float alpha = sinw / (2.0f * q);
        const float sqA  = std::sqrt (A);
        const float a0inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw + 2.0f * sqA * alpha);
        Coefficients c;
        c.b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw + 2.0f * sqA * alpha) * a0inv;
        c.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw) * a0inv;
        c.b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw - 2.0f * sqA * alpha) * a0inv;
        c.a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw) * a0inv;
        c.a2 = ((A + 1.0f) - (A - 1.0f) * cosw - 2.0f * sqA * alpha) * a0inv;
        c.active = true;
        return c;
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
    float preampLinear = 1.0f;
    std::array<StageState, kMaxStages> stages {};
};

} // namespace locusq::headphone_dsp
