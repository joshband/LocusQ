#include "BakeRecorder.h"

#include "ChoreographyWorker.h"
#include "KeyframeTimeline.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <cstring>

//==============================================================================
void BakeRecorder::prepare (const BakeParams& p, double bpm, float physRateHz, int numEmitters)
{
    // Only prepare when not actively recording.
    if (state.load (std::memory_order_acquire) == kRecording)
        return;

    startPPQ_   = p.startPPQ;
    endPPQ_     = std::max (p.startPPQ + 0.01f, p.endPPQ);
    kfDensity_  = std::max (0.5f, p.kfDensity);
    tolerance_  = std::max (0.001f, p.tolerance);
    physRateHz_ = static_cast<double> (std::max (1.0f, physRateHz));
    numEmitters_= std::max (1, numEmitters);

    const double safeBpm     = std::max (1.0, bpm);
    const double durationPPQ = static_cast<double> (endPPQ_ - startPPQ_);
    const double durationSec = durationPPQ * 60.0 / safeBpm;

    // Cap at 5 minutes of capture.
    const int numSamples = static_cast<int> (
        std::ceil (std::min (durationSec, 300.0) * physRateHz_));
    maxSamples_ = std::max (1, numSamples);

    xBuf.assign (static_cast<std::size_t> (numEmitters_ * maxSamples_), 0.0f);
    yBuf.assign (static_cast<std::size_t> (numEmitters_ * maxSamples_), 0.0f);
    zBuf.assign (static_cast<std::size_t> (numEmitters_ * maxSamples_), 0.0f);

    writtenSamples_     = 0;
    recordStartSeconds_ = static_cast<double> (startPPQ_) * 60.0 / safeBpm;

    // seq_cst store: all preceding writes are visible to the worker thread
    // when it subsequently loads state == kPrepared.
    state.store (kPrepared, std::memory_order_seq_cst);
}

//==============================================================================
void BakeRecorder::tick (const ChoreographyOffset* offsets,
                          int numEmitters,
                          double ppqPosition,
                          double bpm,
                          bool isPlaying) noexcept
{
    const int s = state.load (std::memory_order_acquire);

    if (s == kDone || s == kIdle)
        return;

    const bool inRange = isPlaying
                      && ppqPosition >= static_cast<double> (startPPQ_)
                      && ppqPosition <  static_cast<double> (endPPQ_);

    if (s == kPrepared)
    {
        if (! inRange)
            return;

        // Transport entered the bake range: start recording.
        writtenSamples_ = 0;
        const double safeBpm = std::max (1.0, bpm);
        recordStartSeconds_ = static_cast<double> (startPPQ_) * 60.0 / safeBpm;
        state.store (kRecording, std::memory_order_release);
    }

    // state == kRecording from here.
    if (! inRange || writtenSamples_ >= maxSamples_)
    {
        state.store (kDone, std::memory_order_release);
        return;
    }

    const int numCapture = std::min (numEmitters, numEmitters_);
    const int si = writtenSamples_;

    for (int e = 0; e < numCapture; ++e)
    {
        const auto eidx = static_cast<std::size_t> (e * maxSamples_ + si);
        xBuf[eidx] = offsets[e].position.x;
        yBuf[eidx] = offsets[e].position.y;
        zBuf[eidx] = offsets[e].position.z;
    }

    ++writtenSamples_;
}

//==============================================================================
void BakeRecorder::exportToTimeline (KeyframeTimeline& timeline)
{
    if (state.load (std::memory_order_acquire) != kDone)
        return;

    const int n = writtenSamples_;
    if (n <= 0)
        return;

    for (int e = 0; e < numEmitters_; ++e)
    {
        const auto offset = static_cast<std::size_t> (e * maxSamples_);

        auto buildTrack = [&] (const juce::String& trackId, const float* vals)
        {
            std::vector<double> times;
            std::vector<float>  decimated = decimateTrackToKeyframeTimes (
                vals, n, physRateHz_, kfDensity_, tolerance_,
                recordStartSeconds_, times);

            KeyframeTrack track (trackId, KeyframeTrackType::value);
            std::vector<Keyframe> kfs;
            kfs.reserve (times.size());
            for (std::size_t i = 0; i < times.size(); ++i)
                kfs.push_back ({ times[i], decimated[i], KeyframeCurve::linear, TimelineBeatDivision::beat });
            track.setKeyframes (std::move (kfs));
            timeline.addOrReplaceTrack (std::move (track));
        };

        const juce::String idx (e);
        buildTrack ("bake_pos_x_" + idx, xBuf.data() + offset);
        buildTrack ("bake_pos_y_" + idx, yBuf.data() + offset);
        buildTrack ("bake_pos_z_" + idx, zBuf.data() + offset);
    }
}

//==============================================================================
std::vector<float> BakeRecorder::decimateTrackToKeyframeTimes (
    const float* vals,
    int          numSamples,
    double       physRateHz,
    float        kfDensity,
    float        tolerance,
    double       startTimeSeconds,
    std::vector<double>& outTimes)
{
    outTimes.clear();
    std::vector<float> outVals;

    if (numSamples <= 0)
        return outVals;

    // Step 1: stride downsample to approximately kfDensity Hz.
    const int stride = std::max (1, static_cast<int> (physRateHz / static_cast<double> (kfDensity)));

    std::vector<double> times;
    std::vector<float>  sampled;
    times.reserve (static_cast<std::size_t> (numSamples / stride + 2));
    sampled.reserve (times.capacity());

    for (int i = 0; i < numSamples; i += stride)
    {
        times.push_back  (startTimeSeconds + i / physRateHz);
        sampled.push_back (vals[i]);
    }

    // Always include the final sample.
    {
        const int lastStridedIdx = ((numSamples - 1) / stride) * stride;
        if (lastStridedIdx != numSamples - 1)
        {
            times.push_back  (startTimeSeconds + (numSamples - 1) / physRateHz);
            sampled.push_back (vals[numSamples - 1]);
        }
    }

    const int m = static_cast<int> (times.size());

    if (m <= 2)
    {
        outTimes = times;
        return sampled;
    }

    // Step 2: greedy forward linear thinning.
    // Extend a segment from 'segStart' as far right as possible while keeping
    // all skipped samples within tolerance of the linear interpolation.
    outTimes.reserve (static_cast<std::size_t> (m));
    outVals.reserve  (static_cast<std::size_t> (m));

    outTimes.push_back (times[0]);
    outVals.push_back  (sampled[0]);
    int segStart = 0;

    for (int i = 1; i < m - 1; ++i)
    {
        // Tentatively extend segment to i+1.
        const double t0 = times[segStart];
        const float  v0 = sampled[segStart];
        const double t1 = times[i + 1];
        const float  v1 = sampled[i + 1];
        const double dt = t1 - t0;

        bool canExtend = true;
        if (dt > 0.0)
        {
            for (int j = segStart + 1; j <= i; ++j)
            {
                const float interp = v0 + static_cast<float> ((times[j] - t0) / dt) * (v1 - v0);
                if (std::abs (sampled[j] - interp) > tolerance)
                {
                    canExtend = false;
                    break;
                }
            }
        }

        if (! canExtend)
        {
            outTimes.push_back (times[i]);
            outVals.push_back  (sampled[i]);
            segStart = i;
        }
    }

    outTimes.push_back (times[static_cast<std::size_t> (m - 1)]);
    outVals.push_back  (sampled[static_cast<std::size_t> (m - 1)]);

    return outVals;
}
