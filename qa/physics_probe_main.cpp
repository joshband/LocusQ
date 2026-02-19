// LocusQ Phase 2.4 Physics Acceptance Probe
//
// Deterministic probe for core physics behaviors that are difficult to
// validate purely from audio-domain QA metrics.

#include "Source/PhysicsEngine.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace
{
struct TraceSample
{
    double tSeconds = 0.0;
    PhysicsEngine::PhysicsState state;
};

struct ProbeConfig
{
    Vec3 restPosition { 0.0f, 1.0f, 0.0f };
    Vec3 roomDimensions { 2.0f, 2.0f, 2.0f };
    Vec3 throwVelocity { 0.0f, 0.0f, 0.0f };

    bool requestThrow = false;
    bool wallCollision = true;

    float mass = 1.0f;
    float drag = 0.5f;
    float elasticity = 0.7f;
    float friction = 0.3f;
    float gravityMagnitude = 0.0f;
    int gravityDirection = 0;

    int updateRateIndex = 3; // 240 Hz
    double durationSeconds = 2.0;
    double samplePeriodSeconds = 0.01;
};

struct CheckResult
{
    std::string id;
    bool passed = false;
    std::string detail;
};

float speedMagnitude (const Vec3& v)
{
    return std::sqrt (v.x * v.x + v.y * v.y + v.z * v.z);
}

float positionDistance (const Vec3& a, const Vec3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt (dx * dx + dy * dy + dz * dz);
}

std::vector<TraceSample> runTrace (const ProbeConfig& cfg)
{
    PhysicsEngine engine;
    engine.prepare (48000.0);
    engine.setUpdateRateIndex (cfg.updateRateIndex);
    engine.setPhysicsEnabled (true);
    engine.setPaused (false);
    engine.setWallCollisionEnabled (cfg.wallCollision);
    engine.setRoomDimensions (cfg.roomDimensions);
    engine.setRestPosition (cfg.restPosition);
    engine.setMass (cfg.mass);
    engine.setDrag (cfg.drag);
    engine.setElasticity (cfg.elasticity);
    engine.setFriction (cfg.friction);
    engine.setGravity (cfg.gravityMagnitude, cfg.gravityDirection);

    std::this_thread::sleep_for (std::chrono::milliseconds (30));

    if (cfg.requestThrow)
        engine.requestThrow (cfg.throwVelocity);

    std::vector<TraceSample> trace;
    trace.reserve (static_cast<size_t> (cfg.durationSeconds / cfg.samplePeriodSeconds) + 8);

    const auto start = std::chrono::steady_clock::now();
    while (true)
    {
        std::this_thread::sleep_for (
            std::chrono::duration<double> (cfg.samplePeriodSeconds));

        const auto now = std::chrono::steady_clock::now();
        const double elapsed =
            std::chrono::duration<double> (now - start).count();

        trace.push_back ({ elapsed, engine.getState() });

        if (elapsed >= cfg.durationSeconds)
            break;
    }

    engine.shutdown();
    return trace;
}

float trailingMeanSpeed (const std::vector<TraceSample>& trace, double trailingFraction = 0.25)
{
    if (trace.empty())
        return 0.0f;

    const size_t start = static_cast<size_t> (
        std::floor (static_cast<double> (trace.size()) * (1.0 - trailingFraction)));

    double sum = 0.0;
    size_t count = 0;

    for (size_t i = start; i < trace.size(); ++i)
    {
        sum += speedMagnitude (trace[i].state.velocity);
        ++count;
    }

    return count > 0 ? static_cast<float> (sum / static_cast<double> (count)) : 0.0f;
}

std::optional<float> firstBounceReboundSpeedX (const std::vector<TraceSample>& trace)
{
    int prevSign = 0;
    for (const auto& sample : trace)
    {
        const float vx = sample.state.velocity.x;
        if (std::abs (vx) < 0.25f)
            continue;

        const int sign = vx > 0.0f ? 1 : -1;
        if (prevSign > 0 && sign < 0)
            return std::abs (vx);

        prevSign = sign;
    }

    return std::nullopt;
}

CheckResult checkThrowBounceDecay()
{
    ProbeConfig cfg;
    cfg.restPosition = { 0.0f, 1.0f, 0.0f };
    cfg.roomDimensions = { 2.0f, 2.0f, 2.0f };
    cfg.throwVelocity = { 12.0f, 0.0f, 0.0f };
    cfg.requestThrow = true;
    cfg.wallCollision = true;
    cfg.drag = 1.2f;
    cfg.elasticity = 0.8f;
    cfg.friction = 0.2f;
    cfg.gravityMagnitude = 0.0f;
    cfg.durationSeconds = 2.6;

    const auto trace = runTrace (cfg);
    if (trace.empty())
        return { "throw_bounce_decay", false, "no trace samples captured" };

    float maxDisp = 0.0f;
    float peakSpeed = 0.0f;
    int bounceCount = 0;
    int prevSign = 0;

    for (const auto& sample : trace)
    {
        maxDisp = std::max (maxDisp, std::abs (sample.state.position.x - cfg.restPosition.x));

        const float speed = speedMagnitude (sample.state.velocity);
        peakSpeed = std::max (peakSpeed, speed);

        const float vx = sample.state.velocity.x;
        if (std::abs (vx) > 0.25f)
        {
            const int sign = vx > 0.0f ? 1 : -1;
            if (prevSign != 0 && sign != prevSign)
                ++bounceCount;
            prevSign = sign;
        }
    }

    const float endSpeed = trailingMeanSpeed (trace, 0.2);
    const bool moved = maxDisp > 0.20f;
    const bool bounced = bounceCount >= 1;
    const bool decayed = peakSpeed > 0.0f && endSpeed < (peakSpeed * 0.55f);

    CheckResult result;
    result.id = "throw_bounce_decay";
    result.passed = moved && bounced && decayed;
    result.detail = "max_disp=" + std::to_string (maxDisp)
                  + ", bounce_count=" + std::to_string (bounceCount)
                  + ", peak_speed=" + std::to_string (peakSpeed)
                  + ", end_speed=" + std::to_string (endSpeed);
    return result;
}

CheckResult checkGravityPullsDown()
{
    ProbeConfig cfg;
    cfg.restPosition = { 0.0f, 1.5f, 0.0f };
    cfg.roomDimensions = { 12.0f, 12.0f, 12.0f };
    cfg.requestThrow = false;
    cfg.wallCollision = false;
    cfg.drag = 0.0f;
    cfg.gravityMagnitude = 9.8f;
    cfg.gravityDirection = 0; // Down
    cfg.durationSeconds = 0.9;

    const auto trace = runTrace (cfg);
    if (trace.empty())
        return { "gravity_pulls_down", false, "no trace samples captured" };

    const float yStart = trace.front().state.position.y;
    const float yEnd = trace.back().state.position.y;
    const float vyEnd = trace.back().state.velocity.y;

    const bool movedDown = yEnd < (yStart - 0.05f);
    const bool downwardVelocity = vyEnd < -0.05f;

    CheckResult result;
    result.id = "gravity_pulls_down";
    result.passed = movedDown && downwardVelocity;
    result.detail = "y_start=" + std::to_string (yStart)
                  + ", y_end=" + std::to_string (yEnd)
                  + ", vy_end=" + std::to_string (vyEnd);
    return result;
}

CheckResult checkDragSlowsMotion()
{
    ProbeConfig lowDrag;
    lowDrag.restPosition = { 0.0f, 1.0f, 0.0f };
    lowDrag.roomDimensions = { 20.0f, 20.0f, 20.0f };
    lowDrag.throwVelocity = { 4.0f, 0.0f, 0.0f };
    lowDrag.requestThrow = true;
    lowDrag.wallCollision = false;
    lowDrag.drag = 0.0f;
    lowDrag.gravityMagnitude = 0.0f;
    lowDrag.durationSeconds = 1.4;

    auto highDrag = lowDrag;
    highDrag.drag = 5.0f;

    const auto traceLow = runTrace (lowDrag);
    const auto traceHigh = runTrace (highDrag);

    if (traceLow.empty() || traceHigh.empty())
        return { "drag_slows_motion", false, "missing trace samples" };

    const float endLow = trailingMeanSpeed (traceLow, 0.25);
    const float endHigh = trailingMeanSpeed (traceHigh, 0.25);
    const bool slowed = endHigh < (endLow * 0.5f);

    CheckResult result;
    result.id = "drag_slows_motion";
    result.passed = slowed;
    result.detail = "end_speed_low_drag=" + std::to_string (endLow)
                  + ", end_speed_high_drag=" + std::to_string (endHigh);
    return result;
}

CheckResult checkElasticityBounceRetention()
{
    ProbeConfig lowElastic;
    lowElastic.restPosition = { 0.0f, 1.0f, 0.0f };
    lowElastic.roomDimensions = { 1.6f, 2.0f, 2.0f };
    lowElastic.throwVelocity = { 10.0f, 0.0f, 0.0f };
    lowElastic.requestThrow = true;
    lowElastic.wallCollision = true;
    lowElastic.drag = 0.0f;
    lowElastic.friction = 0.0f;
    lowElastic.elasticity = 0.2f;
    lowElastic.gravityMagnitude = 0.0f;
    lowElastic.durationSeconds = 1.2;

    auto highElastic = lowElastic;
    highElastic.elasticity = 0.9f;

    const auto traceLow = runTrace (lowElastic);
    const auto traceHigh = runTrace (highElastic);

    const auto reboundLow = firstBounceReboundSpeedX (traceLow);
    const auto reboundHigh = firstBounceReboundSpeedX (traceHigh);

    if (! reboundLow.has_value() || ! reboundHigh.has_value())
        return { "elasticity_retains_bounce_energy", false, "bounce not observed in one or both traces" };

    const bool retainsMoreEnergy = *reboundHigh > (*reboundLow + 0.8f);

    CheckResult result;
    result.id = "elasticity_retains_bounce_energy";
    result.passed = retainsMoreEnergy;
    result.detail = "rebound_low_elasticity=" + std::to_string (*reboundLow)
                  + ", rebound_high_elasticity=" + std::to_string (*reboundHigh);
    return result;
}

CheckResult checkZeroGDrift()
{
    ProbeConfig cfg;
    cfg.restPosition = { 0.0f, 1.0f, 0.0f };
    cfg.roomDimensions = { 20.0f, 20.0f, 20.0f };
    cfg.throwVelocity = { 3.0f, 0.8f, -0.5f };
    cfg.requestThrow = true;
    cfg.wallCollision = false;
    cfg.drag = 0.0f;
    cfg.gravityMagnitude = 0.0f;
    cfg.durationSeconds = 1.6;

    const auto trace = runTrace (cfg);
    if (trace.empty())
        return { "zero_g_drift", false, "no trace samples captured" };

    const auto startPos = trace.front().state.position;
    const auto endPos = trace.back().state.position;
    const float displacement = positionDistance (startPos, endPos);

    const float speedStart = speedMagnitude (trace.front().state.velocity);
    const float speedEnd = trailingMeanSpeed (trace, 0.2);

    const bool keepsVelocity = std::abs (speedEnd - speedStart) < 0.35f;
    const bool drifts = displacement > 1.0f;

    CheckResult result;
    result.id = "zero_g_drift";
    result.passed = keepsVelocity && drifts;
    result.detail = "speed_start=" + std::to_string (speedStart)
                  + ", speed_end=" + std::to_string (speedEnd)
                  + ", displacement=" + std::to_string (displacement);
    return result;
}
} // namespace

int main()
{
    const std::vector<CheckResult> checks {
        checkThrowBounceDecay(),
        checkGravityPullsDown(),
        checkDragSlowsMotion(),
        checkElasticityBounceRetention(),
        checkZeroGDrift()
    };

    int passed = 0;
    for (const auto& check : checks)
    {
        std::cout << "CHECK " << check.id
                  << " : " << (check.passed ? "PASS" : "FAIL")
                  << " | " << check.detail << "\n";
        if (check.passed)
            ++passed;
    }

    std::cout << "SUMMARY phase_2_4_physics_probe : "
              << passed << "/" << checks.size() << " checks passed\n";

    return passed == static_cast<int> (checks.size()) ? 0 : 1;
}
