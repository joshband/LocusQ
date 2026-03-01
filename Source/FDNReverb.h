#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <vector>

//==============================================================================
/**
 * FDNReverb
 *
 * Late-reverb feedback delay network for quad output.
 *
 * Draft quality: 4-line static FDN (legacy-compatible CPU profile).
 * Final quality: 8-line modulated FDN with higher diffusion.
 *
 * Real-time safety:
 * - No allocation in process().
 * - Delay storage is allocated once in prepare() and reused.
 * - Modulation is deterministic (fixed per-line phases/rates, no RNG).
 */
class FDNReverb
{
public:
    static constexpr int NUM_CHANNELS = 4;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        currentSampleRate = juce::jmax (1.0, sampleRate);

        for (auto& line : delayLines)
        {
            if (static_cast<int> (line.size()) != MAX_DELAY_SAMPLES)
                line.assign (static_cast<size_t> (MAX_DELAY_SAMPLES), 0.0f);
            else
                std::fill (line.begin(), line.end(), 0.0f);
        }

        configureDelayLengths();
        updateCoefficients();
        resetModulationPhases();
        reset();
    }

    void reset()
    {
        for (auto& line : delayLines)
            std::fill (line.begin(), line.end(), 0.0f);

        for (auto& pos : writePos)
            pos = 0;

        for (auto& state : dampingState)
            state = 0.0f;

        resetModulationPhases();
    }

    void setEnabled (bool shouldEnable)        { enabled = shouldEnable; }
    void setMix (float newMix)                 { mix = juce::jlimit (0.0f, 1.0f, newMix); }
    void setRoomSize (float newRoomSize)
    {
        const auto clamped = juce::jlimit (ROOM_SIZE_MIN, ROOM_SIZE_MAX, newRoomSize);
        if (std::abs (roomSize - clamped) < 1.0e-6f)
            return;

        roomSize = clamped;
        configureDelayLengths();
        updateCoefficients();
    }

    void setDamping (float newDamping)
    {
        const auto clamped = juce::jlimit (0.0f, 1.0f, newDamping);
        if (std::abs (damping - clamped) < 1.0e-6f)
            return;

        damping = clamped;
        updateCoefficients();
    }

    void setHighQuality (bool highQuality)
    {
        if (qualityHigh == highQuality)
            return;

        qualityHigh = highQuality;
        configureDelayLengths();
        updateCoefficients();
        resetModulationPhases();
    }

    void setEarlyReflectionsOnly (bool onlyER) { earlyReflectionsOnly = onlyER; }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled || earlyReflectionsOnly || mix <= 0.0f)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = juce::jmin (NUM_CHANNELS, buffer.getNumChannels());
        if (numChannels < NUM_CHANNELS)
            return;

        const float dryMix = 1.0f - mix;
        const int activeLines = getActiveLineCount();

        for (int i = 0; i < numSamples; ++i)
        {
            std::array<float, NUM_CHANNELS> dry {};
            std::array<float, NUM_LINES> delayed {};
            std::array<float, NUM_LINES> lineInput {};

            for (int ch = 0; ch < NUM_CHANNELS; ++ch)
                dry[static_cast<size_t> (ch)] = buffer.getSample (ch, i);

            // Deterministic input projection (4ch -> N lines).
            lineInput[0] = dry[0];
            lineInput[1] = dry[1];
            lineInput[2] = dry[2];
            lineInput[3] = dry[3];
            if (activeLines == NUM_LINES)
            {
                constexpr float kNorm = 0.70710678f;
                lineInput[4] = (dry[0] + dry[2]) * kNorm;
                lineInput[5] = (dry[1] + dry[3]) * kNorm;
                lineInput[6] = (dry[0] - dry[2]) * kNorm;
                lineInput[7] = (dry[1] - dry[3]) * kNorm;
            }

            for (int lineIdx = 0; lineIdx < activeLines; ++lineIdx)
                delayed[static_cast<size_t> (lineIdx)] = readDelaySample (lineIdx);

            std::array<float, NUM_CHANNELS> wet {};
            if (activeLines == NUM_LINES)
            {
                const auto mixed = hadamard8 (delayed);

                for (int lineIdx = 0; lineIdx < NUM_LINES; ++lineIdx)
                {
                    const auto idx = static_cast<size_t> (lineIdx);
                    const float damped = dampingState[idx]
                                       + dampingCoefficient * (mixed[idx] - dampingState[idx]);
                    dampingState[idx] = damped;

                    const float writeSample = lineInput[idx] * inputInjectionGain
                                            + damped * feedbackGain[idx];
                    writeDelaySample (lineIdx, writeSample);
                    advanceLineState (lineIdx);
                }

                wet[0] = 0.5f * (delayed[0] + delayed[4]);
                wet[1] = 0.5f * (delayed[1] + delayed[5]);
                wet[2] = 0.5f * (delayed[2] + delayed[6]);
                wet[3] = 0.5f * (delayed[3] + delayed[7]);
            }
            else
            {
                const auto mixed = hadamard4 (delayed);
                for (int lineIdx = 0; lineIdx < NUM_CHANNELS; ++lineIdx)
                {
                    const auto idx = static_cast<size_t> (lineIdx);
                    const float damped = dampingState[idx]
                                       + dampingCoefficient * (mixed[idx] - dampingState[idx]);
                    dampingState[idx] = damped;

                    const float writeSample = lineInput[idx] * inputInjectionGain
                                            + damped * feedbackGain[idx];
                    writeDelaySample (lineIdx, writeSample);
                    advanceLineState (lineIdx);
                    wet[idx] = delayed[idx];
                }
            }

            for (int ch = 0; ch < NUM_CHANNELS; ++ch)
            {
                const auto idx = static_cast<size_t> (ch);
                const float wetSample = std::isfinite (wet[idx]) ? wet[idx] : 0.0f;
                buffer.setSample (ch, i, dry[idx] * dryMix + wetSample * mix);
            }
        }
    }

