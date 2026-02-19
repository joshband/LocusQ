#pragma once

#include <cmath>
#include <algorithm>

//==============================================================================
/**
 * AirAbsorption - Distance-driven one-pole low-pass filter
 *
 * Simulates high-frequency rolloff with distance.
 * Cutoff = maxCutoff / (1 + distance * absorptionFactor)
 *
 * Uses a simple one-pole IIR filter for efficiency:
 *   y[n] = a0 * x[n] + b1 * y[n-1]
 *   where a0 = 1 - b1, b1 = exp(-2*pi*cutoff/sampleRate)
 */
class AirAbsorption
{
public:
    AirAbsorption() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
        z1_L = 0.0f;
        z1_R = 0.0f;
    }

    void reset()
    {
        z1_L = 0.0f;
        z1_R = 0.0f;
    }

    //--------------------------------------------------------------------------
    void setAbsorptionFactor (float factor)
    {
        absorptionFactor = std::max (0.0f, factor);
    }

    //--------------------------------------------------------------------------
    // Update filter coefficient based on distance
    void updateForDistance (float distance)
    {
        float cutoff = maxCutoff / (1.0f + distance * absorptionFactor);
        cutoff = std::max (200.0f, std::min (maxCutoff, cutoff));

        if (currentSampleRate > 0.0)
        {
            float w = 2.0f * 3.14159265358979323846f * cutoff / static_cast<float> (currentSampleRate);
            coefficient = std::exp (-w);
        }
    }

    //--------------------------------------------------------------------------
    // Process a single sample (mono)
    float processSample (float input)
    {
        float a0 = 1.0f - coefficient;
        z1_L = a0 * input + coefficient * z1_L;
        return z1_L;
    }

    // Process a stereo pair
    void processStereo (float& left, float& right)
    {
        float a0 = 1.0f - coefficient;
        z1_L = a0 * left  + coefficient * z1_L;
        z1_R = a0 * right + coefficient * z1_R;
        left  = z1_L;
        right = z1_R;
    }

    // Process a buffer (mono, in-place)
    void processBlock (float* data, int numSamples)
    {
        float a0 = 1.0f - coefficient;
        for (int i = 0; i < numSamples; ++i)
        {
            z1_L = a0 * data[i] + coefficient * z1_L;
            data[i] = z1_L;
        }
    }

private:
    double currentSampleRate = 44100.0;
    float absorptionFactor = 0.3f;
    float maxCutoff = 20000.0f;    // Hz
    float coefficient = 0.0f;       // One-pole feedback coefficient (b1)
    float z1_L = 0.0f;             // Filter state left
    float z1_R = 0.0f;             // Filter state right
};
