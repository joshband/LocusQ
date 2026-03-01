#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <cstring>
#include "SharedPtrAtomicContract.h"

//==============================================================================
// Vec3 - Minimal 3D vector for lock-free scene data
//==============================================================================
struct Vec3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

//==============================================================================
// RoomProfile - Calibration data (read-only after creation)
//==============================================================================
struct SpeakerProfile
{
    static constexpr int NUM_FREQ_BINS = 256;   // log-spaced 20Hz–20kHz

    Vec3  position;
    float distance       = 2.0f;    // meters
    float angle          = 0.0f;    // degrees
    float height         = 1.2f;    // meters
    float delayComp      = 0.0f;    // ms (time-of-arrival compensation)
    float gainTrim       = 0.0f;    // dB (level trim to match reference)
    float frequencyResponse[NUM_FREQ_BINS] = {}; // dB deviation per log-spaced bin
};

struct RoomProfile
{
    std::array<SpeakerProfile, 4> speakers;
    Vec3 dimensions { 6.0f, 4.0f, 3.0f }; // W, D, H meters
    float estimatedRT60  = 0.4f;            // seconds
    Vec3 listenerPos     { 0.0f, 0.0f, 0.0f };
    bool valid           = false;
};

//==============================================================================
// EmitterSlot - Lock-free double-buffered emitter state
//==============================================================================
struct EmitterData
{
    bool    active       = false;
    Vec3    position     { 0.0f, 1.2f, 0.0f };
    Vec3    size         { 0.5f, 0.5f, 0.5f };
    float   gain         = 0.0f;     // dB
    float   spread       = 0.0f;
    float   directivity  = 0.5f;
    Vec3    directivityAim { 0.0f, 0.0f, -1.0f };
    Vec3    velocity     { 0.0f, 0.0f, 0.0f };
    Vec3    force        { 0.0f, 0.0f, 0.0f };
    std::uint8_t collisionMask = 0;
    float   collisionEnergy = 0.0f;
    char    label[32]    = "Emitter";
    uint8_t colorIndex   = 0;
    bool    muted        = false;
    bool    soloed       = false;
    bool    physicsEnabled = false;
};

class EmitterSlot
{
public:
    static constexpr int MAX_SHARED_AUDIO_SAMPLES = 8192;
    struct AudioReadSnapshot
    {
        const float* mono = nullptr;
        int numSamples = 0;
        bool valid = false;
    };

    EmitterSlot() = default;

    // Writer side (Emitter instance, audio thread)
    void write (const EmitterData& data)
    {
        int writeIdx = 1 - readIndex.load (std::memory_order_acquire);
        buffers[writeIdx] = data;
        readIndex.store (writeIdx, std::memory_order_release);
    }

    // Reader side (Renderer instance, audio thread)
    EmitterData read() const
    {
        return buffers[readIndex.load (std::memory_order_acquire)];
    }

    // Audio handoff buffer (single-producer/single-consumer, double-buffered).
    // The writer copies a mono snapshot so renderer never dereferences host-owned
    // pointers from another plugin instance.
    void setAudioBuffer (const float* const* channels, int numChannels, int numSamples)
    {
        if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        {
            clearAudioBuffer();
            return;
        }

        const int samplesToCopy = juce::jlimit (0, MAX_SHARED_AUDIO_SAMPLES, numSamples);
        if (samplesToCopy <= 0)
        {
            clearAudioBuffer();
            return;
        }

        const int writeIdx = 1 - audioReadIndex.load (std::memory_order_acquire);
        auto& writeBuffer = audioBuffers[writeIdx];

        const float norm = 1.0f / static_cast<float> (numChannels);
        for (int i = 0; i < samplesToCopy; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (const auto* channel = channels[ch])
                    sum += channel[i];
            }

            writeBuffer.mono[static_cast<size_t> (i)] = sum * norm;
        }

