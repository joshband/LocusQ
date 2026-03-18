#pragma once

#include "PhysicsDSPBridge.h"
#include "PhysicsWorker.h"

#include <atomic>
#include <mutex>

//==============================================================================
/**
 * PhysicsSharedRuntime - process-wide owner for coordinated physics runtime.
 *
 * Mirrors the existing SceneGraph singleton pattern so multiple plugin
 * instances can participate in one shared PhysicsWorker / PhysicsDSPBridge
 * authority domain inside a single host process.
 */
class PhysicsSharedRuntime
{
public:
    static PhysicsSharedRuntime& getInstance()
    {
        static PhysicsSharedRuntime instance;
        return instance;
    }

    void acquire (double sampleRate, int rateIndex)
    {
        const std::lock_guard<std::mutex> lock (lifecycleMutex);

        const auto previousUsers = activeUsers.fetch_add (1, std::memory_order_acq_rel);
        if (previousUsers == 0)
            worker.attachDSPBridge (&dspBridge);

        dspBridge.prepare (sampleRate, worker.getPeriodMs() * 0.001);
        worker.prepare (rateIndex);
    }

    void release()
    {
        const std::lock_guard<std::mutex> lock (lifecycleMutex);

        const auto previousUsers = activeUsers.load (std::memory_order_acquire);
        if (previousUsers <= 0)
            return;

        if (activeUsers.fetch_sub (1, std::memory_order_acq_rel) == 1)
            worker.shutdown();
    }

    PhysicsDSPBridge& getDspBridge() noexcept { return dspBridge; }
    PhysicsWorker& getWorker() noexcept { return worker; }

private:
    PhysicsSharedRuntime() = default;

    std::mutex lifecycleMutex;
    std::atomic<int> activeUsers { 0 };
    PhysicsDSPBridge dspBridge;
    PhysicsWorker worker;
};
