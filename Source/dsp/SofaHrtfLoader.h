/**
 * SofaHrtfLoader.h
 *
 * SOFA HRTF loader infrastructure for LocusQ.
 *
 * This header provides:
 *   - SofaHrirResult: a lightweight value type holding a loaded SOFA handle
 *     and metadata, via libmysofa's MYSOFA_EASY API.
 *   - loadSofaFile(): a free function that opens a SOFA file at a given path,
 *     resamples to targetSampleRate, and returns a SofaHrirResult.
 *
 * IMPORTANT — include discipline:
 *   Do NOT include this header from SpatialRenderer.h or any translation unit
 *   that also includes JUCE headers. libmysofa defines C symbols that can clash
 *   with JUCE's global namespace in PCH or unity-build configurations.
 *
 *   Include it only from Source/dsp/SofaHrtfLoader.cpp (the isolated compilation
 *   unit registered in CMakeLists.txt via target_sources).
 *
 *   SpatialRenderer.h interacts with SOFA loading through the SteamAudio HRTF
 *   swap hook described in the TODO(Task 13) comment in initSteamAudioRuntime().
 *
 * libmysofa API reference: https://github.com/hoene/libmysofa
 */

#pragma once

#ifdef LOCUSQ_ENABLE_SOFA

// mysofa.h is a C header; guard against C++ name-mangling.
extern "C"
{
#include <mysofa.h>
}

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace locusq::dsp
{

/**
 * Holds the result of a successful (or failed) SOFA file load.
 *
 * On success, `valid` is true, `handle` is non-null, and `sampleRate` /
 * `firLength` reflect the resampled filter configuration.
 *
 * On failure, `valid` is false and all numeric fields are zero / default.
 * Callers must check `valid` before using any other fields.
 */
struct SofaHrirResult
{
    /// True if the file was loaded and the handle is usable.
    bool valid = false;

    /// The sample rate at which the FIR taps are stored (== targetSampleRate
    /// passed to loadSofaFile, if resampling succeeded).
    int sampleRate = 48000;

    /// Number of taps per FIR filter. 0 on failure.
    int firLength = 0;

    /**
     * Performs a nearest-neighbour lookup for the HRIRs at the given azimuth
     * and elevation and returns the FIR taps for the requested channel (0=left,
     * 1=right).
     *
     * Returns an empty vector if `valid` is false, the channel is out of range,
     * the lookup fails, or any returned tap is non-finite (NaN/Inf).
     * Non-finite taps violate the BL-046 ir_non_finite_count == 0 invariant;
     * an empty return here lets the caller fall back gracefully.
     *
     * @param azDeg   Azimuth in degrees  (spherical coordinate, horizontal plane).
     * @param elDeg   Elevation in degrees (spherical coordinate, above/below horizon).
     * @param channel 0 = left ear, 1 = right ear.
     */
    std::vector<float> getHrir (float azDeg, float elDeg, int channel) const
    {
        if (! valid || handle == nullptr || channel < 0 || channel > 1)
            return {};

        // mysofa_getfilter_float expects Cartesian (x, y, z) coordinates, not
        // degrees.  Convert spherical {phi_deg, theta_deg, r=1} -> {x, y, z}
        // in-place using the libmysofa helper before the lookup.
        float coords[3] = { azDeg, elDeg, 1.0f };
        mysofa_s2c (coords); // in-place: {phi_deg, theta_deg, r} -> {x, y, z}

        // mysofa_getfilter_float fills leftIR / rightIR in-place; we need
        // temporary buffers sized to firLength.
        std::vector<float> leftIR  (static_cast<std::size_t> (firLength), 0.0f);
        std::vector<float> rightIR (static_cast<std::size_t> (firLength), 0.0f);
        float leftDelay  = 0.0f;
        float rightDelay = 0.0f;

        mysofa_getfilter_float (handle.get(),
                                coords[0], coords[1], coords[2],
                                leftIR.data(), rightIR.data(),
                                &leftDelay, &rightDelay);

        // Guard: reject any result containing non-finite (NaN/Inf) taps.
        // This satisfies the BL-046 invariant ir_non_finite_count == 0.
        // Returning {} signals the caller to fall back gracefully.
        auto result = (channel == 0) ? leftIR : rightIR;
        for (float v : result)
            if (! std::isfinite (v))
                return {};

        return result;
    }

    /// The libmysofa handle. Closed automatically on destruction.
    std::unique_ptr<MYSOFA_EASY, decltype (&mysofa_close)> handle { nullptr, mysofa_close };
};

/**
 * Opens a SOFA file at `path`, requests resampling to `targetSampleRate`, and
 * returns a SofaHrirResult.
 *
 * Thread safety: loadSofaFile() is not thread-safe with respect to concurrent
 * calls on the same path; call it from a dedicated loader thread or the message
 * thread, never from the audio callback.
 *
 * @param path             Absolute filesystem path to the .sofa file.
 * @param targetSampleRate Desired output sample rate for the FIR filters.
 * @return SofaHrirResult  valid=true on success, valid=false on any failure.
 */
inline SofaHrirResult loadSofaFile (const std::string& path, int targetSampleRate)
{
    SofaHrirResult result;
    result.sampleRate = targetSampleRate;

    int err = MYSOFA_OK;
    auto* easy = mysofa_open (path.c_str(),
                              static_cast<float> (targetSampleRate),
                              &result.firLength,
                              &err);

    if (easy == nullptr || err != MYSOFA_OK)
    {
        // mysofa_open may have written a non-zero value to firLength before
        // failing; reset it so the caller sees a clean zero on the error path.
        result.firLength = 0;
        // Leave result.valid = false; caller may log err for diagnostics.
        return result;
    }

    result.handle.reset (easy);
    result.valid = true;
    return result;
}

} // namespace locusq::dsp

#endif // LOCUSQ_ENABLE_SOFA
