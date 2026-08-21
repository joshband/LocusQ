#pragma once

#include "SceneGraph.h"

//==============================================================================
/**
 * Per-emitter generative contribution for one choreography worker tick.
 *
 * All fields are additive on the composed rest pose (ADR-0020 Layer 3).
 */
struct ChoreographyOffset
{
    Vec3  position    {};          ///< additive position offset (metres)
    float spreadDelta = 0.0f;      ///< additive spread contribution
    float gainDelta   = 0.0f;      ///< additive gain contribution (for example, teleport dip)
    Vec3  velocity    {};          ///< path velocity forwarded as Doppler source
};
