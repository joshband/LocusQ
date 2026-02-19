#pragma once

#include "SceneGraph.h"

#include <vector>

//==============================================================================
/**
 * DopplerProcessor
 *
 * Draft-quality variable-delay doppler effect. Uses radial emitter velocity
 * relative to listener to modulate read delay and create pitch motion.
 */
class DopplerProcessor
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        delayLineSize = juce::jmax (4096, maxBlockSize * 8);
        delayLine.assign (static_cast<size_t> (delayLineSize), 0.0f);
        writePos = 0;
        currentDelaySamples = baseDelaySamples;
    }

    void reset()
    {
        std::fill (delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        currentDelaySamples = baseDelaySamples;
    }

    void setScale (float newScale)
    {
        dopplerScale = juce::jlimit (0.0f, 5.0f, newScale);
    }

    void processBlock (float* monoData,
                       int numSamples,
                       const Vec3& position,
                       const Vec3& velocity,
                       bool enabled)
    {
        if (! enabled || dopplerScale <= 0.0f || delayLine.empty())
            return;

        const float distance = std::sqrt (position.x * position.x
                                        + position.y * position.y
                                        + position.z * position.z);

        if (distance < 1.0e-4f)
            return;

        const float radialVelocity = (velocity.x * position.x
                                    + velocity.y * position.y
                                    + velocity.z * position.z) / distance;

        // Positive radial velocity means moving away from listener.
        const float c = 343.0f;
        const float ratio = juce::jlimit (0.5f, 2.0f, c / (c + radialVelocity * dopplerScale));

        for (int i = 0; i < numSamples; ++i)
        {
            delayLine[static_cast<size_t> (writePos)] = monoData[i];

            // Delay trajectory that approximates variable playback rate.
            currentDelaySamples += (1.0f - ratio);
            currentDelaySamples = juce::jlimit (8.0f, static_cast<float> (delayLineSize - 2), currentDelaySamples);

            float readPos = static_cast<float> (writePos) - currentDelaySamples;
            while (readPos < 0.0f)
                readPos += static_cast<float> (delayLineSize);

            const int idx0 = static_cast<int> (readPos) % delayLineSize;
            const int idx1 = (idx0 + 1) % delayLineSize;
            const float frac = readPos - static_cast<float> (idx0);

            const float s0 = delayLine[static_cast<size_t> (idx0)];
            const float s1 = delayLine[static_cast<size_t> (idx1)];
            monoData[i] = s0 + (s1 - s0) * frac;

            writePos = (writePos + 1) % delayLineSize;
        }
    }

private:
    double currentSampleRate = 44100.0;
    int delayLineSize = 8192;
    int writePos = 0;

    float dopplerScale = 1.0f;
    float baseDelaySamples = 96.0f;
    float currentDelaySamples = 96.0f;

    std::vector<float> delayLine;
};
