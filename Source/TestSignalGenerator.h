#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

//==============================================================================
/**
 * TestSignalGenerator
 *
 * Generates calibration test signals for room measurement:
 *   - LogSweep  : Farina exponential sine sweep (20 Hz – 20 kHz).
 *                 Also produces the inverse filter needed for IR deconvolution.
 *   - PinkNoise : 1/f spectrum via Paul Kellet filter.
 *   - WhiteNoise: Flat spectrum via xorshift64 PRNG.
 *   - Impulse   : Single-sample Dirac delta.
 *
 * Usage:
 *   TestSignalGenerator gen;
 *   gen.prepare (sampleRate, Type::LogSweep, 3.0f, -20.0f);
 *   while (!gen.isComplete())
 *       gen.generateBlock (output, blockSize);
 *   // After sweep: const auto& inv = gen.getInverseFilter();
 */
class TestSignalGenerator
{
public:
    enum class Type { LogSweep = 0, PinkNoise = 1, WhiteNoise = 2, Impulse = 3 };

    /** Frequency range of the log sweep. */
    static constexpr float SWEEP_F1 = 20.0f;
    static constexpr float SWEEP_F2 = 20000.0f;

    //==========================================================================
    /** Prepare the generator. Pre-computes the sweep buffer when Type == LogSweep.
        @param sampleRate       Host sample rate.
        @param type             Signal type.
        @param durationSeconds  Duration (ignored for Impulse).
        @param levelDb          Output amplitude in dBFS. */
    void prepare (double sampleRate, Type type, float durationSeconds, float levelDb)
    {
        sampleRate_   = sampleRate;
        type_         = type;
        levelGain_    = juce::Decibels::decibelsToGain (levelDb);
        playbackPos_  = 0;
        complete_     = false;

        for (auto& s : pinkState_) s = 0.0f;
        noiseState_ = 12345678901234567ULL;

        if (type == Type::Impulse)
        {
            totalSamples_ = 1;
            sweepBuffer_.clear();
            inverseFilter_.clear();
        }
        else
        {
            totalSamples_ = static_cast<int> (std::round (durationSeconds * sampleRate));

            if (type == Type::LogSweep)
                buildLogSweep (durationSeconds);
        }
    }

    //==========================================================================
    /** Generate one block of audio.
        @param output     Destination buffer (must be at least numSamples long).
        @param numSamples Number of samples to generate.
        @return           true while signal is still playing, false when complete. */
    bool generateBlock (float* output, int numSamples)
    {
        if (complete_)
        {
            std::fill (output, output + numSamples, 0.0f);
            return false;
        }

        switch (type_)
        {
            case Type::LogSweep:   generateSweepBlock   (output, numSamples); break;
            case Type::PinkNoise:  generatePinkBlock    (output, numSamples); break;
            case Type::WhiteNoise: generateWhiteBlock   (output, numSamples); break;
            case Type::Impulse:    generateImpulseBlock (output, numSamples); break;
        }

        return !complete_;
    }

    bool isComplete()        const { return complete_; }
    int  getTotalSamples()   const { return totalSamples_; }
    int  getPlaybackPosition() const { return playbackPos_; }

    /** The Farina inverse filter for deconvolution.
        Only valid after prepare() with Type::LogSweep. */
    const std::vector<float>& getInverseFilter() const { return inverseFilter_; }

private:
    //==========================================================================
    void buildLogSweep (float durationSeconds)
    {
        const int    N  = totalSamples_;
        const double f1 = SWEEP_F1;
        const double f2 = SWEEP_F2;
        const double T  = durationSeconds;
        const double K  = T / std::log (f2 / f1); // rate constant

        sweepBuffer_.resize (N);
        for (int n = 0; n < N; ++n)
        {
            double t     = static_cast<double> (n) / sampleRate_;
            double phase = 2.0 * M_PI * f1 * K * (std::exp (t / T * std::log (f2 / f1)) - 1.0);
            sweepBuffer_[n] = static_cast<float> (std::sin (phase));
        }

        // Inverse filter = time-reversed sweep × 1/f amplitude correction.
        // The correction compensates for the log sweep's frequency-dependent
        // energy (more time at low freqs → more energy there).
        // Envelope at reversed index n: (f2/f1)^(-n/N)
        inverseFilter_.resize (N);
        for (int n = 0; n < N; ++n)
        {
            float env = static_cast<float> (
                std::pow (f2 / f1, -static_cast<double> (n) / N));
            inverseFilter_[n] = sweepBuffer_[N - 1 - n] * env;
        }

        // Normalise inverse filter to unit energy
        float energy = 0.0f;
        for (float s : inverseFilter_) energy += s * s;
        if (energy > 0.0f)
        {
            float norm = 1.0f / std::sqrt (energy / N);
            for (float& s : inverseFilter_) s *= norm;
        }
    }

