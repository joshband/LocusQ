#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>

#if JUCE_MAC
 #include <pthread.h>
#endif

#ifndef LOCUS_HEAD_TRACKING
 #define LOCUS_HEAD_TRACKING 0
#endif

struct alignas(16) HeadTrackingPoseSnapshot
{
    float qx = 0.0f;              // +0
    float qy = 0.0f;              // +4
    float qz = 0.0f;              // +8
    float qw = 1.0f;              // +12
    std::uint64_t timestampMs = 0; // +16
    std::uint32_t seq = 0;        // +24
    std::uint32_t pad = 0;        // +28  (layout compat)
    float angVx = 0.0f;           // +32  rad/s body frame; 0 for v1 packets
    float angVy = 0.0f;           // +36
    float angVz = 0.0f;           // +40
    std::uint32_t sensorLocationFlags = 0; // +44  bits[1:0]=location, bit[2]=hasRotationRate
};                                // = 48 bytes

static_assert (sizeof (HeadTrackingPoseSnapshot) == 48, "HeadTrackingPoseSnapshot size contract");

#if LOCUS_HEAD_TRACKING
class HeadTrackingBridge final
{
public:
    using PoseSnapshot = HeadTrackingPoseSnapshot;

    struct Config
    {
        int port = 19765;
        juce::String bindAddress { "127.0.0.1" };
        int ackPort = 19766;
        juce::String ackAddress { "127.0.0.1" };
        int ackIntervalMs = 100;
    };

    HeadTrackingBridge()
        : HeadTrackingBridge (Config {})
    {
    }

    explicit HeadTrackingBridge (Config bridgeConfig)
        : config (std::move (bridgeConfig))
    {
    }

    ~HeadTrackingBridge()
    {
        stop();
    }

    bool start()
    {
        if (started.load (std::memory_order_acquire))
            return true;

        auto acquired = acquireCore (config);
        if (acquired == nullptr)
            return false;

        core = std::move (acquired);
        corePtr.store (core.get(), std::memory_order_release);
        started.store (true, std::memory_order_release);
        return true;
    }

    void stop()
    {
        if (! started.exchange (false, std::memory_order_acq_rel))
            return;

        corePtr.store (nullptr, std::memory_order_release);
        releaseCore();
        core.reset();
    }

    const PoseSnapshot* currentPose() const noexcept
    {
        if (const auto* sharedCore = corePtr.load (std::memory_order_acquire))
            return sharedCore->currentPose();
        return nullptr;
    }

    std::uint32_t getInvalidPacketCount() const noexcept
    {
        if (const auto* sharedCore = corePtr.load (std::memory_order_acquire))
            return sharedCore->getInvalidPacketCount();
        return 0;
    }

    std::uint32_t getConsumerCount() const noexcept
    {
        if (const auto* sharedCore = corePtr.load (std::memory_order_acquire))
            return sharedCore->getConsumerCount();
        return 0;
    }

private:
    class SharedCore final : private juce::Thread
    {
    public:
        explicit SharedCore (Config bridgeConfig)
            : juce::Thread ("LocusQHeadTrackingBridgeShared"),
              config (std::move (bridgeConfig)),
              sourceToken (static_cast<std::uint32_t> (
                  static_cast<std::uint64_t> (juce::Time::currentTimeMillis())
                  ^ static_cast<std::uint64_t> (juce::Time::getMillisecondCounter())))
        {
            slots[0] = PoseSnapshot {};
            slots[1] = PoseSnapshot {};
        }

        ~SharedCore() override
        {
            stopReceiver();
        }

        bool startReceiver()
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

            ackSocket = std::make_unique<juce::DatagramSocket> (false);
            ackSocket->setEnablePortReuse (true);

            if (! startThread (juce::Thread::Priority::normal))
            {
                socket->shutdown();
                socket.reset();
                ackSocket.reset();
                return false;
            }

            return true;
        }

        void stopReceiver()
        {
            signalThreadShouldExit();

            if (socket != nullptr)
                socket->shutdown();

            if (ackSocket != nullptr)
                ackSocket->shutdown();

            if (isThreadRunning())
                stopThread (500);

            socket.reset();
            ackSocket.reset();
        }

