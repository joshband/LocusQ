#include "PathSystem.h"
#include <algorithm>
#include <cmath>

// ────────────────────────────────────────────────────────────────
//  Helpers
// ────────────────────────────────────────────────────────────────

static constexpr float kTwoPi = 6.28318530717958647692f;
static constexpr float kDegToRad = kTwoPi / 360.0f;
static constexpr float kG = 9.81f;

uint32_t PathSystem::xorshift32(uint32_t s) noexcept
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

float PathSystem::randUnit() noexcept
{
    rngState = xorshift32(rngState);
    // Map [1, 0xFFFFFFFF] to [-1, +1]
    return static_cast<float>(static_cast<int32_t>(rngState)) * (1.0f / 2147483648.0f);
}

void PathSystem::resetWalk(int seed) noexcept
{
    activeSeed = seed;
    rngState   = seed == 0 ? 1u : static_cast<uint32_t>(seed);
    walkPos    = {};
}

// ────────────────────────────────────────────────────────────────
//  Top-level compute
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::compute(const PathParams& params, float dt) noexcept
{
    // Seed change: reset random walk state
    if (params.type == PathType::RandomWalk && params.walkSeed != activeSeed)
        resetWalk(params.walkSeed);

    const float safeDt = std::max(dt, 0.0f);
    timeAcc += safeDt * std::clamp(params.speed, 0.1f, 10.0f);

    Vec3 pos;
    switch (params.type)
    {
        case PathType::Lissajous:   pos = computeLissajous(params);   break;
        case PathType::Orbit:       pos = computeOrbit(params);       break;
        case PathType::Pendulum:    pos = computePendulum(params);     break;
        case PathType::FigureEight: pos = computeFigureEight(params); break;
        case PathType::Helix:       pos = computeHelix(params);       break;
        case PathType::RandomWalk:  pos = stepRandomWalk(params);     break;
        default:                    pos = {};                          break;
    }

    // Velocity by finite difference (avoid divide-by-zero)
    if (safeDt > 1e-9f)
        lastVelocity = { (pos.x - lastPos.x) / safeDt,
                         (pos.y - lastPos.y) / safeDt,
                         (pos.z - lastPos.z) / safeDt };

    lastPos = pos;
    return pos;
}

// ────────────────────────────────────────────────────────────────
//  Lissajous: x=Ax·sin(fa·ω·t + phase), y=Ay·sin(fb·ω·t), z=Az·sin(fc·ω·t)
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::computeLissajous(const PathParams& p) const noexcept
{
    const float period = std::max(p.period, 0.1f);
    const float omega  = kTwoPi / period;
    const float phase  = p.lissPhase * kDegToRad;
    return {
        p.lissAmpX * std::sin(p.lissFreqA * omega * timeAcc + phase),
        p.lissAmpY * std::sin(p.lissFreqB * omega * timeAcc),
        p.lissAmpZ * std::sin(p.lissFreqC * omega * timeAcc)
    };
}

// ────────────────────────────────────────────────────────────────
//  Orbit: ellipse in XZ, static Y offset
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::computeOrbit(const PathParams& p) const noexcept
{
    const float period = std::max(p.period, 0.1f);
    const float theta  = kTwoPi * timeAcc / period;
    return {
        std::max(p.orbitRx, 0.1f) * std::cos(theta),
        p.orbitHeight,
        std::max(p.orbitRz, 0.1f) * std::sin(theta)
    };
}

// ────────────────────────────────────────────────────────────────
//  Pendulum: SHM with ω = √(g/L), swing in chosen plane
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::computePendulum(const PathParams& p) const noexcept
{
    const float L      = std::max(p.pendLength, 0.1f);
    const float omega  = std::sqrt(kG / L);
    const float amp    = std::clamp(p.pendAmp, 0.0f, 180.0f) * kDegToRad;
    const float angle  = amp * std::sin(omega * timeAcc);   // small-angle SHM
    const float swing  = L * std::sin(angle);               // horizontal displacement
    const float drop   = L * (1.0f - std::cos(angle));      // vertical drop (always ≥0)

    switch (p.pendPlane)
    {
        case PathPlane::XZ: return { swing, -drop, 0.0f };
        case PathPlane::XY: return { swing, 0.0f,  drop };
        case PathPlane::YZ: return { 0.0f,  -drop, swing };
        default:            return { swing, -drop, 0.0f };
    }
}

// ────────────────────────────────────────────────────────────────
//  Figure Eight: Lissajous 2:1 ratio
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::computeFigureEight(const PathParams& p) const noexcept
{
    const float period = std::max(p.period, 0.1f);
    const float omega  = kTwoPi / period;
    const float s      = std::max(p.fig8Scale, 0.1f);
    const float a      = s * std::sin(omega * timeAcc);
    const float b      = s * std::sin(2.0f * omega * timeAcc);

    switch (p.fig8Plane)
    {
        case PathPlane::XZ: return { a, 0.0f, b };
        case PathPlane::XY: return { a, b,    0.0f };
        case PathPlane::YZ: return { 0.0f, a, b };
        default:            return { a, 0.0f, b };
    }
}

// ────────────────────────────────────────────────────────────────
//  Helix: circle in XZ + triangle-wave Y (bounded, continuous)
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::computeHelix(const PathParams& p) const noexcept
{
    const float period = std::max(p.period, 0.1f);
    const float theta  = kTwoPi * timeAcc / period;
    const float r      = std::max(p.helixRadius, 0.1f);
    const float pitch  = std::max(p.helixPitch,  0.01f);

    // Triangle wave: rises/falls by pitch per full revolution
    // raw = how much we've risen total
    const float totalRev = timeAcc / period;
    // half-period of the triangle wave = 1 revolution
    const float phase = std::fmod(totalRev, 2.0f);
    float yRaw = (phase < 1.0f) ? phase : (2.0f - phase);   // 0→1→0 triangle
    yRaw *= pitch;

    const float y = (p.helixDir == HelixDir::Down) ? -yRaw : yRaw;

    return {
        r * std::cos(theta),
        y,
        r * std::sin(theta)
    };
}

// ────────────────────────────────────────────────────────────────
//  Random Walk: XorShift32 PRNG, bounce-bounded per axis
// ────────────────────────────────────────────────────────────────

Vec3 PathSystem::stepRandomWalk(const PathParams& p) noexcept
{
    const float step   = std::clamp(p.walkStep, 0.001f, 0.5f);
    const float bounds = std::max(p.walkBounds, 0.1f);

    walkPos.x = std::clamp(walkPos.x + randUnit() * step, -bounds, bounds);
    walkPos.y = std::clamp(walkPos.y + randUnit() * step, -bounds, bounds);
    walkPos.z = std::clamp(walkPos.z + randUnit() * step, -bounds, bounds);

    return walkPos;
}