        writeBuffer.numSamples = samplesToCopy;
        writeBuffer.valid = true;
        audioReadIndex.store (writeIdx, std::memory_order_release);
    }

    void clearAudioBuffer()
    {
        const int writeIdx = 1 - audioReadIndex.load (std::memory_order_acquire);
        auto& writeBuffer = audioBuffers[writeIdx];
        writeBuffer.numSamples = 0;
        writeBuffer.valid = false;
        audioReadIndex.store (writeIdx, std::memory_order_release);
    }

    AudioReadSnapshot readAudioSnapshot() const
    {
        const int readIdx = audioReadIndex.load (std::memory_order_acquire);
        const auto& readBuffer = audioBuffers[readIdx];

        AudioReadSnapshot snapshot;
        snapshot.valid = readBuffer.valid;
        if (! snapshot.valid)
            return snapshot;

        snapshot.mono = readBuffer.mono.data();
        snapshot.numSamples = readBuffer.numSamples;
        return snapshot;
    }

    const float* getAudioMono() const
    {
        return readAudioSnapshot().mono;
    }

    int getAudioNumSamples() const
    {
        return readAudioSnapshot().numSamples;
    }

private:
    struct AudioBufferSnapshot
    {
        std::array<float, MAX_SHARED_AUDIO_SAMPLES> mono {};
        int numSamples = 0;
        bool valid = false;
    };

    std::array<EmitterData, 2> buffers;
    std::atomic<int> readIndex { 0 };

    std::array<AudioBufferSnapshot, 2> audioBuffers;
    std::atomic<int> audioReadIndex { 0 };
};

//==============================================================================
// SceneGraph - Process-wide singleton for inter-instance communication
//==============================================================================
class SceneGraph
{
public:
    static constexpr int MAX_EMITTERS = 256;
    static constexpr uint8_t kSlotStateFree = 0;
    static constexpr uint8_t kSlotStateInitializing = 1;
    static constexpr uint8_t kSlotStateActive = 2;
    static constexpr uint8_t kSlotStateRetiring = 3;

    //--------------------------------------------------------------------------
    // Singleton access
    static SceneGraph& getInstance()
    {
        static SceneGraph instance;
        return instance;
    }

    //--------------------------------------------------------------------------
    // Emitter registration
    int registerEmitter()
    {
        for (int i = 0; i < MAX_EMITTERS; ++i)
        {
            auto& state = slotStates[static_cast<size_t> (i)];
            uint8_t expected = kSlotStateFree;
            if (! state.compare_exchange_strong (expected,
                                                 kSlotStateInitializing,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire))
                continue;

            EmitterData d;
            d.active = true;
            d.colorIndex = seededPaletteIndexForSlot (i);
            snprintf (d.label, sizeof (d.label), "Emitter %d", i + 1);
            slots[static_cast<size_t> (i)].write (d);

            state.store (kSlotStateActive, std::memory_order_release);
            activeEmitterCount.fetch_add (1, std::memory_order_acq_rel);
            return i;
        }
        return -1; // No slots available
    }

    void unregisterEmitter (int slotId)
    {
        if (slotId < 0 || slotId >= MAX_EMITTERS) return;
        auto& state = slotStates[static_cast<size_t> (slotId)];
        uint8_t expected = kSlotStateActive;
        if (! state.compare_exchange_strong (expected,
                                             kSlotStateRetiring,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire))
            return;

        EmitterData d;
        d.active = false;
        slots[static_cast<size_t> (slotId)].write (d);
        slots[static_cast<size_t> (slotId)].clearAudioBuffer();
        state.store (kSlotStateFree, std::memory_order_release);

        int currentCount = activeEmitterCount.load (std::memory_order_acquire);
        while (currentCount > 0
               && ! activeEmitterCount.compare_exchange_weak (currentCount,
                                                              currentCount - 1,
                                                              std::memory_order_acq_rel,
                                                              std::memory_order_acquire))
        {
        }
    }

    //--------------------------------------------------------------------------
    // Renderer registration (only one allowed)
    bool registerRenderer()
    {
        bool expected = false;
        return rendererRegistered.compare_exchange_strong (expected,
                                                           true,
                                                           std::memory_order_acq_rel,
                                                           std::memory_order_acquire);
    }

    void unregisterRenderer()
    {
        rendererRegistered.store (false, std::memory_order_release);
    }

    bool isRendererRegistered() const { return rendererRegistered.load (std::memory_order_acquire); }

