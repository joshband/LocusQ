#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

//==============================================================================
/**
 * IRCapture
 *
 * Records microphone input during a calibration sweep and deconvolves it
 * with the inverse filter (from TestSignalGenerator) to extract the room
 * impulse response (IR) for one speaker.
 *
 * All heap allocation happens in prepare() / startRecording().
 * recordBlock() is real-time safe (no allocation, no locks).
 * computeIR() is expensive — call from a background thread.
 *
 * Usage:
 *   IRCapture cap;
 *   cap.prepare (sampleRate);
 *   cap.startRecording (sweepSamples, tailSeconds);
 *   // audio thread:
 *   while (!cap.isComplete())
 *       cap.recordBlock (micInput, numSamples);
 *   // background thread:
 *   cap.computeIR (gen.getInverseFilter());
 *   const auto& ir = cap.getIR();
 */
class IRCapture
{
public:
    static constexpr int   DEFAULT_IR_SECS  = 1;    // Extract first 1 s of IR
    static constexpr int   MAX_RECORD_SECS  = 15;   // Maximum recording length

    //==========================================================================
    /** Initialise; call once from prepareToPlay.
        @param sampleRate   Host sample rate.
        @param tailSeconds  Extra recording time after sweep ends (reverb tail). */
    void prepare (double sampleRate, float tailSeconds = 1.5f)
    {
        sampleRate_     = sampleRate;
        tailSeconds_    = tailSeconds;
        recordPos_      = 0;
        expectedLength_ = 0;
        computeDone_    = false;

        int maxSamples = static_cast<int> ((MAX_RECORD_SECS + tailSeconds) * sampleRate);
        recordBuffer_.assign (static_cast<size_t> (maxSamples), 0.0f);
        impulseResponse_.clear();
    }

    /** Call just before the sweep starts to configure the capture window.
        @param sweepSamples  Length of the test signal (samples).
        @param tailSeconds   Additional tail to record after signal ends. */
    void startRecording (int sweepSamples, float tailSeconds = 1.5f)
    {
        recordPos_   = 0;
        computeDone_ = false;
        int tailSamp = static_cast<int> (tailSeconds * sampleRate_);
        expectedLength_ = sweepSamples + tailSamp;

        size_t needed = static_cast<size_t> (expectedLength_);
        if (recordBuffer_.size() < needed)
            recordBuffer_.resize (needed, 0.0f);
        else
            std::fill (recordBuffer_.begin(),
                       recordBuffer_.begin() + expectedLength_, 0.0f);
    }

    //==========================================================================
    /** Feed microphone input into the recording buffer (audio-thread safe). */
    void recordBlock (const float* micInput, int numSamples)
    {
        if (recordPos_ >= expectedLength_) return;

        int toWrite = std::min (numSamples, expectedLength_ - recordPos_);
        std::memcpy (recordBuffer_.data() + recordPos_, micInput,
                     static_cast<size_t> (toWrite) * sizeof (float));
        recordPos_ += toWrite;
    }

    bool isComplete()       const { return recordPos_ >= expectedLength_; }
    int  getExpectedLength() const { return expectedLength_; }
    int  getRecordedSamples() const { return recordPos_; }

    //==========================================================================
    /** Deconvolve recording with inverse filter via overlap-save FFT.
        Call on a background thread AFTER isComplete() returns true.
        @param inverseFilter  Time-domain inverse filter from TestSignalGenerator. */
    void computeIR (const std::vector<float>& inverseFilter)
    {
        computeDone_ = false;
        impulseResponse_.clear();

        if (inverseFilter.empty() || recordPos_ == 0) return;

        const int recLen = recordPos_;
        const int invLen = static_cast<int> (inverseFilter.size());
        const int outLen = recLen + invLen - 1; // linear convolution length

        // Choose smallest FFT order that fits
        int fftOrder = 1;
        while ((1 << fftOrder) < outLen) ++fftOrder;
        const int fftSize = 1 << fftOrder;

        juce::dsp::FFT fft (fftOrder);

        // Allocate interleaved complex buffers (size = 2 × fftSize)
        std::vector<float> recFFT (static_cast<size_t> (2 * fftSize), 0.0f);
        std::vector<float> invFFT (static_cast<size_t> (2 * fftSize), 0.0f);

        // Copy real signals into first-half of each buffer
        std::memcpy (recFFT.data(), recordBuffer_.data(),
                     static_cast<size_t> (recLen) * sizeof (float));
        std::memcpy (invFFT.data(), inverseFilter.data(),
                     static_cast<size_t> (invLen) * sizeof (float));

        // Forward FFT
        fft.performRealOnlyForwardTransform (recFFT.data());
        fft.performRealOnlyForwardTransform (invFFT.data());

        // Complex multiply in frequency domain (convolution = deconvolution
        // because inverseFilter already encodes 1/H(w))
        // JUCE interleaved layout: [re0, im0, re1, im1, ...]
        for (int k = 0; k < fftSize; ++k)
        {
            float re1 = recFFT[2 * k],     im1 = recFFT[2 * k + 1];
            float re2 = invFFT[2 * k],     im2 = invFFT[2 * k + 1];
            recFFT[2 * k]     = re1 * re2 - im1 * im2;
            recFFT[2 * k + 1] = re1 * im2 + im1 * re2;
        }

        // Inverse FFT — real output is in first fftSize elements
        fft.performRealOnlyInverseTransform (recFFT.data());

        // Extract the causal IR (first DEFAULT_IR_SECS worth of samples).
        // In the Farina method the direct-sound IR peak appears near t = 0.
        int irLen = std::min (
            static_cast<int> (DEFAULT_IR_SECS * sampleRate_), fftSize);
        impulseResponse_.resize (static_cast<size_t> (irLen));
        std::memcpy (impulseResponse_.data(), recFFT.data(),
                     static_cast<size_t> (irLen) * sizeof (float));

        // Normalise to peak magnitude
        float peak = 0.0f;
        for (float s : impulseResponse_) peak = std::max (peak, std::abs (s));
        if (peak > 1e-10f)
        {
            float norm = 1.0f / peak;
            for (float& s : impulseResponse_) s *= norm;
        }

        computeDone_ = true;
    }

    bool isIRReady()                         const { return computeDone_; }
    const std::vector<float>& getIR()        const { return impulseResponse_; }
    double getSampleRate()                   const { return sampleRate_; }

private:
    double sampleRate_     = 44100.0;
    float  tailSeconds_    = 1.5f;
    int    recordPos_      = 0;
    int    expectedLength_ = 0;
    bool   computeDone_    = false;

    std::vector<float> recordBuffer_;
    std::vector<float> impulseResponse_;
};