private:
    static constexpr int NUM_LINES = 8;
    // Reference sample rate for which the base delay sample counts below are calibrated.
    static constexpr double REFERENCE_SAMPLE_RATE = 44100.0;
    // Buffer sized for finalBaseDelays[7] * ROOM_SIZE_MAX at 192 kHz:
    //   3989 * (192000/44100) * 5.0 ≈ 86837 → next power-of-two = 131072.
    static constexpr int MAX_DELAY_SAMPLES = 131072;
    static constexpr int MIN_DELAY_SAMPLES = 64;
    static constexpr float ROOM_SIZE_MIN = 0.5f;
    static constexpr float ROOM_SIZE_MAX = 5.0f;
    // Modulation depth cap at REFERENCE_SAMPLE_RATE; scaled by srScale in updateCoefficients().
    static constexpr float MAX_MOD_DEPTH_SAMPLES_REF = 48.0f;

    static std::array<float, NUM_LINES> hadamard8 (const std::array<float, NUM_LINES>& input) noexcept
    {
        auto output = input;

        for (int stride = 1; stride < NUM_LINES; stride <<= 1)
        {
            for (int base = 0; base < NUM_LINES; base += (stride << 1))
            {
                for (int i = 0; i < stride; ++i)
                {
                    const float a = output[static_cast<size_t> (base + i)];
                    const float b = output[static_cast<size_t> (base + i + stride)];
                    output[static_cast<size_t> (base + i)] = a + b;
                    output[static_cast<size_t> (base + i + stride)] = a - b;
                }
            }
        }

        constexpr float kNorm = 0.35355339f; // 1/sqrt(8)
        for (auto& v : output)
            v *= kNorm;

        return output;
    }

    static std::array<float, NUM_CHANNELS> hadamard4 (const std::array<float, NUM_LINES>& input) noexcept
    {
        const float x0 = input[0];
        const float x1 = input[1];
        const float x2 = input[2];
        const float x3 = input[3];

        return {
            0.5f * (x0 + x1 + x2 + x3),
            0.5f * (x0 - x1 + x2 - x3),
            0.5f * (x0 + x1 - x2 - x3),
            0.5f * (x0 - x1 - x2 + x3)
        };
    }

    int getActiveLineCount() const noexcept
    {
        return qualityHigh ? NUM_LINES : NUM_CHANNELS;
    }

    float getRoomSizeNormalized() const noexcept
    {
        return juce::jlimit (0.0f,
                             1.0f,
                             (roomSize - ROOM_SIZE_MIN) / (ROOM_SIZE_MAX - ROOM_SIZE_MIN));
    }

    void configureDelayLengths()
    {
        // Base delay lengths in samples at REFERENCE_SAMPLE_RATE (44100 Hz).
        // Multiplied by srScale = currentSampleRate / REFERENCE_SAMPLE_RATE so that
        // delay times in milliseconds remain constant across all sample rates.
        static constexpr std::array<int, NUM_CHANNELS> draftBaseDelays {
            1499, 1877, 2137, 2557
        };
        static constexpr std::array<int, NUM_LINES> finalBaseDelays {
            1423, 1777, 2137, 2557, 2879, 3251, 3623, 3989
        };

        const int activeLines = getActiveLineCount();
        const float srScale = static_cast<float> (currentSampleRate / REFERENCE_SAMPLE_RATE);

        for (int lineIdx = 0; lineIdx < NUM_LINES; ++lineIdx)
        {
            int baseDelay = MIN_DELAY_SAMPLES;

            if (lineIdx < activeLines)
            {
                if (qualityHigh)
                    baseDelay = finalBaseDelays[static_cast<size_t> (lineIdx)];
                else
                    baseDelay = draftBaseDelays[static_cast<size_t> (lineIdx)];
            }

            const auto scaledDelay = static_cast<int> (
                std::lround (static_cast<float> (baseDelay) * roomSize * srScale));
            delaySamples[static_cast<size_t> (lineIdx)] = juce::jlimit (
                MIN_DELAY_SAMPLES,
                MAX_DELAY_SAMPLES - 2,
                scaledDelay);
        }
    }

    void updateCoefficients()
    {
        static constexpr std::array<float, NUM_LINES> finalModRatesHz {
            0.071f, 0.089f, 0.103f, 0.127f,
            0.149f, 0.167f, 0.191f, 0.223f
        };

        const float roomNorm = getRoomSizeNormalized();
        const float baseRt60 = qualityHigh
                             ? (1.6f + roomNorm * 4.6f)
                             : (0.9f + roomNorm * 2.4f);
        const float dampingRtScale = 1.0f - (damping * 0.45f);
        const float targetRt60 = juce::jmax (0.25f, baseRt60 * dampingRtScale);

        dampingCoefficient = juce::jlimit (0.08f, 0.92f, 0.82f - damping * 0.64f);
        inputInjectionGain = qualityHigh ? 0.42f : 0.58f;

        const int activeLines = getActiveLineCount();
        for (int lineIdx = 0; lineIdx < NUM_LINES; ++lineIdx)
        {
            const auto idx = static_cast<size_t> (lineIdx);
            if (lineIdx >= activeLines)
            {
                feedbackGain[idx] = 0.0f;
                lfoIncrement[idx] = 0.0f;
                modDepthSamples[idx] = 0.0f;
                dampingState[idx] = 0.0f;
                continue;
            }

            const float delaySeconds = static_cast<float> (delaySamples[idx]) / static_cast<float> (currentSampleRate);
            float feedback = std::pow (10.0f, (-3.0f * delaySeconds) / targetRt60);
            feedback = juce::jlimit (0.15f, 0.985f, feedback);
            feedbackGain[idx] = feedback;

            float modDepth = 0.0f;
            float modRateHz = 0.0f;
            if (qualityHigh)
            {
                const float srScale = static_cast<float> (currentSampleRate / REFERENCE_SAMPLE_RATE);
                modRateHz = finalModRatesHz[idx];
                modDepth = (2.0f + roomNorm * 18.0f) * (1.0f - damping * 0.65f) * srScale;
            }

            const float maxModDepth = MAX_MOD_DEPTH_SAMPLES_REF
                                    * static_cast<float> (currentSampleRate / REFERENCE_SAMPLE_RATE);
            modDepthSamples[idx] = juce::jlimit (0.0f, maxModDepth, modDepth);
            lfoIncrement[idx] = juce::MathConstants<float>::twoPi
                              * modRateHz
                              / static_cast<float> (currentSampleRate);
        }
    }

    void resetModulationPhases() noexcept
    {
        for (int lineIdx = 0; lineIdx < NUM_LINES; ++lineIdx)
            lfoPhase[static_cast<size_t> (lineIdx)] = 0.53125f * static_cast<float> (lineIdx + 1);
    }

    float readDelaySample (int lineIdx) const noexcept
    {
        const auto idx = static_cast<size_t> (lineIdx);
        const auto& line = delayLines[idx];
        if (line.empty())
            return 0.0f;

        float delayWithMod = static_cast<float> (delaySamples[idx]);
        if (modDepthSamples[idx] > 0.0f)
            delayWithMod += std::sin (lfoPhase[idx]) * modDepthSamples[idx];

        delayWithMod = juce::jlimit (static_cast<float> (MIN_DELAY_SAMPLES),
                                     static_cast<float> (MAX_DELAY_SAMPLES - 2),
                                     delayWithMod);

        float readPos = static_cast<float> (writePos[idx]) - delayWithMod;
        while (readPos < 0.0f)
            readPos += static_cast<float> (MAX_DELAY_SAMPLES);

        const int indexA = static_cast<int> (readPos);
        const int indexB = (indexA + 1 < MAX_DELAY_SAMPLES) ? (indexA + 1) : 0;
        const float frac = readPos - static_cast<float> (indexA);
        const float a = line[static_cast<size_t> (indexA)];
        const float b = line[static_cast<size_t> (indexB)];
        return a + (b - a) * frac;
    }

    void writeDelaySample (int lineIdx, float sample) noexcept
    {
        const auto idx = static_cast<size_t> (lineIdx);
        delayLines[idx][static_cast<size_t> (writePos[idx])] = std::isfinite (sample) ? sample : 0.0f;
    }

    void advanceLineState (int lineIdx) noexcept
    {
        const auto idx = static_cast<size_t> (lineIdx);

        ++writePos[idx];
        if (writePos[idx] >= MAX_DELAY_SAMPLES)
            writePos[idx] = 0;

        lfoPhase[idx] += lfoIncrement[idx];
        if (lfoPhase[idx] >= juce::MathConstants<float>::twoPi)
            lfoPhase[idx] -= juce::MathConstants<float>::twoPi;
    }

    double currentSampleRate = 44100.0;
    bool enabled = false;
    bool qualityHigh = false;
    bool earlyReflectionsOnly = false;
    float mix = 0.3f;
    float roomSize = 1.0f;
    float damping = 0.5f;
    float dampingCoefficient = 0.4f;
    float inputInjectionGain = 0.5f;

    std::array<std::vector<float>, NUM_LINES> delayLines;
    std::array<int, NUM_LINES> delaySamples {};
    std::array<int, NUM_LINES> writePos {};
    std::array<float, NUM_LINES> feedbackGain {};
    std::array<float, NUM_LINES> dampingState {};
    std::array<float, NUM_LINES> lfoPhase {};
    std::array<float, NUM_LINES> lfoIncrement {};
    std::array<float, NUM_LINES> modDepthSamples {};
};