        const PoseSnapshot* currentPose() const noexcept
        {
            return activePose.load (std::memory_order_acquire);
        }

        std::uint32_t getInvalidPacketCount() const noexcept
        {
            return invalidPacketCount.load (std::memory_order_relaxed);
        }

        std::uint32_t getConsumerCount() const noexcept
        {
            return consumerCount.load (std::memory_order_relaxed);
        }

        void setConsumerCount (std::uint32_t count) noexcept
        {
            consumerCount.store (count, std::memory_order_relaxed);
        }

    private:
       #if JUCE_MAC
        static void applySchedulingHint() noexcept
        {
            (void) pthread_set_qos_class_self_np (QOS_CLASS_UTILITY, 0);
        }
       #else
        static void applySchedulingHint() noexcept {}
       #endif

        static constexpr std::uint32_t packetMagic = 0x4C515054u; // "LQPT"
        static constexpr int packetSizeV1 = 36; // v1 actual wire size (fixes prior off-by-4)
        static constexpr int packetSizeV2 = 52; // v2 wire size with angV + sensorLocationFlags
        static constexpr std::uint32_t ackMagic = 0x4C514143u; // "LQAC"
        static constexpr std::uint32_t ackVersion = 1u;
        static constexpr int ackPacketSizeBytes = 48;
        static constexpr std::uint32_t ackFlagPoseAvailable = 1u << 0;
        static constexpr std::uint32_t ackFlagPoseStale = 1u << 1;
        static constexpr std::uint64_t staleThresholdMs = 500;

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

        static void writeU32LE (std::uint8_t* bytes, std::uint32_t value) noexcept
        {
            bytes[0] = static_cast<std::uint8_t> (value & 0xFFu);
            bytes[1] = static_cast<std::uint8_t> ((value >> 8) & 0xFFu);
            bytes[2] = static_cast<std::uint8_t> ((value >> 16) & 0xFFu);
            bytes[3] = static_cast<std::uint8_t> ((value >> 24) & 0xFFu);
        }

        static void writeU64LE (std::uint8_t* bytes, std::uint64_t value) noexcept
        {
            for (int i = 0; i < 8; ++i)
                bytes[i] = static_cast<std::uint8_t> ((value >> (8 * i)) & 0xFFu);
        }

        static void writeF32LE (std::uint8_t* bytes, float value) noexcept
        {
            std::uint32_t raw = 0;
            std::memcpy (&raw, &value, sizeof (raw));
            writeU32LE (bytes, raw);
        }