    //--------------------------------------------------------------------------
    // Slot access
    EmitterSlot& getSlot (int id) { return slots[static_cast<size_t> (id)]; }
    const EmitterSlot& getSlot (int id) const { return slots[static_cast<size_t> (id)]; }
    bool isSlotActive (int id) const
    {
        if (id < 0 || id >= MAX_EMITTERS)
            return false;
        return slotStates[static_cast<size_t> (id)].load (std::memory_order_acquire) == kSlotStateActive;
    }
    int getActiveEmitterCount() const { return activeEmitterCount.load(); }

    //--------------------------------------------------------------------------
    // Room Profile (atomic pointer swap for thread safety)
    void setRoomProfile (const RoomProfile& profile)
    {
        auto newProfile = std::make_shared<RoomProfile> (profile);
        currentRoomProfile.store (std::move (newProfile));
    }

    std::shared_ptr<RoomProfile> getRoomProfile() const
    {
        return currentRoomProfile.load();
    }

    //--------------------------------------------------------------------------
    // Global sample counter for sync
    void advanceSampleCounter (int numSamples)
    {
        globalSampleCounter.fetch_add (static_cast<uint64_t> (numSamples), std::memory_order_relaxed);
    }

    uint64_t getSampleCounter() const
    {
        return globalSampleCounter.load (std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // Global physics controls (written by renderer, read by emitters)
    void setPhysicsRateIndex (int index)
    {
        physicsRateIndex.store (juce::jlimit (0, 3, index), std::memory_order_release);
    }

    int getPhysicsRateIndex() const
    {
        return physicsRateIndex.load (std::memory_order_acquire);
    }

    void setPhysicsPaused (bool paused)
    {
        physicsPaused.store (paused, std::memory_order_release);
    }

    bool isPhysicsPaused() const
    {
        return physicsPaused.load (std::memory_order_acquire);
    }

    void setPhysicsWallCollisionEnabled (bool enabled)
    {
        physicsWallCollisionEnabled.store (enabled, std::memory_order_release);
    }

    bool isPhysicsWallCollisionEnabled() const
    {
        return physicsWallCollisionEnabled.load (std::memory_order_acquire);
    }

    void setPhysicsInteractionEnabled (bool enabled)
    {
        physicsInteractionEnabled.store (enabled, std::memory_order_release);
    }

    bool isPhysicsInteractionEnabled() const
    {
        return physicsInteractionEnabled.load (std::memory_order_acquire);
    }

private:
    static uint8_t seededPaletteIndexForSlot (int slotId) noexcept
    {
        // Deterministic pseudo-random index to spread new emitters across the 16-color palette.
        const auto s = static_cast<uint32_t> (juce::jmax (0, slotId) + 1);
        uint32_t x = (s * 0x9e3779b1u) ^ 0x7f4a7c15u;
        x ^= (x >> 16);
        x *= 0x85ebca6bu;
        x ^= (x >> 13);
        x *= 0xc2b2ae35u;
        x ^= (x >> 16);
        return static_cast<uint8_t> (x % 16u);
    }

    SceneGraph()
    {
        for (auto& state : slotStates)
            state.store (kSlotStateFree, std::memory_order_relaxed);
    }
    ~SceneGraph() = default;

    SceneGraph (const SceneGraph&) = delete;
    SceneGraph& operator= (const SceneGraph&) = delete;

    std::array<EmitterSlot, MAX_EMITTERS> slots;
    std::array<std::atomic<uint8_t>, MAX_EMITTERS> slotStates;
    std::atomic<int> activeEmitterCount { 0 };
    std::atomic<bool> rendererRegistered { false };

    SharedPtrAtomicContract<RoomProfile> currentRoomProfile { std::make_shared<RoomProfile>() };

    std::atomic<uint64_t> globalSampleCounter { 0 };

    std::atomic<int> physicsRateIndex { 1 }; // 0=30,1=60,2=120,3=240 Hz
    std::atomic<bool> physicsPaused { false };
    std::atomic<bool> physicsWallCollisionEnabled { true };
    std::atomic<bool> physicsInteractionEnabled { false };

};
