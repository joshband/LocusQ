#pragma once

#include <cmath>
#include <algorithm>

//==============================================================================
/**
 * DistanceAttenuator - Distance-based gain attenuation with 3 models
 *
 * Models:
 *   InverseSquare: gain = refDist / max(distance, refDist)  (clamped)
 *   Linear:        gain = 1 - (distance - refDist) / (maxDist - refDist)
 *   Logarithmic:   gain = 1 - log(distance / refDist) / log(maxDist / refDist)
 *
 * All models return gain in linear scale (0.0 to 1.0).
 * Below refDist, gain is 1.0. Beyond maxDist, gain is 0.0 (silence floor).
 */
class DistanceAttenuator
{
public:
    enum class Model
    {
        InverseSquare = 0,
        Linear,
        Logarithmic
    };

    DistanceAttenuator() = default;

    //--------------------------------------------------------------------------
    void setModel (Model m) { model = m; }
    void setModel (int m) { model = static_cast<Model> (std::max (0, std::min (2, m))); }

    void setReferenceDistance (float refDist)
    {
        referenceDistance = std::max (0.01f, refDist);
    }

    void setMaxDistance (float maxDist)
    {
        maxDistance = std::max (referenceDistance + 0.1f, maxDist);
    }

    //--------------------------------------------------------------------------
    // Calculate linear gain for a given distance (meters)
    float calculateGain (float distance) const
    {
        if (distance <= referenceDistance)
            return 1.0f;

        if (distance >= maxDistance)
            return 0.0f;

        switch (model)
        {
            case Model::InverseSquare:
            {
                float ratio = referenceDistance / distance;
                return ratio * ratio; // 1/r^2
            }

            case Model::Linear:
            {
                float range = maxDistance - referenceDistance;
                if (range < 0.01f) return 1.0f;
                return 1.0f - (distance - referenceDistance) / range;
            }

            case Model::Logarithmic:
            {
                float logRange = std::log (maxDistance / referenceDistance);
                if (logRange < 0.001f) return 1.0f;
                float logDist = std::log (distance / referenceDistance);
                return 1.0f - (logDist / logRange);
            }

            default:
                return 1.0f;
        }
    }

    //--------------------------------------------------------------------------
    // Calculate gain in dB
    float calculateGainDb (float distance) const
    {
        float gain = calculateGain (distance);
        if (gain <= 0.0f) return -100.0f;
        return 20.0f * std::log10 (gain);
    }

private:
    Model model = Model::InverseSquare;
    float referenceDistance = 1.0f;  // meters
    float maxDistance = 50.0f;       // meters
};
