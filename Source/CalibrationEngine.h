#pragma once

#include "TestSignalGenerator.h"
#include "IRCapture.h"
#include "RoomAnalyzer.h"
#include "RoomProfileSerializer.h"
#include "SceneGraph.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>

//==============================================================================
/**
 * CalibrationEngine
 *
 * Orchestrates the room calibration state machine:
 *
 *   Idle → Playing (speaker 0) → Recording → Analyzing
 *        → Playing (speaker 1) → Recording → Analyzing
 *        → ...
 *        → Playing (speaker 3) → Recording → Analyzing → Complete
 *
 * Audio-thread interface (processBlock) is real-time safe:
 *   - No heap allocation during Playing / Recording states.
 *   - Atomic state reads/writes with acquire-release ordering.
 *   - Background thread handles FFT-based IR analysis.
 *
 * Usage:
 *   // From prepareToPlay (non-RT):
 *   engine.prepare (sampleRate, maxBlockSize);
 *
 *   // From editor/UI (non-RT, triggers calibration sequence):
 *   int spkCh[4] = {0, 1, 0, 1};
 *   engine.startCalibration (TestSignalGenerator::Type::LogSweep,
 *                             -20.0f, 3.0f, 1.5f, spkCh, 0);
 *
 *   // From processBlock (RT):
 *   engine.processBlock (buffer, micInputChannel);
 *
 *   // Poll from editor / timer:
 *   auto prog = engine.getProgress();
 *   if (engine.isComplete())
 *       auto& result = engine.getResult();
 */
class CalibrationEngine
{
public:
    enum class State
    {
        Idle      = 0,
        Playing   = 1,   // Generating test signal → output channel
        Recording = 2,   // Recording mic input (sweep + reverb tail)
        Analyzing = 3,   // Background FFT deconvolution + analysis
        Complete  = 4,   // All 4 speakers measured; RoomProfile ready
        Error     = 5
    };

    struct Progress
    {
        State        state           = State::Idle;
        int          currentSpeaker  = 0;   // 0-based
        float        playPercent     = 0.0f;
        float        recordPercent   = 0.0f;
        juce::String message         = "Idle";
    };

    //==========================================================================
    CalibrationEngine() = default;

    ~CalibrationEngine()
    {
        // Signal analysis thread to stop and wait for it
        analysisRunning_.store (false, std::memory_order_release);
        if (analysisThread_.joinable())
            analysisThread_.join();
    }

    //==========================================================================
    /** Call from prepareToPlay. Allocates recording buffers and starts the
        background analysis thread. Not RT safe. */
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sampleRate_ = sampleRate;
        capture_.prepare (sampleRate, DEFAULT_TAIL_SECS);
        analyzer_.prepare (sampleRate);

        // Start the background analysis worker (if not already running)
        if (! analysisThread_.joinable())
        {
            analysisRunning_.store (true, std::memory_order_release);
            analysisThread_ = std::thread ([this]() { analysisWorker(); });
        }
    }

    //==========================================================================
    /** Kick off a new calibration sequence. Call from a non-audio thread (UI).
        Not real-time safe (may allocate).
        @param type             Signal type (LogSweep recommended).
        @param testLevelDb      Output amplitude in dBFS.
        @param sweepDurationSecs Duration of the sweep signal.
        @param tailSeconds      Extra recording time for the reverb tail.
        @param speakerOutputCh  0-indexed output channel for each of the 4 speakers.
        @param micInputChannel  0-indexed input channel for the measurement mic. */
    void startCalibration (TestSignalGenerator::Type type,
                           float testLevelDb,
                           float sweepDurationSecs,
                           float tailSeconds,
                           const int speakerOutputCh[4],
                           int micInputChannel)
    {
        if (state_.load() != State::Idle) return;

        type_          = type;
        testLevelDb_   = testLevelDb;
        sweepDuration_ = sweepDurationSecs;
        tailSeconds_   = tailSeconds;
        micChannel_    = micInputChannel;
        for (int i = 0; i < 4; ++i)
            speakerOutputCh_[i] = speakerOutputCh[i];

        resultProfile_ = RoomProfile{};
        startSpeaker (0);
    }

    /** Abort calibration and return to Idle. */
    void abortCalibration()
    {
        state_.store (State::Idle, std::memory_order_release);
    }

    //==========================================================================
    /** Real-time safe. Call from processBlock when mode == Calibrate. */
    void processBlock (juce::AudioBuffer<float>& buffer, int micInputChannel)
    {
        const auto state    = state_.load (std::memory_order_acquire);
        const int  numSamps = buffer.getNumSamples();
        const int  numCh    = buffer.getNumChannels();

        if (state == State::Playing)
        {
            buffer.clear();

            int outCh = juce::jlimit (0, numCh - 1,
                                       speakerOutputCh_[currentSpeaker_.load()]);
            float* out = buffer.getWritePointer (outCh);

            bool stillPlaying = generator_.generateBlock (out, numSamps);

            if (! stillPlaying)
            {
                // Sweep ended → begin recording the reverb tail
                capture_.startRecording (generator_.getTotalSamples(), tailSeconds_);
                state_.store (State::Recording, std::memory_order_release);
            }
        }
        else if (state == State::Recording)
        {
            // IMPORTANT: read mic input BEFORE clearing (in-place buffer:
            // getReadPointer and getWritePointer may return the same address).
            int micCh = juce::jlimit (0, numCh - 1, micInputChannel);
            const float* mic = buffer.getReadPointer (micCh);
            capture_.recordBlock (mic, numSamps);

            buffer.clear();

            if (capture_.isComplete())
            {
                state_.store (State::Analyzing, std::memory_order_release);
                analysisRequested_.store (true, std::memory_order_release);
            }
        }
        else
        {
            buffer.clear();
        }
    }

    //==========================================================================
    Progress getProgress() const
    {
        Progress p;
        p.state          = state_.load (std::memory_order_acquire);
        p.currentSpeaker = currentSpeaker_.load (std::memory_order_acquire);

        switch (p.state)
        {
            case State::Idle:
                p.message = "Idle — press Start to begin calibration";
                break;

            case State::Playing:
            {
                int total = generator_.getTotalSamples();
                int pos   = generator_.getPlaybackPosition();
                p.playPercent = total > 0 ? static_cast<float> (pos) / total : 0.0f;
                p.message = "Playing test signal — Speaker "
                          + juce::String (p.currentSpeaker + 1) + " of 4";
                break;
            }

            case State::Recording:
            {
                int exp = capture_.getExpectedLength();
                int rec = capture_.getRecordedSamples();
                p.recordPercent = exp > 0 ? static_cast<float> (rec) / exp : 0.0f;
                p.message = "Recording room response — Speaker "
                          + juce::String (p.currentSpeaker + 1) + " of 4";
                break;
            }

            case State::Analyzing:
                p.message = "Analysing impulse response — Speaker "
                          + juce::String (p.currentSpeaker + 1) + " of 4";
                break;

            case State::Complete:
                p.message = "Calibration complete — Room Profile ready";
                break;

            case State::Error:
                p.message = "Error during calibration";
                break;
        }

        return p;
    }

    bool              isComplete()   const { return state_.load() == State::Complete; }
    const RoomProfile& getResult()   const { return resultProfile_; }
    State             getState()     const { return state_.load(); }

