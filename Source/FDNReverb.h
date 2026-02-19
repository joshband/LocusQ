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
 * Lightweight 4x4 feedback delay network for quad late-reverb tail.
 */
class FDNReverb
{
public:
    static constexpr int NUM_CHANNELS = 4;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        currentSampleRate = sampleRate;
        configureDelayLengths();
        reset();
        updateFeedbackGains();
    }

    void reset()
    {
        for (auto& line : delayLines)
            std::fill (line.begin(), line.end(), 0.0f);

        for (auto& pos : writePos)
            pos = 0;

        for (auto& state : dampingState)
            state = 0.0f;
    }

    void setEnabled (bool shouldEnable)        { enabled = shouldEnable; }
    void setMix (float newMix)                 { mix = juce::jlimit (0.0f, 1.0f, newMix); }
    void setRoomSize (float newRoomSize)       { roomSize = juce::jlimit (0.5f, 5.0f, newRoomSize); configureDelayLengths(); updateFeedbackGains(); }
    void setDamping (float newDamping)         { damping = juce::jlimit (0.0f, 1.0f, newDamping); }
    void setHighQuality (bool highQuality)     { qualityHigh = highQuality; configureDelayLengths(); updateFeedbackGains(); }
    void setEarlyReflectionsOnly (bool onlyER) { earlyReflectionsOnly = onlyER; }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled || earlyReflectionsOnly || mix <= 0.0f)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = juce::jmin (NUM_CHANNELS, buffer.getNumChannels());
        if (numChannels < NUM_CHANNELS)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            std::array<float, NUM_CHANNELS> delayed {};
            std::array<float, NUM_CHANNELS> dry {};

            for (int ch = 0; ch < NUM_CHANNELS; ++ch)
            {
                dry[static_cast<size_t> (ch)] = buffer.getSample (ch, i);
                const auto& line = delayLines[static_cast<size_t> (ch)];
                delayed[static_cast<size_t> (ch)] = line[static_cast<size_t> (writePos[static_cast<size_t> (ch)])];
            }

            // Hadamard-like orthogonal mixing.
            const float m0 =  0.5f * (delayed[0] + delayed[1] + delayed[2] + delayed[3]);
            const float m1 =  0.5f * (delayed[0] - delayed[1] + delayed[2] - delayed[3]);
            const float m2 =  0.5f * (delayed[0] + delayed[1] - delayed[2] - delayed[3]);
            const float m3 =  0.5f * (delayed[0] - delayed[1] - delayed[2] + delayed[3]);
            const std::array<float, NUM_CHANNELS> mixed { m0, m1, m2, m3 };

            for (int ch = 0; ch < NUM_CHANNELS; ++ch)
            {
                const float dampCoeff = 1.0f - damping * 0.7f;
                dampingState[static_cast<size_t> (ch)] =
                    dampingState[static_cast<size_t> (ch)] * (1.0f - dampCoeff)
                    + mixed[static_cast<size_t> (ch)] * dampCoeff;

                float writeSample = dry[static_cast<size_t> (ch)]
                                  + dampingState[static_cast<size_t> (ch)] * feedbackGain[static_cast<size_t> (ch)];

                delayLines[static_cast<size_t> (ch)][static_cast<size_t> (writePos[static_cast<size_t> (ch)])] = writeSample;

                const float wet = delayed[static_cast<size_t> (ch)];
                buffer.setSample (ch, i, dry[static_cast<size_t> (ch)] * (1.0f - mix) + wet * mix);

                ++writePos[static_cast<size_t> (ch)];
                if (writePos[static_cast<size_t> (ch)] >= static_cast<int> (delayLines[static_cast<size_t> (ch)].size()))
                    writePos[static_cast<size_t> (ch)] = 0;
            }
        }
    }

private:
    void configureDelayLengths()
    {
        static constexpr std::array<int, NUM_CHANNELS> draftBase { 1499, 1877, 2137, 2557 };
        static constexpr std::array<int, NUM_CHANNELS> finalBase { 2029, 2467, 2903, 3343 };

        for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        {
            const int base = qualityHigh ? finalBase[static_cast<size_t> (ch)]
                                         : draftBase[static_cast<size_t> (ch)];
            const int scaled = juce::jmax (64, static_cast<int> (base * roomSize));
            delayLines[static_cast<size_t> (ch)].assign (static_cast<size_t> (scaled), 0.0f);
            writePos[static_cast<size_t> (ch)] = 0;
        }
    }

    void updateFeedbackGains()
    {
        const float rt60 = qualityHigh ? 2.8f : 1.8f;
        for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        {
            const float delaySeconds = static_cast<float> (delayLines[static_cast<size_t> (ch)].size()) / static_cast<float> (currentSampleRate);
            feedbackGain[static_cast<size_t> (ch)] = std::pow (10.0f, (-3.0f * delaySeconds) / rt60);
            feedbackGain[static_cast<size_t> (ch)] = juce::jlimit (0.2f, 0.93f, feedbackGain[static_cast<size_t> (ch)]);
        }
    }

    double currentSampleRate = 44100.0;
    bool enabled = false;
    bool qualityHigh = false;
    bool earlyReflectionsOnly = false;
    float mix = 0.3f;
    float roomSize = 1.0f;
    float damping = 0.5f;

    std::array<std::vector<float>, NUM_CHANNELS> delayLines;
    std::array<int, NUM_CHANNELS> writePos {};
    std::array<float, NUM_CHANNELS> feedbackGain {};
    std::array<float, NUM_CHANNELS> dampingState {};
};
