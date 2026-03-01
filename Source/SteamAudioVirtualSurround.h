#pragma once

#include "QuadSpeakerLayout.h"
#include "SpatialRenderer.h"
#include <array>
#include <vector>

// SteamAudioVirtualSurround — RT-safe quad-to-binaural adapter for calibration
// monitoring. Delegates virtual-surround rendering to the shared SpatialRenderer
// so that no additional Steam Audio context or dynamic-library load is required.
//
// Ownership: LocusQAudioProcessor constructs and holds one instance, passing a
// reference to the shared SpatialRenderer.
//
// Thread safety:
//   prepare() — must be called from the message thread before first audio use.
//   applyBlock(), isAvailable() — RT-safe; may be called from the audio thread.
class SteamAudioVirtualSurround
{
public:
    explicit SteamAudioVirtualSurround (SpatialRenderer& renderer) noexcept
        : renderer_ (renderer)
    {}

    // Prepare scratch buffers. Call from the message thread before first use.
    // maxBlockSize must be >= 1.
    void prepare (int maxBlockSize)
    {
        jassert (maxBlockSize >= 1);
        const auto sz = static_cast<size_t> (juce::jmax (1, maxBlockSize));
        zeroPad_.assign (sz, 0.0f);
        preparedBlockSize_ = maxBlockSize;
    }

    // RT-safe. Apply quad-to-binaural virtual surround to the caller-supplied
    // channel pointers.
    //
    // inputChannels: array of at least numInputChannels const float* read pointers,
    //   ordered as host output channels. Channels at indices 0..kQuadSpeakerCount-1
    //   are consumed (FL=0, FR=1, RL=2, RR=3). Channels beyond kQuadSpeakerCount
    //   are ignored. Channels below kQuadSpeakerCount are zero-padded internally.
    //   A null pointer at a valid index is treated as a silent channel.
    //
    // outL, outR: caller-allocated arrays of at least numSamples floats. Written
    //   with the binaural L/R output on success; untouched on failure.
    //
    // listenerOrientation: optional Steam-style listener frame (+X right, +Y up,
    //   -Z ahead). When supplied, the monitoring quad bed is rotated into this
    //   frame before virtual-surround binaural render.
    //
    // layout: reserved for future multi-layout support; only Quadraphonic is used.
    //
    // Returns true when binaural was applied. When false, Steam Audio is
    // unavailable; the caller should apply a fallback (stereo downmix or pass-
    // through to speakers).
    bool applyBlock (const float* const* inputChannels,
                     int numInputChannels,
                     float* outL,
                     float* outR,
                     int numSamples,
                     QuadSpeakerLayout layout = QuadSpeakerLayout::Quadraphonic,
                     const IPLCoordinateSpace3* listenerOrientation = nullptr) noexcept
    {
        juce::ignoreUnused (layout); // Reserved for future multi-layout expansion.

        if (outL == nullptr || outR == nullptr
            || numSamples <= 0 || numSamples > preparedBlockSize_
            || zeroPad_.empty())
            return false;

        // Build a quad channel pointer array; zero-pad missing or null channels.
        const float* const zeroPadPtr = zeroPad_.data();
        for (int ch = 0; ch < kQuadSpeakerCount; ++ch)
        {
            quadPtrs_[static_cast<size_t> (ch)] =
                (inputChannels != nullptr
                 && ch < numInputChannels
                 && inputChannels[ch] != nullptr)
                    ? inputChannels[ch]
                    : zeroPadPtr;
        }

        return renderer_.renderVirtualSurroundForMonitoring (
            quadPtrs_.data(), outL, outR, numSamples, listenerOrientation);
    }

    // RT-safe. Returns true when the Steam Audio backend is ready to render.
    bool isAvailable() const noexcept
    {
        return renderer_.isSteamAudioAvailable();
    }

private:
    SpatialRenderer& renderer_;
    int              preparedBlockSize_ = 0;

    // Pre-allocated zero-padding scratch: used for channels absent in the input.
    std::vector<float> zeroPad_;

    // Quad input channel pointers assembled per applyBlock() call.
    std::array<const float*, kQuadSpeakerCount> quadPtrs_ {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteamAudioVirtualSurround)
};