private:
    //==========================================================================
    // Prepare generator and kick off Playing state for a specific speaker.
    // Called from the analysis background thread — allocations are fine here.
    void startSpeaker (int speakerIdx)
    {
        currentSpeaker_.store (speakerIdx, std::memory_order_release);
        generator_.prepare (sampleRate_, type_, sweepDuration_, testLevelDb_);
        state_.store (State::Playing, std::memory_order_release);
    }

    //==========================================================================
    // Background worker: waits for analysisRequested_, runs FFT deconvolution
    // + analysis, stores results, then advances the state machine.
    void analysisWorker()
    {
        while (analysisRunning_.load (std::memory_order_acquire))
        {
            if (analysisRequested_.exchange (false, std::memory_order_acq_rel))
            {
                int spk = currentSpeaker_.load (std::memory_order_acquire);

                // --- IR deconvolution (expensive) ---
                capture_.computeIR (generator_.getInverseFilter());

                if (capture_.isIRReady())
                {
                    auto res = analyzer_.analyze (capture_.getIR());

                    if (res.valid)
                    {
                        auto& spkProfile = resultProfile_.speakers[spk];
                        spkProfile.delayComp = res.delayMs;
                        spkProfile.gainTrim  = res.gainTrimDb;

                        for (int b = 0; b < SpeakerProfile::NUM_FREQ_BINS; ++b)
                            spkProfile.frequencyResponse[b] = res.frequencyResponse[b];

                        // Estimate speaker distance from time-of-arrival
                        // (speed of sound ≈ 343 m/s)
                        spkProfile.distance = (res.delayMs / 1000.0f) * 343.0f;

                        DBG ("CalibrationEngine: Speaker " << (spk + 1)
                             << "  delay=" << res.delayMs  << " ms"
                             << "  gain="  << res.gainTrimDb << " dB"
                             << "  RT60="  << res.estimatedRT60 << " s"
                             << "  reflections=" << res.numReflections);
                    }
                }

                // Advance to next speaker or finish
                if (spk < 3)
                {
                    startSpeaker (spk + 1);
                }
                else
                {
                    // All 4 speakers measured — assemble final profile
                    resultProfile_.valid        = true;
                    resultProfile_.estimatedRT60 = 0.0f;
                    for (int i = 0; i < 4; ++i)
                    {
                        resultProfile_.estimatedRT60 =
                            std::max (resultProfile_.estimatedRT60,
                                      resultProfile_.speakers[i].distance / 343.0f);
                    }

                    // Publish to SceneGraph so Renderer mode picks it up
                    SceneGraph::getInstance().setRoomProfile (resultProfile_);

                    state_.store (State::Complete, std::memory_order_release);

                    DBG ("CalibrationEngine: Calibration complete — "
                         "Room Profile published to SceneGraph");
                }
            }

            // Poll every 5 ms to avoid busy-spinning
            std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }
    }

    //==========================================================================
    static constexpr float DEFAULT_TAIL_SECS = 1.5f;

    // Atomic state
    std::atomic<State> state_             { State::Idle };
    std::atomic<int>   currentSpeaker_    { 0 };
    std::atomic<bool>  analysisRunning_   { false };
    std::atomic<bool>  analysisRequested_ { false };
    std::thread        analysisThread_;

    // Configuration (set at startCalibration, read-only during calibration)
    double sampleRate_   = 44100.0;
    TestSignalGenerator::Type type_ = TestSignalGenerator::Type::LogSweep;
    float  testLevelDb_  = -20.0f;
    float  sweepDuration_= 3.0f;
    float  tailSeconds_  = DEFAULT_TAIL_SECS;
    int    micChannel_   = 0;
    int    speakerOutputCh_[4] = { 0, 1, 0, 1 };

    // Subsystems
    TestSignalGenerator generator_;
    IRCapture           capture_;
    RoomAnalyzer        analyzer_;

    // Result (written by background thread, read by UI/editor)
    RoomProfile resultProfile_;

    JUCE_DECLARE_NON_COPYABLE (CalibrationEngine)
};
