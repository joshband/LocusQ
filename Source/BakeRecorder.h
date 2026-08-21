#pragma once

#include <atomic>
#include <vector>

class KeyframeTimeline;
struct ChoreographyOffset;

//==============================================================================
/**
 * BakeRecorder - CL-P7 Bake-to-Timeline position capture engine.
 *
 * Captures per-emitter choreography positions over a configured DAW transport
 * range, then decimates and exports them as KeyframeTrack entries.
 *
 * Threading contract:
 *   prepare()          — main thread only; safe when isRecording() is false.
 *   tick()             — PhysicsWorker thread only (called from ChoreographyWorker::compute()).
 *   isExportReady()    — any thread (atomic read).
 *   exportToTimeline() — main thread only; call after isExportReady() returns true.
 *   resetToIdle()      — main thread only; call after exportToTimeline().
 *
 * Allocation contract:
 *   prepare()  allocates the capture buffer (main thread, no RT constraint).
 *   tick()     writes to the pre-allocated buffer only — no heap allocation.
 *   exportToTimeline() allocates decimated keyframe vectors (main thread).
 */
class BakeRecorder
{
public:
    struct BakeParams
    {
        float startPPQ  = 0.0f;    ///< bake_start: transport PPQ start
        float endPPQ    = 8.0f;    ///< bake_end: transport PPQ end
        float kfDensity = 10.0f;   ///< bake_kf_density: keyframes per second
        float tolerance = 0.01f;   ///< bake_curve_fit_tolerance: metres max error
    };

    //==========================================================================
    // Main-thread interface

    /** Allocate capture buffer for the given bake range.
     *  Safe to call only when isRecording() returns false.
     *  physRateHz: PhysicsWorker tick rate (30/60/120/240 Hz).
     *  bpm: current tempo for PPQ→seconds conversion.
     *  numEmitters: number of active emitters to capture. */
    void prepare (const BakeParams& p, double bpm, float physRateHz, int numEmitters);

    /** True after tick() has finished capturing the configured range. */
    bool isExportReady () const noexcept
    {
        return state.load (std::memory_order_acquire) == kDone;
    }

    bool isRecording () const noexcept
    {
        return state.load (std::memory_order_acquire) == kRecording;
    }

    bool isPrepared () const noexcept
    {
        return state.load (std::memory_order_acquire) == kPrepared;
    }

    /** Decimate captured positions and write KeyframeTrack entries into the timeline.
     *  Call only on the main thread after isExportReady() returns true.
     *  Tracks are named: bake_pos_x_N, bake_pos_y_N, bake_pos_z_N (N = emitter index). */
    void exportToTimeline (KeyframeTimeline& timeline);

    /** Return to Idle so a new prepare() can be accepted. */
    void resetToIdle () noexcept
    {
        state.store (kIdle, std::memory_order_release);
    }

    //==========================================================================
    // Worker-thread interface

    /** Capture one tick of choreography offsets.
     *  Called from PhysicsWorker thread via ChoreographyWorker::compute().
     *  offsets: per-emitter offsets — positions are captured.
     *  numEmitters: active emitter count this tick.
     *  ppqPosition, bpm, isPlaying: from transport atomics. */
    void tick (const ChoreographyOffset* offsets,
               int numEmitters,
               double ppqPosition,
               double bpm,
               bool isPlaying) noexcept;

private:
    static constexpr int kIdle      = 0;
    static constexpr int kPrepared  = 1;
    static constexpr int kRecording = 2;
    static constexpr int kDone      = 3;

    std::atomic<int> state { kIdle };

    // Written by main thread in prepare(); read by worker thread in tick().
    // Safe because prepare() stores kPrepared with seq_cst after all writes,
    // and tick() loads state with acquire before reading these fields.
    float  startPPQ_    = 0.0f;
    float  endPPQ_      = 8.0f;
    float  kfDensity_   = 10.0f;
    float  tolerance_   = 0.01f;
    double physRateHz_  = 60.0;
    int    numEmitters_ = 0;

    // Capture buffers; allocated by prepare() on main thread.
    // Index: buf[emitter * maxSamples_ + sampleIdx].
    std::vector<float> xBuf, yBuf, zBuf;
    int    maxSamples_           = 0;
    int    writtenSamples_       = 0;    // worker thread only until kDone
    double recordStartSeconds_   = 0.0;  // seconds of bake_start PPQ at record-start BPM

    //==========================================================================
    // Decimation helper

    /** Adaptively decimate a position axis track.
     *  vals[0..numSamples-1]: raw capture at physRateHz.
     *  kfDensity seeds the target keyframe spacing; extra keyframes are inserted
     *  when needed so interpolation against every raw sample stays within
     *  tolerance. */
    static std::vector<float> decimateTrackToKeyframeTimes (const float* vals,
                                                             int numSamples,
                                                             double physRateHz,
                                                             float kfDensity,
                                                             float tolerance,
                                                             double startTimeSeconds,
                                                             std::vector<double>& outTimes);
};
