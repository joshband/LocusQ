#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <vector>

//==============================================================================
/**
 * EarlyReflections
 *
 * Multi-tap delay network that adds room-dependent early reflections.
 */
class EarlyReflections
{
public:
    static constexpr int NUM_SPEAKERS = 4;
    static constexpr int MAX_TAPS = 16;

    void prepare (double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        const int minSamples = juce::jmax (maxBlockSize * 8, static_cast<int> (sampleRate * 2.0));

        for (int spk = 0; spk < NUM_SPEAKERS; ++spk)
        {
            delayLines[static_cast<size_t> (spk)].assign (static_cast<size_t> (minSamples), 0.0f);
            writePos[static_cast<size_t> (spk)] = 0;
        }

        updateTapTable();
    }

    void reset()
    {
        for (auto& line : delayLines)
            std::fill (line.begin(), line.end(), 0.0f);

        for (auto& pos : writePos)
            pos = 0;
    }

    void setEnabled (bool shouldEnable)      { enabled = shouldEnable; }
    void setMix (float newMix)               { mix = juce::jlimit (0.0f, 1.0f, newMix); }
    void setRoomSize (float newRoomSize)     { roomSize = juce::jlimit (0.5f, 5.0f, newRoomSize); updateTapTable(); }
    void setDamping (float newDamping)       { damping = juce::jlimit (0.0f, 1.0f, newDamping); updateTapTable(); }
    void setHighQuality (bool highQuality)   { qualityHigh = highQuality; updateTapTable(); }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled || mix <= 0.0f)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = juce::jmin (NUM_SPEAKERS, buffer.getNumChannels());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channel = buffer.getWritePointer (ch);
            auto& line = delayLines[static_cast<size_t> (ch)];
            auto& wp = writePos[static_cast<size_t> (ch)];
            const int lineSize = static_cast<int> (line.size());

            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = channel[i];
                line[static_cast<size_t> (wp)] = dry;

                float wet = 0.0f;
                for (int tap = 0; tap < numTaps; ++tap)
                {
                    int rp = wp - tapDelaySamples[static_cast<size_t> (tap)];
                    if (rp < 0)
                        rp += lineSize;
                    wet += line[static_cast<size_t> (rp)] * tapGains[static_cast<size_t> (tap)];
                }

                channel[i] = dry + wet * mix;

                ++wp;
                if (wp >= lineSize)
                    wp = 0;
            }
        }
    }

private:
    void updateTapTable()
    {
        static constexpr std::array<float, 8> baseDraftMs { 7.0f, 13.0f, 19.0f, 29.0f, 41.0f, 53.0f, 67.0f, 83.0f };
        static constexpr std::array<float, 16> baseFinalMs { 7.0f, 13.0f, 19.0f, 29.0f, 41.0f, 53.0f, 67.0f, 83.0f,
                                                              101.0f, 127.0f, 149.0f, 173.0f, 197.0f, 223.0f, 251.0f, 281.0f };

        numTaps = qualityHigh ? 16 : 8;
        const float sizeScale = roomSize;

        for (int tap = 0; tap < numTaps; ++tap)
        {
            const float baseMs = qualityHigh ? baseFinalMs[static_cast<size_t> (tap)]
                                             : baseDraftMs[static_cast<size_t> (tap)];
            const float delayMs = baseMs * sizeScale;
            tapDelaySamples[static_cast<size_t> (tap)] = juce::jmax (
                1, static_cast<int> (delayMs * 0.001f * static_cast<float> (currentSampleRate)));

            const float reflectionDecay = std::pow (0.72f, static_cast<float> (tap + 1));
            const float dampingScale = 1.0f - damping * 0.65f;
            tapGains[static_cast<size_t> (tap)] = reflectionDecay * dampingScale;
        }
    }

    double currentSampleRate = 44100.0;
    bool enabled = false;
    bool qualityHigh = false;
    float mix = 0.3f;
    float roomSize = 1.0f;
    float damping = 0.5f;

    int numTaps = 8;
    std::array<int, MAX_TAPS> tapDelaySamples {};
    std::array<float, MAX_TAPS> tapGains {};

    std::array<std::vector<float>, NUM_SPEAKERS> delayLines;
    std::array<int, NUM_SPEAKERS> writePos {};
};