    //==========================================================================
    void generateSweepBlock (float* output, int numSamples)
    {
        int remain = std::min (numSamples, totalSamples_ - playbackPos_);
        for (int i = 0; i < remain; ++i)
            output[i] = sweepBuffer_[playbackPos_++] * levelGain_;
        for (int i = remain; i < numSamples; ++i) output[i] = 0.0f;
        if (playbackPos_ >= totalSamples_) complete_ = true;
    }

    void generatePinkBlock (float* output, int numSamples)
    {
        int remain = std::min (numSamples, totalSamples_ - playbackPos_);
        for (int i = 0; i < remain; ++i)
        {
            float w = nextWhite() * 2.0f - 1.0f;
            pinkState_[0] =  0.99886f * pinkState_[0] + w * 0.0555179f;
            pinkState_[1] =  0.99332f * pinkState_[1] + w * 0.0750759f;
            pinkState_[2] =  0.96900f * pinkState_[2] + w * 0.1538520f;
            pinkState_[3] =  0.86650f * pinkState_[3] + w * 0.3104856f;
            pinkState_[4] =  0.55000f * pinkState_[4] + w * 0.5329522f;
            pinkState_[5] = -0.76160f * pinkState_[5] - w * 0.0168980f;
            float pink = pinkState_[0] + pinkState_[1] + pinkState_[2]
                       + pinkState_[3] + pinkState_[4] + pinkState_[5]
                       + pinkState_[6] + w * 0.5362f;
            pinkState_[6] = w * 0.115926f;
            output[i] = pink * 0.11f * levelGain_;
            ++playbackPos_;
        }
        for (int i = remain; i < numSamples; ++i) output[i] = 0.0f;
        if (playbackPos_ >= totalSamples_) complete_ = true;
    }

    void generateWhiteBlock (float* output, int numSamples)
    {
        int remain = std::min (numSamples, totalSamples_ - playbackPos_);
        for (int i = 0; i < remain; ++i)
        {
            output[i] = (nextWhite() * 2.0f - 1.0f) * levelGain_;
            ++playbackPos_;
        }
        for (int i = remain; i < numSamples; ++i) output[i] = 0.0f;
        if (playbackPos_ >= totalSamples_) complete_ = true;
    }

    void generateImpulseBlock (float* output, int numSamples)
    {
        output[0] = (playbackPos_ == 0) ? levelGain_ : 0.0f;
        for (int i = 1; i < numSamples; ++i) output[i] = 0.0f;
        playbackPos_ = totalSamples_;
        complete_ = true;
    }

    //==========================================================================
    // xorshift64 PRNG → float in [0, 1)
    float nextWhite()
    {
        noiseState_ ^= noiseState_ << 13;
        noiseState_ ^= noiseState_ >> 7;
        noiseState_ ^= noiseState_ << 17;
        return static_cast<float> (noiseState_ & 0xFFFFFFu) / static_cast<float> (0x1000000u);
    }

    //==========================================================================
    double sampleRate_   = 44100.0;
    Type   type_         = Type::LogSweep;
    float  levelGain_    = 1.0f;
    int    totalSamples_ = 0;
    int    playbackPos_  = 0;
    bool   complete_     = false;

    std::vector<float> sweepBuffer_;
    std::vector<float> inverseFilter_;

    float    pinkState_[7] = {};
    uint64_t noiseState_   = 12345678901234567ULL;
};
