#include "FormationSystem.h"

#include <algorithm>
#include <cmath>

static constexpr float kPi     = 3.14159265358979323846f;
static constexpr float kTwoPi  = 2.0f * kPi;
static constexpr float kDegRad = kPi / 180.0f;

//==============================================================================
void FormationSystem::compute (const FormationParams& params,
                               int numSlots,
                               float morphPhase) noexcept
{
    const int n = std::clamp (numSlots, 0, kMaxSlots);
    morphPhase  = std::clamp (morphPhase, 0.0f, 1.0f);

    // Compute analytical slot positions into a temp array on the stack.
    Vec3 analytical[kMaxSlots] {};

    if (n > 0)
    {
        switch (params.type)
        {
            case FormationType::Line:          computeLine          (analytical, n, params); break;
            case FormationType::Arc:           computeArc           (analytical, n, params); break;
            case FormationType::Circle:        computeCircle        (analytical, n, params); break;
            case FormationType::Grid:          computeGrid          (analytical, n, params); break;
            case FormationType::Spiral:        computeSpiral        (analytical, n, params); break;
            case FormationType::SphereSurface: computeSphereSurface (analytical, n, params); break;
            case FormationType::Custom:
            default:
                // Custom produces zero offsets; per-slot data not via APVTS.
                break;
        }
    }

    // Apply morph: lerp each slot from origin toward analytical target.
    for (int i = 0; i < n; ++i)
    {
        slotPositions[static_cast<std::size_t> (i)] = {
            analytical[i].x * morphPhase,
            analytical[i].y * morphPhase,
            analytical[i].z * morphPhase
        };
    }

    // Zero unused slots.
    for (int i = n; i < kMaxSlots; ++i)
        slotPositions[static_cast<std::size_t> (i)] = {};

    spreadDelta = computeSpreadDelta (slotPositions.data(), n);
}

//==============================================================================
// Geometry builders
//==============================================================================

void FormationSystem::computeLine (Vec3* out, int n, const FormationParams& p) noexcept
{
    const float spacing = std::max (0.0f, p.spacing);
    const float center  = 0.5f * static_cast<float> (n - 1) * spacing;

    for (int i = 0; i < n; ++i)
    {
        const float pos = static_cast<float> (i) * spacing - center;
        switch (p.axis)
        {
            case FormationAxis::X: out[i] = { pos, 0.0f, 0.0f }; break;
            case FormationAxis::Y: out[i] = { 0.0f, pos, 0.0f }; break;
            case FormationAxis::Z: out[i] = { 0.0f, 0.0f, pos }; break;
        }
    }
}

void FormationSystem::computeArc (Vec3* out, int n, const FormationParams& p) noexcept
{
    const float r          = std::max (0.0f, p.radius);
    const float sweepRad   = std::clamp (p.arcAngle, 0.0f, 360.0f) * kDegRad;
    const float startRad   = -0.5f * sweepRad;
    const float stepRad    = (n > 1) ? (sweepRad / static_cast<float> (n - 1)) : 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float a = startRad + static_cast<float> (i) * stepRad;
        const float s = std::sin (a);
        const float c = std::cos (a);

        switch (p.plane)
        {
            case FormationPlane::XZ: out[i] = { r * s, 0.0f, r * c }; break;
            case FormationPlane::XY: out[i] = { r * s, r * c, 0.0f }; break;
            case FormationPlane::YZ: out[i] = { 0.0f, r * c, r * s }; break;
        }
    }
}

void FormationSystem::computeCircle (Vec3* out, int n, const FormationParams& p) noexcept
{
    const float r        = std::max (0.0f, p.radius);
    const float phase0   = p.phaseOffset * kDegRad;
    const float denom    = static_cast<float> (std::max (1, n));

    for (int i = 0; i < n; ++i)
    {
        const float a = phase0 + (static_cast<float> (i) / denom) * kTwoPi;
        const float s = std::sin (a);
        const float c = std::cos (a);

        switch (p.plane)
        {
            case FormationPlane::XZ: out[i] = { r * s, 0.0f, r * c }; break;
            case FormationPlane::XY: out[i] = { r * s, r * c, 0.0f }; break;
            case FormationPlane::YZ: out[i] = { 0.0f, r * c, r * s }; break;
        }
    }
}

void FormationSystem::computeGrid (Vec3* out, int n, const FormationParams& p) noexcept
{
    const int   cols     = std::max (1, p.cols);
    const int   rows     = std::max (1, p.rows);
    const float sX       = std::max (0.0f, p.spacingX);
    const float sZ       = std::max (0.0f, p.spacingZ);
    const float centerX  = 0.5f * static_cast<float> (cols - 1) * sX;
    const float centerZ  = 0.5f * static_cast<float> (rows - 1) * sZ;

    for (int i = 0; i < n; ++i)
    {
        const int col = i % cols;
        const int row = i / cols;
        out[i] = {
            static_cast<float> (col) * sX - centerX,
            0.0f,
            static_cast<float> (row) * sZ - centerZ
        };
    }

    // Clamp rows to avoid out-of-bounds writes (n may exceed rows*cols).
    (void) rows;
}

void FormationSystem::computeSpiral (Vec3* out, int n, const FormationParams& p) noexcept
{
    const float r       = std::max (0.0f, p.radius);
    const float turns   = std::max (0.5f, p.turns);
    const float height  = p.heightRise;
    const float inv     = 1.0f / static_cast<float> (std::max (1, n - 1));

    for (int i = 0; i < n; ++i)
    {
        const float t     = static_cast<float> (i) * inv;   // normalised [0..1]
        const float angle = t * turns * kTwoPi;
        const float ri    = r * t;
        out[i] = { ri * std::sin (angle), height * t, ri * std::cos (angle) };
    }
}

void FormationSystem::computeSphereSurface (Vec3* out, int n, const FormationParams& p) noexcept
{
    const float r           = std::max (0.0f, p.radius);
    const float goldenAngle = kPi * (3.0f - std::sqrt (5.0f));   // ~2.39996 rad

    for (int i = 0; i < n; ++i)
    {
        const float normalized = (n > 1)
            ? static_cast<float> (i) / static_cast<float> (n - 1)
            : 0.5f;
        const float y      = 1.0f - 2.0f * normalized;
        const float radial = std::sqrt (std::max (0.0f, 1.0f - y * y));
        const float angle  = goldenAngle * static_cast<float> (i);

        out[i] = {
            r * radial * std::cos (angle),
            r * y,
            r * radial * std::sin (angle)
        };
    }
}

//==============================================================================
float FormationSystem::computeSpreadDelta (const Vec3* positions, int n) noexcept
{
    if (n < 2) return 0.0f;

    float sumDist = 0.0f;
    float maxDist = 0.0f;
    int   numPairs = 0;

    for (int i = 0; i < n; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            const float dx = positions[i].x - positions[j].x;
            const float dy = positions[i].y - positions[j].y;
            const float dz = positions[i].z - positions[j].z;
            const float d  = std::sqrt (dx * dx + dy * dy + dz * dz);
            sumDist += d;
            if (d > maxDist) maxDist = d;
            ++numPairs;
        }
    }

    if (maxDist < 1e-6f) return 0.0f;
    const float avgDist = sumDist / static_cast<float> (numPairs);
    return std::clamp (avgDist / maxDist, 0.0f, 1.0f);
}
