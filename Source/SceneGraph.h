#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <cstring>

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
    Vec3 position;
    float distance       = 2.0f;    // meters
    float angle          = 0.0f;    // degrees
    float height         = 1.2f;    // meters
    float delayComp      = 0.0f;    // ms
    float gainTrim       = 0.0f;    // dB
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
    char    label[32]    = "Emitter";
    uint8_t colorIndex   = 0;
    bool    muted        = false;
    bool    soloed       = false;
    bool    physicsEnabled = false;
};

class EmitterSlot
{
public:
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

    // Audio buffer pointer (valid only during processBlock)
    void setAudioBuffer (const float* const* channels, int numChannels, int numSamples)
    {
        audioChannels = channels;
        audioNumChannels = numChannels;
        audioNumSamples = numSamples;
    }

    void clearAudioBuffer()
    {
        audioChannels = nullptr;
        audioNumChannels = 0;
        audioNumSamples = 0;
    }

    const float* const* getAudioChannels() const { return audioChannels; }
    int getAudioNumChannels() const { return audioNumChannels; }
    int getAudioNumSamples() const { return audioNumSamples; }

private:
    std::array<EmitterData, 2> buffers;
    std::atomic<int> readIndex { 0 };

    // Audio buffer pointers (only valid during processBlock cycle)
    const float* const* audioChannels = nullptr;
    int audioNumChannels = 0;
    int audioNumSamples = 0;
};

//==============================================================================
// SceneGraph - Process-wide singleton for inter-instance communication
//==============================================================================
class SceneGraph
{
public:
    static constexpr int MAX_EMITTERS = 256;

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
        juce::SpinLock::ScopedLockType lock (registrationLock);
        for (int i = 0; i < MAX_EMITTERS; ++i)
        {
            if (! slotOccupied[i])
            {
                slotOccupied[i] = true;
                EmitterData d;
                d.active = true;
                d.colorIndex = static_cast<uint8_t> (i % 16);
                snprintf (d.label, sizeof (d.label), "Emitter %d", i + 1);
                slots[i].write (d);
                ++activeEmitterCount;
                return i;
            }
        }
        return -1; // No slots available
    }

    void unregisterEmitter (int slotId)
    {
        if (slotId < 0 || slotId >= MAX_EMITTERS) return;
        juce::SpinLock::ScopedLockType lock (registrationLock);
        EmitterData d;
        d.active = false;
        slots[slotId].write (d);
        slots[slotId].clearAudioBuffer();
        slotOccupied[slotId] = false;
        --activeEmitterCount;
    }

    //--------------------------------------------------------------------------
    // Renderer registration (only one allowed)
    bool registerRenderer()
    {
        juce::SpinLock::ScopedLockType lock (registrationLock);
        if (rendererRegistered) return false;
        rendererRegistered = true;
        return true;
    }

    void unregisterRenderer()
    {
        juce::SpinLock::ScopedLockType lock (registrationLock);
        rendererRegistered = false;
    }

    bool isRendererRegistered() const { return rendererRegistered; }

    //--------------------------------------------------------------------------
    // Slot access
    EmitterSlot& getSlot (int id) { return slots[id]; }
    const EmitterSlot& getSlot (int id) const { return slots[id]; }
    bool isSlotActive (int id) const { return slotOccupied[id]; }
    int getActiveEmitterCount() const { return activeEmitterCount.load(); }

    //--------------------------------------------------------------------------
    // Room Profile (atomic pointer swap for thread safety)
    void setRoomProfile (const RoomProfile& profile)
    {
        auto newProfile = std::make_shared<RoomProfile> (profile);
        std::atomic_store (&currentRoomProfile, newProfile);
    }

    std::shared_ptr<RoomProfile> getRoomProfile() const
    {
        return std::atomic_load (&currentRoomProfile);
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

private:
    SceneGraph() = default;
    ~SceneGraph() = default;

    SceneGraph (const SceneGraph&) = delete;
    SceneGraph& operator= (const SceneGraph&) = delete;

    std::array<EmitterSlot, MAX_EMITTERS> slots;
    std::array<bool, MAX_EMITTERS> slotOccupied {};
    std::atomic<int> activeEmitterCount { 0 };
    bool rendererRegistered = false;

    std::shared_ptr<RoomProfile> currentRoomProfile = std::make_shared<RoomProfile>();

    std::atomic<uint64_t> globalSampleCounter { 0 };

    juce::SpinLock registrationLock;
};
