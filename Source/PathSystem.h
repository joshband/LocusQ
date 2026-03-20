#pragma once

#include "SceneGraph.h"
#include <array>
#include <cstdint>

enum class PathType : int
{
    Lissajous  = 0,
    Orbit      = 1,
    Pendulum   = 2,
    FigureEight = 3,
    Helix      = 4,
    RandomWalk = 5
};

enum class PathPlane : int { XZ = 0, XY = 1, YZ = 2 };
enum class HelixDir  : int { Up  = 0, Down = 1 };

struct PathParams
{
    PathType  type         = PathType::Orbit;
    float     period       = 4.0f;    // s — Lissajous/Orbit/FigureEight/Helix
    float     speed        = 1.0f;    // time multiplier

    // Lissajous
    float     lissFreqA    = 3.0f;
    float     lissFreqB    = 2.0f;
    float     lissFreqC    = 1.0f;
    float     lissAmpX     = 2.0f;   // m
    float     lissAmpY     = 0.0f;   // m
    float     lissAmpZ     = 2.0f;   // m
    float     lissPhase    = 0.0f;   // deg

    // Orbit
    float     orbitRx      = 3.0f;   // m
    float     orbitRz      = 3.0f;   // m
    float     orbitHeight  = 0.0f;   // m

    // Pendulum
    float     pendLength   = 2.0f;   // m
    float     pendAmp      = 45.0f;  // deg
    PathPlane pendPlane    = PathPlane::XZ;

    // Figure Eight
    float     fig8Scale    = 2.0f;   // m
    PathPlane fig8Plane    = PathPlane::XZ;

    // Helix
    float     helixRadius  = 2.0f;   // m
    float     helixPitch   = 1.0f;   // m/rev
    HelixDir  helixDir     = HelixDir::Up;

    // Random Walk
    float     walkStep     = 0.05f;  // m/tick
    float     walkBounds   = 5.0f;   // m (symmetric XYZ)
    int       walkSeed     = 0;
};

class PathSystem
{
public:
    // Advance internal time by dt (seconds) and compute new position.
    // All emitters share the same path; formation offsets are additive on top.
    Vec3 compute(const PathParams& params, float dt) noexcept;

    Vec3  getLastVelocity() const noexcept { return lastVelocity; }

    // Called when walk seed changes to re-seed the PRNG.
    void resetWalk(int seed) noexcept;

private:
    float    timeAcc    = 0.0f;  // accumulated path time (s)

    // Last position for velocity finite-difference
    Vec3     lastPos    {};
    Vec3     lastVelocity {};

    // Random Walk state
    uint32_t rngState   = 1u;
    Vec3     walkPos    {};
    int      activeSeed = -1;

    // Path-specific helpers
    Vec3 computeLissajous  (const PathParams& p) const noexcept;
    Vec3 computeOrbit      (const PathParams& p) const noexcept;
    Vec3 computePendulum   (const PathParams& p) const noexcept;
    Vec3 computeFigureEight(const PathParams& p) const noexcept;
    Vec3 computeHelix      (const PathParams& p) const noexcept;
    Vec3 stepRandomWalk    (const PathParams& p) noexcept;

    static uint32_t xorshift32(uint32_t s) noexcept;
    // Returns float in [-1, +1]
    float randUnit() noexcept;
};