        static bool decodePacket (const std::uint8_t* bytes, int numBytes, PoseSnapshot& snapshot) noexcept
        {
            if (bytes == nullptr || numBytes < packetSizeV1)
                return false;

            if (readU32LE (bytes + 0) != packetMagic)
                return false;

            const auto version = readU32LE (bytes + 4);

            if (version == 1u && numBytes >= packetSizeV1)
            {
                snapshot.qx          = readF32LE (bytes + 8);
                snapshot.qy          = readF32LE (bytes + 12);
                snapshot.qz          = readF32LE (bytes + 16);
                snapshot.qw          = readF32LE (bytes + 20);
                snapshot.timestampMs = readU64LE (bytes + 24);
                snapshot.seq         = readU32LE (bytes + 32);
                snapshot.pad         = 0;
                snapshot.angVx       = 0.0f;
                snapshot.angVy       = 0.0f;
                snapshot.angVz       = 0.0f;
                snapshot.sensorLocationFlags = 0;
            }
            else if (version == 2u && numBytes >= packetSizeV2)
            {
                snapshot.qx          = readF32LE (bytes + 8);
                snapshot.qy          = readF32LE (bytes + 12);
                snapshot.qz          = readF32LE (bytes + 16);
                snapshot.qw          = readF32LE (bytes + 20);
                snapshot.timestampMs = readU64LE (bytes + 24);
                snapshot.seq         = readU32LE (bytes + 32);
                snapshot.pad         = 0;
                snapshot.angVx       = readF32LE (bytes + 36);
                snapshot.angVy       = readF32LE (bytes + 40);
                snapshot.angVz       = readF32LE (bytes + 44);
                snapshot.sensorLocationFlags = readU32LE (bytes + 48);
            }
            else
            {
                return false;
            }

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

        void sendAckPacket (std::uint64_t nowMs) noexcept
        {
            auto* txSocket = ackSocket.get();
            if (txSocket == nullptr || config.ackPort <= 0 || config.ackAddress.isEmpty())
                return;

            std::array<std::uint8_t, ackPacketSizeBytes> bytes {};
            writeU32LE (bytes.data() + 0, ackMagic);
            writeU32LE (bytes.data() + 4, ackVersion);
            writeU32LE (bytes.data() + 8, sourceToken);
            writeU32LE (bytes.data() + 12, consumerCount.load (std::memory_order_relaxed));
            writeU32LE (bytes.data() + 16, lastSeq.load (std::memory_order_relaxed));
            writeU32LE (bytes.data() + 20, invalidPacketCount.load (std::memory_order_relaxed));

            const auto* pose = activePose.load (std::memory_order_acquire);
            const auto poseTimestampMs = (pose != nullptr ? pose->timestampMs : 0u);
            writeU64LE (bytes.data() + 24, poseTimestampMs);

            std::uint32_t flags = 0;
            float ageMs = 0.0f;
            if (pose != nullptr)
            {
                flags |= ackFlagPoseAvailable;
                if (poseTimestampMs > 0 && nowMs >= poseTimestampMs)
                    ageMs = static_cast<float> (nowMs - poseTimestampMs);
                if (ageMs > static_cast<float> (staleThresholdMs))
                    flags |= ackFlagPoseStale;
            }
            else
            {
                flags |= ackFlagPoseStale;
            }

            writeF32LE (bytes.data() + 32, ageMs);
            writeU32LE (bytes.data() + 36, flags);
            writeU32LE (bytes.data() + 40, static_cast<std::uint32_t> (config.port));
            writeU32LE (bytes.data() + 44, ackCounter.fetch_add (1, std::memory_order_relaxed) + 1);

            const auto bytesWritten = txSocket->write (
                config.ackAddress,
                config.ackPort,
                bytes.data(),
                static_cast<int> (bytes.size()));
            if (bytesWritten <= 0)
                ackSendErrors.fetch_add (1, std::memory_order_relaxed);
        }

        void run() override
        {
            applySchedulingHint();

            auto* udpSocket = socket.get();
            if (udpSocket == nullptr)
                return;

            std::array<std::uint8_t, 64> packetBytes {}; // 64B headroom: v1=36, v2=52
            const auto ackIntervalMs = juce::jmax (20, config.ackIntervalMs);
            auto nextAckTick = juce::Time::getMillisecondCounterHiRes();

            while (! threadShouldExit())
            {
                const auto ready = udpSocket->waitUntilReady (true, 50);
                if (ready > 0)
                {
                    const auto bytesRead = udpSocket->read (
                        packetBytes.data(),
                        static_cast<int> (packetBytes.size()),
                        false);

                    if (bytesRead > 0)
                    {
                        bool shouldPublish = true;
                        PoseSnapshot snapshot {};
                        if (! decodePacket (packetBytes.data(), bytesRead, snapshot))
                        {
                            invalidPacketCount.fetch_add (1, std::memory_order_relaxed);
                            shouldPublish = false;
                        }

                        if (shouldPublish && hasPose.load (std::memory_order_acquire))
                        {
                            const auto previousSeq = lastSeq.load (std::memory_order_relaxed);
                            if (snapshot.seq <= previousSeq)
                            {
                                bool acceptSequenceRestart = false;
                                const auto* currentPose = activePose.load (std::memory_order_acquire);
                                if (currentPose != nullptr)
                                {
                                    const auto currentTimestampMs = currentPose->timestampMs;
                                    const auto nowMs = static_cast<std::uint64_t> (juce::Time::currentTimeMillis());
                                    const bool currentPoseStale = currentTimestampMs == 0
                                        || (nowMs >= (currentTimestampMs + staleThresholdMs));
                                    const bool incomingTimestampAdvanced = snapshot.timestampMs > currentTimestampMs;
                                    // Accept sequence restarts only when the prior
                                    // stream is stale and the sender timestamp has
                                    // moved forward (process restarts).
                                    acceptSequenceRestart = currentPoseStale && incomingTimestampAdvanced;
                                }
                                else
                                {
                                    acceptSequenceRestart = true;
                                }

                                if (! acceptSequenceRestart)
                                    shouldPublish = false;
                            }
                        }

                        if (shouldPublish)
                            publishSnapshot (snapshot);
                    }
                }

                const auto nowTick = juce::Time::getMillisecondCounterHiRes();
                if (nowTick >= nextAckTick)
                {
                    sendAckPacket (static_cast<std::uint64_t> (juce::Time::currentTimeMillis()));
                    nextAckTick = nowTick + static_cast<double> (ackIntervalMs);
                }
            }
        }

        Config config;
        std::unique_ptr<juce::DatagramSocket> socket;
        std::unique_ptr<juce::DatagramSocket> ackSocket;
        PoseSnapshot slots[2] {};
        std::atomic<const PoseSnapshot*> activePose { nullptr };
        std::atomic<int> writeSlot { 0 };
        std::atomic<bool> hasPose { false };
        std::atomic<std::uint32_t> lastSeq { 0 };
        std::atomic<std::uint32_t> invalidPacketCount { 0 };
        std::atomic<std::uint32_t> consumerCount { 0 };
        std::atomic<std::uint32_t> ackCounter { 0 };
        std::atomic<std::uint32_t> ackSendErrors { 0 };
        std::uint32_t sourceToken = 0;
    };

