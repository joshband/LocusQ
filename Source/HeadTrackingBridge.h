#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>

#ifndef LOCUS_HEAD_TRACKING
 #define LOCUS_HEAD_TRACKING 0
#endif

struct alignas(16) HeadTrackingPoseSnapshot
{
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;
    float qw = 1.0f;
    std::uint64_t timestampMs = 0;
    std::uint32_t seq = 0;
    std::uint32_t pad = 0;
};

static_assert (sizeof (HeadTrackingPoseSnapshot) == 32, "HeadTrackingPoseSnapshot size contract");

#if LOCUS_HEAD_TRACKING
class HeadTrackingBridge final : private juce::Thread
{
public:
    using PoseSnapshot = HeadTrackingPoseSnapshot;

    struct Config
    {
        int port = 19765;
        juce::String bindAddress { "127.0.0.1" };
    };

    HeadTrackingBridge()
        : HeadTrackingBridge (Config {})
    {
    }

    explicit HeadTrackingBridge (Config bridgeConfig)
        : juce::Thread ("LocusQHeadTrackingBridge"),
          config (std::move (bridgeConfig))
    {
        slots[0] = PoseSnapshot {};
        slots[1] = PoseSnapshot {};
    }

    ~HeadTrackingBridge() override
    {
        stop();
    }

    bool start()
    {
        if (isThreadRunning())
            return true;

        invalidPacketCount.store (0, std::memory_order_relaxed);

        socket = std::make_unique<juce::DatagramSocket> (false);
        socket->setEnablePortReuse (true);

        bool bound = false;
        if (config.bindAddress.isNotEmpty())
            bound = socket->bindToPort (config.port, config.bindAddress);

        if (! bound)
            bound = socket->bindToPort (config.port);

        if (! bound)
        {
            socket.reset();
            return false;
        }

        if (! startThread (juce::Thread::Priority::normal))
        {
            socket->shutdown();
            socket.reset();
            return false;
        }

        return true;
    }

    void stop()
    {
        signalThreadShouldExit();

        if (socket != nullptr)
            socket->shutdown();

        if (isThreadRunning())
            stopThread (500);

        socket.reset();
    }

    const PoseSnapshot* currentPose() const noexcept
    {
        return activePose.load (std::memory_order_acquire);
    }

    std::uint32_t getInvalidPacketCount() const noexcept
    {
        return invalidPacketCount.load (std::memory_order_relaxed);
    }

private:
    static constexpr std::uint32_t packetMagic = 0x4C515054u; // "LQPT"
    static constexpr std::uint32_t packetVersion = 1u;
    static constexpr int packetSizeBytes = 40;

    static std::uint32_t readU32LE (const std::uint8_t* bytes) noexcept
    {
        return static_cast<std::uint32_t> (bytes[0])
             | (static_cast<std::uint32_t> (bytes[1]) << 8)
             | (static_cast<std::uint32_t> (bytes[2]) << 16)
             | (static_cast<std::uint32_t> (bytes[3]) << 24);
    }

    static std::uint64_t readU64LE (const std::uint8_t* bytes) noexcept
    {
        std::uint64_t value = 0;
        for (int i = 0; i < 8; ++i)
            value |= (static_cast<std::uint64_t> (bytes[i]) << (8 * i));
        return value;
    }

    static float readF32LE (const std::uint8_t* bytes) noexcept
    {
        const auto raw = readU32LE (bytes);
        float value = 0.0f;
        std::memcpy (&value, &raw, sizeof (value));
        return value;
    }

    static bool decodePacket (const std::uint8_t* bytes, int numBytes, PoseSnapshot& snapshot) noexcept
    {
        if (bytes == nullptr || numBytes < packetSizeBytes)
            return false;

        if (readU32LE (bytes + 0) != packetMagic || readU32LE (bytes + 4) != packetVersion)
            return false;

        snapshot.qx = readF32LE (bytes + 8);
        snapshot.qy = readF32LE (bytes + 12);
        snapshot.qz = readF32LE (bytes + 16);
        snapshot.qw = readF32LE (bytes + 20);
        snapshot.timestampMs = readU64LE (bytes + 24);
        snapshot.seq = readU32LE (bytes + 32);
        snapshot.pad = 0;

        if (! std::isfinite (snapshot.qx)
            || ! std::isfinite (snapshot.qy)
            || ! std::isfinite (snapshot.qz)
            || ! std::isfinite (snapshot.qw))
        {
            return false;
        }

        const auto normSq = (snapshot.qx * snapshot.qx)
                          + (snapshot.qy * snapshot.qy)
                          + (snapshot.qz * snapshot.qz)
                          + (snapshot.qw * snapshot.qw);

        if (! std::isfinite (normSq) || normSq < 1.0e-12f)
            return false;

        const auto invNorm = 1.0f / std::sqrt (normSq);
        snapshot.qx *= invNorm;
        snapshot.qy *= invNorm;
        snapshot.qz *= invNorm;
        snapshot.qw *= invNorm;
        return true;
    }

    void publishSnapshot (const PoseSnapshot& snapshot) noexcept
    {
        const auto slot = writeSlot.load (std::memory_order_relaxed);
        slots[slot] = snapshot;
        activePose.store (&slots[slot], std::memory_order_release);
        writeSlot.store (slot ^ 1, std::memory_order_relaxed);
        hasPose.store (true, std::memory_order_release);
        lastSeq.store (snapshot.seq, std::memory_order_relaxed);
    }

    void run() override
    {
        auto* udpSocket = socket.get();
        if (udpSocket == nullptr)
            return;

        std::array<std::uint8_t, packetSizeBytes> packetBytes {};
        while (! threadShouldExit())
        {
            const auto ready = udpSocket->waitUntilReady (true, 50);
            if (ready <= 0)
                continue;

            const auto bytesRead = udpSocket->read (packetBytes.data(), static_cast<int> (packetBytes.size()), false);
            if (bytesRead <= 0)
                continue;

            PoseSnapshot snapshot {};
            if (! decodePacket (packetBytes.data(), bytesRead, snapshot))
            {
                invalidPacketCount.fetch_add (1, std::memory_order_relaxed);
                continue;
            }

            if (hasPose.load (std::memory_order_acquire))
            {
                const auto previousSeq = lastSeq.load (std::memory_order_relaxed);
                if (snapshot.seq <= previousSeq)
                    continue;
            }

            publishSnapshot (snapshot);
        }
    }

    Config config;
    std::unique_ptr<juce::DatagramSocket> socket;
    PoseSnapshot slots[2] {};
    std::atomic<const PoseSnapshot*> activePose { nullptr };
    std::atomic<int> writeSlot { 0 };
    std::atomic<bool> hasPose { false };
    std::atomic<std::uint32_t> lastSeq { 0 };
    std::atomic<std::uint32_t> invalidPacketCount { 0 };
};
#else
class HeadTrackingBridge final
{
public:
    using PoseSnapshot = HeadTrackingPoseSnapshot;

    struct Config
    {
        int port = 19765;
        juce::String bindAddress { "127.0.0.1" };
    };

    HeadTrackingBridge() = default;

    explicit HeadTrackingBridge (Config bridgeConfig)
        : config (std::move (bridgeConfig))
    {
    }

    bool start() noexcept
    {
        return false;
    }

    void stop() noexcept {}

    const PoseSnapshot* currentPose() const noexcept
    {
        return nullptr;
    }

    std::uint32_t getInvalidPacketCount() const noexcept
    {
        return 0;
    }

private:
    Config config;
};
#endif
