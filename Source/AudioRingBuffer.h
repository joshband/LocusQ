#pragma once

#include <algorithm>
#include <array>
#include <atomic>

//==============================================================================
/**
 * AudioRingBuffer - lock-free single-producer/single-consumer audio ring buffer.
 *
 * Audio thread writes per-block audio data into the ring via write().
 * ChoreographyWorker reads or discards data from the worker thread via read() /
 * discardAll().
 *
 * Fixed-capacity storage is pre-allocated — no heap allocation during operation.
 * Samples are stored interleaved: [f0_ch0, f0_ch1, ..., f1_ch0, f1_ch1, ...].
 *
 * Threading contract:
 * - write()      — audio thread only (single producer).
 * - read()       — worker thread only (single consumer).
 * - discardAll() — worker thread only.
 * - availableToRead() / getChannelCount() — any thread (acquire load).
 */
template <int kCapacityFrames = 4096, int kMaxChannels = 2>
class AudioRingBuffer
{
public:
    /** Write non-interleaved audio block into the ring.
     *  Drops frames silently if the ring is full (audio thread must not block).
     *  Returns number of frames actually written. */
    int write (const float* const* channelData, int numChannels, int numFrames) noexcept
    {
        const int channels = std::min (numChannels, kMaxChannels);
        const int wPos     = writeHead.load (std::memory_order_relaxed);
        const int rPos     = readHead.load  (std::memory_order_acquire);
        const int toWrite  = std::min (numFrames, freeSlots (wPos, rPos));

        for (int f = 0; f < toWrite; ++f)
        {
            const int slot = (wPos + f) % kCapacityFrames;
            for (int c = 0; c < channels; ++c)
                storage[static_cast<std::size_t> (slot * kMaxChannels + c)] = channelData[c][f];
            for (int c = channels; c < kMaxChannels; ++c)
                storage[static_cast<std::size_t> (slot * kMaxChannels + c)] = 0.0f;
        }

        channelCountStore.store (channels, std::memory_order_relaxed);
        writeHead.store ((wPos + toWrite) % kCapacityFrames, std::memory_order_release);
        return toWrite;
    }

    /** Read up to maxFrames into per-channel destination buffers.
     *  Returns number of frames read. */
    int read (float* const* dest, int maxChannels, int maxFrames) noexcept
    {
        const int channels = std::min (maxChannels, kMaxChannels);
        const int rPos     = readHead.load  (std::memory_order_relaxed);
        const int wPos     = writeHead.load (std::memory_order_acquire);
        const int toRead   = std::min (maxFrames, filledSlots (wPos, rPos));

        for (int f = 0; f < toRead; ++f)
        {
            const int slot = (rPos + f) % kCapacityFrames;
            for (int c = 0; c < channels; ++c)
                dest[c][f] = storage[static_cast<std::size_t> (slot * kMaxChannels + c)];
        }

        readHead.store ((rPos + toRead) % kCapacityFrames, std::memory_order_release);
        return toRead;
    }

    /** Discard all buffered frames. Called from the worker thread to drain the ring
     *  without copying data (used when feature extraction is not yet active). */
    void discardAll() noexcept
    {
        const int wPos = writeHead.load (std::memory_order_acquire);
        readHead.store (wPos, std::memory_order_release);
    }

    /** Number of frames available to read. */
    int availableToRead() const noexcept
    {
        return filledSlots (writeHead.load (std::memory_order_acquire),
                            readHead.load  (std::memory_order_acquire));
    }

    /** Number of channels written by the most recent write() call. */
    int getChannelCount() const noexcept
    {
        return channelCountStore.load (std::memory_order_acquire);
    }

private:
    static int filledSlots (int w, int r) noexcept
    {
        return (w >= r) ? (w - r) : (kCapacityFrames - r + w);
    }

    static int freeSlots (int w, int r) noexcept
    {
        return kCapacityFrames - filledSlots (w, r) - 1;
    }

    std::array<float, kCapacityFrames * kMaxChannels> storage {};
    std::atomic<int> writeHead        { 0 };
    std::atomic<int> readHead         { 0 };
    std::atomic<int> channelCountStore { 0 };
};