    struct SharedRegistry
    {
        juce::CriticalSection lock;
        std::weak_ptr<SharedCore> core;
        Config activeConfig;
        std::uint32_t consumers = 0;
        bool configInitialized = false;
    };

    static SharedRegistry& registry() noexcept
    {
        static SharedRegistry sharedRegistry;
        return sharedRegistry;
    }

    static bool configMatches (const Config& lhs, const Config& rhs) noexcept
    {
        return lhs.port == rhs.port
            && lhs.bindAddress == rhs.bindAddress
            && lhs.ackPort == rhs.ackPort
            && lhs.ackAddress == rhs.ackAddress
            && lhs.ackIntervalMs == rhs.ackIntervalMs;
    }

    static std::shared_ptr<SharedCore> acquireCore (const Config& requestedConfig)
    {
        auto& sharedRegistry = registry();
        const juce::ScopedLock scopedLock (sharedRegistry.lock);

        auto shared = sharedRegistry.core.lock();
        if (shared != nullptr)
        {
            if (sharedRegistry.configInitialized
                && ! configMatches (requestedConfig, sharedRegistry.activeConfig))
            {
                return nullptr;
            }

            ++sharedRegistry.consumers;
            shared->setConsumerCount (sharedRegistry.consumers);
            return shared;
        }

        auto created = std::shared_ptr<SharedCore> (new SharedCore (requestedConfig));
        if (! created->startReceiver())
            return nullptr;

        sharedRegistry.core = created;
        sharedRegistry.activeConfig = requestedConfig;
        sharedRegistry.configInitialized = true;
        sharedRegistry.consumers = 1;
        created->setConsumerCount (1);
        return created;
    }

    static void releaseCore()
    {
        std::shared_ptr<SharedCore> shared;
        {
            auto& sharedRegistry = registry();
            const juce::ScopedLock scopedLock (sharedRegistry.lock);
            shared = sharedRegistry.core.lock();
            if (shared == nullptr)
                return;

            if (sharedRegistry.consumers > 0)
                --sharedRegistry.consumers;
            shared->setConsumerCount (sharedRegistry.consumers);

            if (sharedRegistry.consumers == 0)
            {
                sharedRegistry.core.reset();
                sharedRegistry.configInitialized = false;
            }
            else
            {
                shared.reset();
            }
        }

        if (shared != nullptr)
            shared->stopReceiver();
    }

    Config config;
    std::shared_ptr<SharedCore> core;
    std::atomic<SharedCore*> corePtr { nullptr };
    std::atomic<bool> started { false };
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

    std::uint32_t getConsumerCount() const noexcept
    {
        return 0;
    }

private:
    Config config;
};
#endif
