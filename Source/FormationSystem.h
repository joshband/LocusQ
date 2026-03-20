#pragma once

#include "SceneGraph.h"

#include <array>
#include <cmath>

//==============================================================================
// Formation geometry types
//==============================================================================

enum class FormationType : int
{
    Line = 0,
    Arc,
    Circle,
    Grid,
    Spiral,
    SphereSurface,
    Custom
};

enum class FormationPlane : int { XZ = 0, XY, YZ };
enum class FormationAxis  : int { X  = 0, Y,  Z  };

//==============================================================================
/** FormationParams - snapshot of all formation APVTS params for one compute().
 *
 *  Written by PluginProcessor (audio thread) via ChoreographyWorker setters;
 *  read on the PhysicsWorker thread inside FormationSystem::compute(). */
struct FormationParams
{
    FormationType type        = FormationType::Circle;
    FormationAxis axis        = FormationAxis::X;
    FormationPlane plane      = FormationPlane::XZ;

    float radius              = 2.0f;    // Arc, Circle, Spiral, SphereSurface (m)
    float spacing             = 1.0f;    // Line inter-slot spacing; Spiral inter-turn spacing (m)
    float arcAngle            = 180.0f;  // Arc total sweep (degrees)
    float phaseOffset         = 0.0f;    // Circle slot-0 phase (degrees)

    int   rows                = 2;       // Grid
    int   cols                = 2;       // Grid
    float spacingX            = 1.0f;    // Grid (m)
    float spacingZ            = 1.0f;    // Grid (m)

    float turns               = 2.0f;    // Spiral number of full turns
    float heightRise          = 1.0f;    // Spiral total Y rise (m)
};

//==============================================================================
/** FormationSystem - computes per-slot geometry positions for all 7 formation
 *  types and a morph-scaled spread delta.
 *
 *  Threading: compute() is called on the PhysicsWorker thread.  All output
 *  is stored in fixed pre-allocated arrays — no heap allocation during operation.
 *
 *  Morph contract:
 *    morphPhase ∈ [0..1] scales each slot position linearly from origin (0,0,0)
 *    toward the analytical formation target.  At morphPhase=1.0 slots are at
 *    their exact analytical positions (acceptance gate: < 1mm error).
 */
class FormationSystem
{
public:
    static constexpr int kMaxSlots = 64;

    /** Compute slot positions and spread delta for up to numSlots emitters.
     *  morphPhase ∈ [0..1]: 0 = all at origin, 1 = full formation geometry. */
    void compute (const FormationParams& params,
                  int numSlots,
                  float morphPhase) noexcept;

    /** Position for slot i after the last compute(). */
    Vec3 getSlotPosition (int i) const noexcept
    {
        if (i < 0 || i >= kMaxSlots) return {};
        return slotPositions[static_cast<std::size_t> (i)];
    }

    /** Additive spread contribution after the last compute().
     *  = avgPairwiseDist / maxPairwiseDist, clamped [0..1].
     *  Zero for n ≤ 1 emitters. */
    float getSpreadDelta() const noexcept { return spreadDelta; }

private:
    std::array<Vec3, kMaxSlots> slotPositions {};
    float spreadDelta = 0.0f;

    // Geometry builders — write into the first numSlots entries of out[].
    static void computeLine          (Vec3* out, int n, const FormationParams& p) noexcept;
    static void computeArc           (Vec3* out, int n, const FormationParams& p) noexcept;
    static void computeCircle        (Vec3* out, int n, const FormationParams& p) noexcept;
    static void computeGrid          (Vec3* out, int n, const FormationParams& p) noexcept;
    static void computeSpiral        (Vec3* out, int n, const FormationParams& p) noexcept;
    static void computeSphereSurface (Vec3* out, int n, const FormationParams& p) noexcept;

    // Spread delta from finalised slot positions.
    static float computeSpreadDelta  (const Vec3* positions, int n) noexcept;
};
