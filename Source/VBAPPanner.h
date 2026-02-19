#pragma once

#include <cmath>
#include <array>
#include <algorithm>

//==============================================================================
/**
 * VBAPPanner - Vector Base Amplitude Panning for Quad Speaker Layout
 *
 * 2D VBAP implementation for 4 speakers arranged in a rectangle.
 * Speaker positions (default quad layout):
 *   SPK1: Front-Left  (azimuth = -45)
 *   SPK2: Front-Right (azimuth = +45)
 *   SPK3: Rear-Right  (azimuth = +135)
 *   SPK4: Rear-Left   (azimuth = -135)
 *
 * For each emitter position (azimuth), finds the enclosing speaker pair
 * and calculates gain weights using tangent law / inverse matrix method.
 */
class VBAPPanner
{
public:
    static constexpr int NUM_SPEAKERS = 4;

    struct SpeakerGains
    {
        std::array<float, NUM_SPEAKERS> gains {};
    };

    VBAPPanner()
    {
        // Default quad layout: FL, FR, RR, RL
        setSpeakerAngles ({ -45.0f, 45.0f, 135.0f, -135.0f });
    }

    //--------------------------------------------------------------------------
    // Configure speaker angles (degrees, -180 to +180, clockwise from front)
    void setSpeakerAngles (const std::array<float, NUM_SPEAKERS>& anglesDeg)
    {
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            speakerAngles[i] = anglesDeg[i];
            float rad = anglesDeg[i] * degToRad;
            speakerX[i] = std::sin (rad);
            speakerY[i] = std::cos (rad);
        }

        // Pre-compute inverse matrices for each speaker pair
        // Pairs are adjacent speakers going clockwise: (0,1), (1,2), (2,3), (3,0)
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            int j = (i + 1) % NUM_SPEAKERS;
            computePairInverse (i, j, i);
        }
    }

    //--------------------------------------------------------------------------
    // Calculate per-speaker gains for a source at given azimuth (degrees)
    SpeakerGains calculateGains (float azimuthDeg) const
    {
        SpeakerGains result;

        // Normalize azimuth to -180..+180
        float az = normalizeAngle (azimuthDeg);

        // Find which speaker pair the source falls between
        int pairIdx = findEnclosingPair (az);

        if (pairIdx < 0)
        {
            // Fallback: equal power to all speakers
            float g = 0.5f;
            for (int i = 0; i < NUM_SPEAKERS; ++i)
                result.gains[i] = g;
            return result;
        }

        int spkA = pairIdx;
        int spkB = (pairIdx + 1) % NUM_SPEAKERS;

        // Source direction vector
        float srcRad = az * degToRad;
        float srcX = std::sin (srcRad);
        float srcY = std::cos (srcRad);

        // Apply pre-computed inverse matrix for this pair
        float gA = pairInvMatrix[pairIdx][0] * srcX + pairInvMatrix[pairIdx][1] * srcY;
        float gB = pairInvMatrix[pairIdx][2] * srcX + pairInvMatrix[pairIdx][3] * srcY;

        // Clamp negative gains (shouldn't happen if pair is correct)
        gA = std::max (0.0f, gA);
        gB = std::max (0.0f, gB);

        // Normalize to preserve energy (constant power panning)
        float norm = std::sqrt (gA * gA + gB * gB);
        if (norm > 1e-6f)
        {
            gA /= norm;
            gB /= norm;
        }

        result.gains[spkA] = gA;
        result.gains[spkB] = gB;

        return result;
    }

    //--------------------------------------------------------------------------
    // Calculate per-speaker gains with elevation (3D → 2D projection)
    // Elevation reduces overall gain and widens spread slightly
    SpeakerGains calculateGains (float azimuthDeg, float elevationDeg) const
    {
        auto result = calculateGains (azimuthDeg);

        // Elevation attenuation: objects directly above/below project to center
        float elevRad = elevationDeg * degToRad;
        float horizontalWeight = std::cos (elevRad);

        // Blend between directional panning and equal distribution
        float equalGain = 0.5f; // For 4 speakers, equal power = 0.5 per speaker
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            result.gains[i] = result.gains[i] * horizontalWeight
                             + equalGain * (1.0f - horizontalWeight);
        }

        return result;
    }

private:
    static constexpr float degToRad = 3.14159265358979323846f / 180.0f;

    std::array<float, NUM_SPEAKERS> speakerAngles {};
    std::array<float, NUM_SPEAKERS> speakerX {};
    std::array<float, NUM_SPEAKERS> speakerY {};

    // Pre-computed inverse matrices for each adjacent speaker pair
    // [pairIdx][0..3] = { inv00, inv01, inv10, inv11 }
    std::array<std::array<float, 4>, NUM_SPEAKERS> pairInvMatrix {};

    //--------------------------------------------------------------------------
    void computePairInverse (int spkA, int spkB, int pairIdx)
    {
        // 2x2 matrix: [spkA_x, spkA_y; spkB_x, spkB_y]
        float a = speakerX[spkA], b = speakerY[spkA];
        float c = speakerX[spkB], d = speakerY[spkB];

        float det = a * d - b * c;
        if (std::abs (det) < 1e-8f)
        {
            // Degenerate pair (speakers at same angle) - use identity
            pairInvMatrix[pairIdx] = { 1.0f, 0.0f, 0.0f, 1.0f };
            return;
        }

        float invDet = 1.0f / det;
        pairInvMatrix[pairIdx][0] =  d * invDet;
        pairInvMatrix[pairIdx][1] = -b * invDet;
        pairInvMatrix[pairIdx][2] = -c * invDet;
        pairInvMatrix[pairIdx][3] =  a * invDet;
    }

    //--------------------------------------------------------------------------
    int findEnclosingPair (float azDeg) const
    {
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            int j = (i + 1) % NUM_SPEAKERS;
            if (isAngleBetween (azDeg, speakerAngles[i], speakerAngles[j]))
                return i;
        }

        // Source exactly on a speaker or numerical edge case - find closest
        float minDist = 360.0f;
        int closest = 0;
        for (int i = 0; i < NUM_SPEAKERS; ++i)
        {
            float d = std::abs (angleDifference (azDeg, speakerAngles[i]));
            if (d < minDist)
            {
                minDist = d;
                closest = i;
            }
        }
        // Return the pair starting at the closest speaker
        return closest;
    }

    //--------------------------------------------------------------------------
    static float normalizeAngle (float deg)
    {
        while (deg > 180.0f)  deg -= 360.0f;
        while (deg < -180.0f) deg += 360.0f;
        return deg;
    }

    static float angleDifference (float a, float b)
    {
        float d = a - b;
        while (d > 180.0f)  d -= 360.0f;
        while (d < -180.0f) d += 360.0f;
        return d;
    }

    static bool isAngleBetween (float angle, float from, float to)
    {
        // Check if 'angle' is between 'from' and 'to' going clockwise
        float spanFromTo = angleDifference (to, from);
        float spanFromAngle = angleDifference (angle, from);

        // Both spans should be in the same direction (positive = clockwise)
        if (spanFromTo > 0.0f)
            return spanFromAngle >= 0.0f && spanFromAngle <= spanFromTo;
        else
            return spanFromAngle <= 0.0f && spanFromAngle >= spanFromTo;
    }
};
