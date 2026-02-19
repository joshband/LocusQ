#pragma once

#include "SceneGraph.h"         // for SpeakerProfile::NUM_FREQ_BINS
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

//==============================================================================
/**
 * RoomAnalyzer
 *
 * Analyses a measured impulse response (from IRCapture) and extracts:
 *   - Direct-sound time-of-arrival (delay compensation in ms)
 *   - Level trim relative to a reference RMS measurement
 *   - Frequency response (SpeakerProfile::NUM_FREQ_BINS log-spaced dB values)
 *   - Early reflections (up to MAX_REFLECTIONS peaks in the 1–80 ms window)
 *   - Estimated RT60 via Schroeder backward integration
 *
 * Not real-time safe. Call from a background / analysis thread.
 */
class RoomAnalyzer
{
public:
    static constexpr int MAX_REFLECTIONS = 20;

    //--------------------------------------------------------------------------
    struct Reflection
    {
        float delayMs  = 0.0f;   // time after direct sound
        float levelDb  = -96.0f; // level relative to direct
    };

    struct AnalysisResult
    {
        float    delayMs                                       = 0.0f;
        float    gainTrimDb                                    = 0.0f;
        float    frequencyResponse[SpeakerProfile::NUM_FREQ_BINS] = {};
        Reflection earlyReflections[MAX_REFLECTIONS]           = {};
        int      numReflections                                = 0;
        float    estimatedRT60                                 = 0.0f;
        bool     valid                                         = false;
    };

    //==========================================================================
    void prepare (double sampleRate) { sampleRate_ = sampleRate; }

    /** Analyse one speaker IR.
        @param ir       IR vector from IRCapture::getIR().
        @param refRms   RMS of the direct-sound window from a reference
                        measurement (used to compute gain trim).
                        Pass 0 to skip gain trim calculation. */
    AnalysisResult analyze (const std::vector<float>& ir, float refRms = 0.01f)
    {
        AnalysisResult result;
        if (ir.empty()) return result;

        result.delayMs    = findDirectArrivalMs (ir);
        result.gainTrimDb = computeGainTrimDb (ir, result.delayMs, refRms);
        computeFrequencyResponse (ir, result.frequencyResponse);
        extractEarlyReflections (ir, result.delayMs, result);
        result.estimatedRT60 = estimateRT60 (ir);
        result.valid = true;
        return result;
    }

private:
    //==========================================================================
    // Find first sample exceeding 10 % of overall peak → direct arrival
    float findDirectArrivalMs (const std::vector<float>& ir)
    {
        float peak = 0.0f;
        for (float s : ir) peak = std::max (peak, std::abs (s));
        if (peak < 1e-6f) return 0.0f;

        float thr = peak * 0.10f;
        for (int n = 0; n < static_cast<int> (ir.size()); ++n)
            if (std::abs (ir[n]) >= thr)
                return static_cast<float> (n) / static_cast<float> (sampleRate_) * 1000.0f;

        return 0.0f;
    }

    // RMS in a 10 ms window after direct arrival; gain trim vs. reference
    float computeGainTrimDb (const std::vector<float>& ir,
                             float directDelayMs, float refRms)
    {
        int start = static_cast<int> (directDelayMs * 0.001f * sampleRate_);
        int len   = static_cast<int> (0.010f * sampleRate_); // 10 ms window
        int end   = std::min (start + len, static_cast<int> (ir.size()));

        if (end <= start || refRms < 1e-10f) return 0.0f;

        float sumSq = 0.0f;
        for (int i = start; i < end; ++i) sumSq += ir[i] * ir[i];
        float rms = std::sqrt (sumSq / (end - start));

        if (rms < 1e-10f) return 0.0f;
        return juce::Decibels::gainToDecibels (refRms / rms); // +dB = needs boost
    }

