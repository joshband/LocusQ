// qa/bake_recorder_probe_main.cpp
// BL-113 BakeRecorder deterministic lifecycle and Timeline export probe.

#include "Source/BakeRecorder.h"
#include "Source/ChoreographyOffset.h"
#include "Source/KeyframeTimeline.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace allocationProbe
{
std::atomic<bool> enabled { false };
std::atomic<std::size_t> count { 0 };
}

void* operator new (std::size_t size)
{
    if (allocationProbe::enabled.load (std::memory_order_relaxed))
        allocationProbe::count.fetch_add (1, std::memory_order_relaxed);

    if (void* memory = std::malloc (std::max<std::size_t> (size, 1)))
        return memory;

    throw std::bad_alloc {};
}

void* operator new[] (std::size_t size)
{
    return ::operator new (size);
}

void operator delete (void* memory) noexcept
{
    std::free (memory);
}

void operator delete[] (void* memory) noexcept
{
    ::operator delete (memory);
}

void operator delete (void* memory, std::size_t) noexcept
{
    ::operator delete (memory);
}

void operator delete[] (void* memory, std::size_t) noexcept
{
    ::operator delete[] (memory);
}

namespace
{
struct CheckResult
{
    std::string name;
    bool passed = false;
    std::string detail;
};

std::vector<CheckResult> checks;

void check (std::string name, bool passed, std::string detail)
{
    checks.push_back ({ std::move (name), passed, std::move (detail) });
    const auto& result = checks.back();
    std::cout << "CHECK " << result.name << " : " << (result.passed ? "PASS" : "FAIL")
              << " | " << result.detail << '\n';
}

bool approximatelyEqual (float lhs, float rhs, float epsilon = 1.0e-5f)
{
    return std::abs (lhs - rhs) <= epsilon;
}

std::string escapeJson (const std::string& value)
{
    std::ostringstream out;
    for (const unsigned char c : value)
    {
        switch (c)
        {
            case '\\': out << "\\\\"; break;
            case '"':  out << "\\\""; break;
            case '\n': out << "\\n";  break;
            case '\r': out << "\\r";  break;
            case '\t': out << "\\t";  break;
            default:
                if (c < 0x20)
                    out << "\\u" << std::hex << std::setw (4) << std::setfill ('0')
                        << static_cast<int> (c) << std::dec;
                else
                    out << static_cast<char> (c);
                break;
        }
    }
    return out.str();
}

std::string makeJsonResult (bool passed)
{
    std::ostringstream out;
    out << "{\n  \"suite\": \"locusq_bake_recorder_probe\",\n"
        << "  \"passed\": " << (passed ? "true" : "false") << ",\n"
        << "  \"checks\": [\n";

    for (std::size_t i = 0; i < checks.size(); ++i)
    {
        const auto& result = checks[i];
        out << "    {\"name\": \"" << escapeJson (result.name)
            << "\", \"passed\": " << (result.passed ? "true" : "false")
            << ", \"detail\": \"" << escapeJson (result.detail) << "\"}"
            << (i + 1 < checks.size() ? "," : "") << '\n';
    }

    out << "  ]\n}\n";
    return out.str();
}

const KeyframeTrack* findTrack (const KeyframeTimeline& timeline, const juce::String& id)
{
    const auto& tracks = timeline.getTracks();
    const auto match = std::find_if (tracks.begin(), tracks.end(), [&id] (const auto& track)
    {
        return track.getParameterId() == id;
    });
    return match == tracks.end() ? nullptr : std::addressof (*match);
}

float axisValue (const ChoreographyOffset& offset, int axis)
{
    if (axis == 0) return offset.position.x;
    if (axis == 1) return offset.position.y;
    return offset.position.z;
}

std::array<ChoreographyOffset, 2> makeFrame (int sampleIndex)
{
    std::array<ChoreographyOffset, 2> frame {};
    const float t = static_cast<float> (sampleIndex) / 20.0f;

    // The narrow impulses fall between the 2 Hz density seed points.  They
    // prove the decimator validates against raw capture samples.
    frame[0].position = {
        sampleIndex == 5 ? 1.0f : (sampleIndex == 15 ? -1.0f : 0.0f),
        t,
        0.25f * std::sin (t * 6.28318530717958647692f)
    };
    frame[1].position = { 2.0f + t, -0.5f * t, 0.75f };
    return frame;
}

void tickAndTrackAllocations (BakeRecorder& recorder,
                              const ChoreographyOffset* offsets,
                              int numEmitters,
                              double ppqPosition,
                              double bpm,
                              bool isPlaying,
                              bool& allocationFree)
{
    allocationProbe::count.store (0, std::memory_order_relaxed);
    allocationProbe::enabled.store (true, std::memory_order_release);
    recorder.tick (offsets, numEmitters, ppqPosition, bpm, isPlaying);
    allocationProbe::enabled.store (false, std::memory_order_release);
    allocationFree = allocationFree
        && allocationProbe::count.load (std::memory_order_relaxed) == 0;
}

void runLifecycleAndExportProbe()
{
    constexpr double bpm = 60.0;
    constexpr float physicsRateHz = 20.0f;
    constexpr int sampleCount = 20;
    constexpr float tolerance = 0.05f;

    BakeRecorder recorder;
    BakeRecorder::BakeParams params;
    params.startPPQ = 1.0f;
    params.endPPQ = 2.0f;
    params.kfDensity = 2.0f;
    params.tolerance = tolerance;

    recorder.prepare (params, bpm, physicsRateHz, 2);
    check ("prepare_state", recorder.isPrepared(), "recorder is prepared before transport enters range");

    bool tickAllocationFree = true;
    const auto outsideFrame = makeFrame (0);
    tickAndTrackAllocations (recorder, outsideFrame.data(), 2, 1.25, bpm, false,
                             tickAllocationFree);
    check ("paused_in_range_no_record", recorder.isPrepared(),
           "an in-range tick does not record while transport is paused");
    tickAndTrackAllocations (recorder, outsideFrame.data(), 2, 0.5, bpm, true,
                             tickAllocationFree);
    check ("outside_range_no_record", recorder.isPrepared(), "pre-range tick leaves recorder prepared");

    std::vector<std::array<ChoreographyOffset, 2>> rawFrames;
    rawFrames.reserve (sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        rawFrames.push_back (makeFrame (i));
        const double ppq = 1.0 + static_cast<double> (i) / physicsRateHz;
        tickAndTrackAllocations (recorder, rawFrames.back().data(), 2, ppq, bpm, true,
                                 tickAllocationFree);
    }

    check ("recording_state", recorder.isRecording(), "in-range transport ticks enter recording state");
    tickAndTrackAllocations (recorder, rawFrames.back().data(), 2, 2.0, bpm, true,
                             tickAllocationFree);
    check ("done_state", recorder.isExportReady(), "end-of-range tick marks export ready");

    KeyframeTimeline timeline;
    recorder.exportToTimeline (timeline);

    const std::array<const char*, 3> axisNames { "x", "y", "z" };
    bool allTracksPresent = timeline.getTracks().size() == 6;
    for (int emitter = 0; emitter < 2; ++emitter)
        for (const auto* axis : axisNames)
            allTracksPresent = allTracksPresent
                && timeline.hasTrack (juce::String ("bake_pos_") + axis
                                      + "_" + juce::String (emitter));
    check ("track_schema", allTracksPresent, "two emitters export exactly six named position tracks");

    bool endpointsPreserved = true;
    bool orderedTimes = true;
    bool rawToleranceBound = true;
    float maxObservedError = 0.0f;
    constexpr double firstTimeSeconds = 1.0;
    constexpr double lastTimeSeconds = firstTimeSeconds
        + static_cast<double> (sampleCount - 1) / static_cast<double> (physicsRateHz);

    for (int emitter = 0; emitter < 2; ++emitter)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            const juce::String id = juce::String ("bake_pos_") + axisNames[axis]
                                  + "_" + juce::String (emitter);
            const auto* track = findTrack (timeline, id);
            if (track == nullptr || track->getKeyframes().empty())
            {
                endpointsPreserved = false;
                orderedTimes = false;
                rawToleranceBound = false;
                continue;
            }

            const auto& keyframes = track->getKeyframes();
            endpointsPreserved = endpointsPreserved
                && approximatelyEqual (keyframes.front().value, axisValue (rawFrames.front()[emitter], axis))
                && approximatelyEqual (keyframes.back().value, axisValue (rawFrames.back()[emitter], axis))
                && std::abs (keyframes.front().timeSeconds - firstTimeSeconds) <= 1.0e-9
                && std::abs (keyframes.back().timeSeconds - lastTimeSeconds) <= 1.0e-9;

            for (std::size_t i = 1; i < keyframes.size(); ++i)
                orderedTimes = orderedTimes
                    && keyframes[i - 1].timeSeconds < keyframes[i].timeSeconds
                    && keyframes[i].timeSeconds <= lastTimeSeconds + 1.0e-9;

            for (int sample = 0; sample < sampleCount; ++sample)
            {
                const double timeSeconds = firstTimeSeconds + sample / physicsRateHz;
                const auto evaluated = timeline.evaluateTrack (id, timeSeconds);
                if (! evaluated.has_value())
                {
                    rawToleranceBound = false;
                    continue;
                }

                const float error = std::abs (*evaluated - axisValue (rawFrames[sample][emitter], axis));
                maxObservedError = std::max (maxObservedError, error);
                rawToleranceBound = rawToleranceBound && error <= tolerance + 1.0e-5f;
            }
        }
    }

    check ("endpoint_preservation", endpointsPreserved, "first and last captured samples survive export");
    check ("ordered_keyframe_times", orderedTimes, "keyframe times are strictly ordered inside the bake window");
    check ("raw_capture_tolerance", rawToleranceBound,
           "max interpolation error=" + std::to_string (maxObservedError)
               + " tolerance=" + std::to_string (tolerance));

    const auto firstExportTrackCount = timeline.getTracks().size();
    recorder.exportToTimeline (timeline);
    check ("idempotent_export", timeline.getTracks().size() == firstExportTrackCount,
           "re-export replaces tracks without increasing track count");

    recorder.resetToIdle();
    recorder.prepare (params, bpm, physicsRateHz, 2);
    for (int i = 0; i < sampleCount; ++i)
    {
        auto frame = makeFrame (i);
        frame[0].position.x += 3.0f;
        frame[1].position.x += 3.0f;
        const double ppq = 1.0 + static_cast<double> (i) / physicsRateHz;
        tickAndTrackAllocations (recorder, frame.data(), 2, ppq, bpm, true,
                                 tickAllocationFree);
    }
    const auto finalFrame = makeFrame (sampleCount - 1);
    tickAndTrackAllocations (recorder, finalFrame.data(), 2, 2.0, bpm, true,
                             tickAllocationFree);
    check ("tick_allocation_free", tickAllocationFree,
           "ordinary heap allocation count remains zero across worker-thread ticks");
    recorder.exportToTimeline (timeline);

    const auto rebakedStart = timeline.evaluateTrack ("bake_pos_x_0", firstTimeSeconds);
    check ("reset_and_rebake", recorder.isExportReady() && rebakedStart.has_value()
              && approximatelyEqual (*rebakedStart, 3.0f),
           "reset permits a second bake that replaces prior track values");
}
} // namespace

int main (int argc, char* argv[])
{
    runLifecycleAndExportProbe();

    std::filesystem::path outputPath;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string (argv[i]) == "--json-output")
            outputPath = argv[++i];

    std::ofstream output;
    if (! outputPath.empty())
    {
        std::error_code error;
        if (outputPath.has_parent_path())
            std::filesystem::create_directories (outputPath.parent_path(), error);
        output.open (outputPath);
        check ("json_artifact_ready", ! error && output.is_open(),
               "machine-readable result path is writable: " + outputPath.string());
    }

    const bool passed = std::all_of (checks.begin(), checks.end(), [] (const auto& result)
    {
        return result.passed;
    });
    const std::string json = makeJsonResult (passed);

    if (output.is_open())
        output << json;
    else if (outputPath.empty())
        std::cout << json;

    std::cout << "SUMMARY " << (passed ? "PASS" : "FAIL")
              << " checks=" << checks.size() << '\n';
    return passed ? 0 : 1;
}