    // FFT-based frequency response: NUM_FREQ_BINS log-spaced magnitude in dB,
    // normalised so that the mean = 0 dB
    void computeFrequencyResponse (const std::vector<float>& ir, float* out)
    {
        const int NUM_BINS  = SpeakerProfile::NUM_FREQ_BINS;
        const int FFT_ORDER = 12;          // 2^12 = 4096 samples
        const int FFT_SIZE  = 1 << FFT_ORDER;

        std::fill (out, out + NUM_BINS, 0.0f);

        juce::dsp::FFT fft (FFT_ORDER);
        std::vector<float> fftData (static_cast<size_t> (2 * FFT_SIZE), 0.0f);

        int copyLen = std::min (static_cast<int> (ir.size()), FFT_SIZE);
        std::copy (ir.begin(), ir.begin() + copyLen, fftData.begin());

        fft.performRealOnlyForwardTransform (fftData.data());

        const float binHz = static_cast<float> (sampleRate_) / FFT_SIZE;
        const float logF1 = std::log (20.0f);
        const float logF2 = std::log (20000.0f);

        for (int i = 0; i < NUM_BINS; ++i)
        {
            float logFreq = logF1 + (logF2 - logF1) * static_cast<float> (i) / (NUM_BINS - 1);
            float freq    = std::exp (logFreq);
            int   bin     = juce::jlimit (1, FFT_SIZE / 2 - 1,
                                          static_cast<int> (freq / binHz));
            float re  = fftData[2 * bin];
            float im  = fftData[2 * bin + 1];
            out[i]    = juce::Decibels::gainToDecibels (std::sqrt (re * re + im * im) + 1e-10f);
        }

        // Normalise: subtract mean so that 0 dB = flat response
        float sum = 0.0f;
        for (int i = 0; i < NUM_BINS; ++i) sum += out[i];
        float mean = sum / NUM_BINS;
        for (int i = 0; i < NUM_BINS; ++i) out[i] -= mean;
    }

    // Peak-picking in the 1–80 ms window after direct arrival
    void extractEarlyReflections (const std::vector<float>& ir,
                                  float directDelayMs, AnalysisResult& result)
    {
        result.numReflections = 0;

        int directSample = static_cast<int> (directDelayMs * 0.001f * sampleRate_);
        int startSample  = directSample + static_cast<int> (0.001f * sampleRate_);
        int endSample    = directSample + static_cast<int> (0.080f * sampleRate_);
        endSample        = std::min (endSample, static_cast<int> (ir.size()));

        if (startSample >= endSample) return;

        // Measure direct-sound peak for reference level
        float directPeak = 0.0f;
        int   directWin  = static_cast<int> (0.003f * sampleRate_);
        for (int n = directSample;
             n < std::min (directSample + directWin, (int) ir.size()); ++n)
            directPeak = std::max (directPeak, std::abs (ir[n]));

        if (directPeak < 1e-6f) return;

        const float minLevelDb = -40.0f;
        const int   minSpacing = static_cast<int> (0.002f * sampleRate_); // 2 ms
        int lastPeak = 0;

        for (int n = startSample + 1; n < endSample - 1; ++n)
        {
            float a = std::abs (ir[n]);
            // Local maximum
            if (a > std::abs (ir[n - 1]) && a > std::abs (ir[n + 1]))
            {
                float levelDb = juce::Decibels::gainToDecibels (a / directPeak);
                if (levelDb > minLevelDb && (n - lastPeak) > minSpacing
                    && result.numReflections < MAX_REFLECTIONS)
                {
                    auto& ref     = result.earlyReflections[result.numReflections++];
                    ref.delayMs   = static_cast<float> (n - directSample)
                                  / static_cast<float> (sampleRate_) * 1000.0f;
                    ref.levelDb   = levelDb;
                    lastPeak      = n;
                }
            }
        }
    }

    // Schroeder backward-integration EDC → T20 extrapolated to T60
    float estimateRT60 (const std::vector<float>& ir)
    {
        if (ir.empty()) return 0.4f;

        // Build energy decay curve
        float totalEnergy = 0.0f;
        for (float s : ir) totalEnergy += s * s;
        if (totalEnergy < 1e-12f) return 0.4f;

        std::vector<float> edc (ir.size());
        float running = 0.0f;
        for (int n = static_cast<int> (ir.size()) - 1; n >= 0; --n)
        {
            running += ir[n] * ir[n];
            edc[n]   = running / totalEnergy;
        }

        // Find −5 dB and −25 dB crossing times
        float t5 = -1.0f, t25 = -1.0f;
        for (int n = 0; n < static_cast<int> (edc.size()); ++n)
        {
            float db = juce::Decibels::gainToDecibels (std::sqrt (edc[n] + 1e-12f));
            if (db <= -5.0f  && t5  < 0.0f)
                t5  = static_cast<float> (n) / static_cast<float> (sampleRate_);
            if (db <= -25.0f && t25 < 0.0f)
                t25 = static_cast<float> (n) / static_cast<float> (sampleRate_);
        }

        if (t5 >= 0.0f && t25 > t5)
            return (t25 - t5) * 3.0f; // T20 → T60 extrapolation

        // Fallback: rough estimate from IR length
        return static_cast<float> (ir.size()) / static_cast<float> (sampleRate_) * 0.7f;
    }

    double sampleRate_ = 44100.0;
};
