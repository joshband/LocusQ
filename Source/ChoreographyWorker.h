#pragma once

#include "AudioRingBuffer.h"
#include "BakeRecorder.h"
#include "BeatSyncSystem.h"
#include "ChoreographyOffset.h"
#include "FormationSystem.h"
#include "PathSystem.h"
#include "SceneGraph.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

//==============================================================================
/**
 * ChoreographyWorker - ADR-0020 Layer 3 generative motion module.
 *
 * Runs colocated within the PhysicsWorker tick — no separate OS thread.
 * Called once per tick via compute() before physics integration (step 3 of
 * the ADR-0020 tick sequence).
 *
 * Threading contract:
 *   - compute()                  — PhysicsWorker thread only.
 *   - getOffset()                — PhysicsWorker thread only (valid after compute() returns).
 *   - setEnabled()               — any thread (atomic store).
 *   - setFormation*() setters    — audio thread only (called from publishEmitterState()).
 *   - setPath*() setters         — audio thread only (called from publishEmitterState()).
 *   - setBeat*() / setTransport* — audio thread only (called from publishEmitterState()).
 *   - pushAudioBlock()           — audio thread only (single producer into AudioRingBuffer).
 *
 * Phase history:
 *   CL-P1: infrastructure (AudioRingBuffer, choro_enable gate, zero-offset bypass).
 *   CL-P2: FormationSystem integrated; morph animation; spread delta.
 *   CL-P3: PathSystem integrated; 6 analytical path types; velocity Doppler hook.
 *   CL-P4: BeatSyncSystem integrated; beat-boundary detection; snap/glide/teleport;
 *           teleport gain-dip envelope; 16-step pattern sequencer.
 *   CL-P7: BakeRecorder integrated; auto-records choreography positions over a
 *           configured PPQ range; exports per-emitter position KeyframeTracks.
 */
class ChoreographyWorker
{
public:
    static constexpr int kMaxEmitters = 64;

    //==========================================================================
    // Control interface (any thread)

    /** Enable or disable choreography computation.
     *  When false, compute() produces zero offsets with no side effects. */
    void setEnabled (bool enabled) noexcept
    {
        enabledFlag.store (enabled, std::memory_order_release);
    }

    bool isEnabled() const noexcept
    {
        return enabledFlag.load (std::memory_order_acquire);
    }

    //==========================================================================
    // Formation param setters (audio thread — called from publishEmitterState())
    // Values are stored and consumed on the worker thread in compute().

    void setFormationType      (int   v) noexcept { formationParams.type        = static_cast<FormationType>  (std::clamp (v, 0, 6)); }
    void setFormationAxis      (int   v) noexcept { formationParams.axis        = static_cast<FormationAxis>  (std::clamp (v, 0, 2)); }
    void setFormationPlane     (int   v) noexcept { formationParams.plane       = static_cast<FormationPlane> (std::clamp (v, 0, 2)); }
    void setFormationRadius    (float v) noexcept { formationParams.radius      = std::max (0.1f, v); }
    void setFormationSpacing   (float v) noexcept { formationParams.spacing     = std::max (0.1f, v); }
    void setFormationArcAngle  (float v) noexcept { formationParams.arcAngle    = std::clamp (v, 0.0f, 360.0f); }
    void setFormationPhaseOffset(float v) noexcept { formationParams.phaseOffset = std::clamp (v, 0.0f, 360.0f); }
    void setFormationRows      (float v) noexcept { formationParams.rows        = std::clamp (static_cast<int> (std::lround (v)), 1, 16); }
    void setFormationCols      (float v) noexcept { formationParams.cols        = std::clamp (static_cast<int> (std::lround (v)), 1, 16); }
    void setFormationSpacingX  (float v) noexcept { formationParams.spacingX    = std::max (0.1f, v); }
    void setFormationSpacingZ  (float v) noexcept { formationParams.spacingZ    = std::max (0.1f, v); }
    void setFormationTurns     (float v) noexcept { formationParams.turns       = std::clamp (v, 0.5f, 8.0f); }
    void setFormationHeightRise(float v) noexcept { formationParams.heightRise  = v; }
    void setFormationMorphRate (float v) noexcept { morphRate                   = std::clamp (v, 0.01f, 10.0f); }
    void setFormationMorphLoop (bool  v) noexcept { morphLoop                   = v; }
    void setFormationMorphPingpong(bool v) noexcept { morphPingpong             = v; }

    //==========================================================================
    // Path param setters (audio thread — called from publishEmitterState())
    // CL-P3: 6 analytical path types; velocity Doppler hook.

    void setPathType        (int   v) noexcept { pathParams.type        = static_cast<PathType>  (std::clamp (v, 0, 5)); }
    void setPathPeriod      (float v) noexcept { pathParams.period      = std::clamp (v, 0.1f, 60.0f); }
    void setPathSpeed       (float v) noexcept { pathParams.speed       = std::clamp (v, 0.1f, 10.0f); }
    void setPathLissFreqA   (float v) noexcept { pathParams.lissFreqA   = std::clamp (v, 1.0f, 8.0f); }
    void setPathLissFreqB   (float v) noexcept { pathParams.lissFreqB   = std::clamp (v, 1.0f, 8.0f); }
    void setPathLissFreqC   (float v) noexcept { pathParams.lissFreqC   = std::clamp (v, 1.0f, 8.0f); }
    void setPathLissAmpX    (float v) noexcept { pathParams.lissAmpX    = std::max (0.0f, v); }
    void setPathLissAmpY    (float v) noexcept { pathParams.lissAmpY    = std::max (0.0f, v); }
    void setPathLissAmpZ    (float v) noexcept { pathParams.lissAmpZ    = std::max (0.0f, v); }
    void setPathLissPhase   (float v) noexcept { pathParams.lissPhase   = std::clamp (v, 0.0f, 360.0f); }
    void setPathOrbitRx     (float v) noexcept { pathParams.orbitRx     = std::max (0.1f, v); }
    void setPathOrbitRz     (float v) noexcept { pathParams.orbitRz     = std::max (0.1f, v); }
    void setPathOrbitHeight (float v) noexcept { pathParams.orbitHeight = v; }
    void setPathPendLength  (float v) noexcept { pathParams.pendLength  = std::max (0.1f, v); }
    void setPathPendAmp     (float v) noexcept { pathParams.pendAmp     = std::clamp (v, 0.0f, 180.0f); }
    void setPathPendPlane   (int   v) noexcept { pathParams.pendPlane   = static_cast<PathPlane> (std::clamp (v, 0, 2)); }
    void setPathFig8Scale   (float v) noexcept { pathParams.fig8Scale   = std::max (0.1f, v); }
    void setPathFig8Plane   (int   v) noexcept { pathParams.fig8Plane   = static_cast<PathPlane> (std::clamp (v, 0, 2)); }
    void setPathHelixRadius (float v) noexcept { pathParams.helixRadius = std::max (0.1f, v); }
    void setPathHelixPitch  (float v) noexcept { pathParams.helixPitch  = std::max (0.01f, v); }
    void setPathHelixDir    (int   v) noexcept { pathParams.helixDir    = static_cast<HelixDir> (std::clamp (v, 0, 1)); }
    void setPathWalkStep    (float v) noexcept { pathParams.walkStep    = std::clamp (v, 0.001f, 0.5f); }
    void setPathWalkBounds  (float v) noexcept { pathParams.walkBounds  = std::max (0.1f, v); }
    void setPathWalkSeed    (int   v) noexcept { pathParams.walkSeed    = std::clamp (v, 0, 65535); }

    //==========================================================================
    // Beat-sync param setters (audio thread — called from publishEmitterState())
    // CL-P4: beat-boundary detection; snap/glide/teleport; gain-dip envelope; step sequencer.

    void setBeatEnabled      (bool  v) noexcept { beatEnabled              = v; }
    void setBeatDivision     (int   v) noexcept { beatParams.division      = static_cast<BeatDivision> (std::clamp (v, 0, 4)); }
    void setBeatMode         (int   v) noexcept { beatParams.mode          = static_cast<BeatMode>     (std::clamp (v, 0, 2)); }
    void setTeleportDipDb    (float v) noexcept { beatParams.dipDb         = std::clamp (v, -60.0f, 0.0f); }
    void setTeleportDecayMs  (float v) noexcept { beatParams.decayMs       = std::max (1.0f, v); }

    //==========================================================================
    // Bake param preparation (main thread — called from handleAsyncUpdate())
    // CL-P7: delegates to BakeRecorder::prepare(); safe when isRecording() is false.

    void prepareBake (const BakeRecorder::BakeParams& p,
                      double bpm,
                      float physRateHz,
                      int numEmitters) noexcept
    {
        bakeRecorder.prepare (p, bpm, physRateHz, numEmitters);
    }

    BakeRecorder& getBakeRecorder() noexcept { return bakeRecorder; }

    //==========================================================================
    // Transport info setters (audio thread — called from processBlock())
    // Stored atomically; read on PhysicsWorker thread in compute().

    void setTransportInfo (double ppqPosition, double bpm, bool isPlaying) noexcept
    {
        transportPpq.store     (ppqPosition, std::memory_order_relaxed);
        transportBpm.store     (bpm,         std::memory_order_relaxed);
        transportPlaying.store (isPlaying,   std::memory_order_relaxed);
    }

    //==========================================================================
    // Worker-thread interface

    /** Compute per-emitter choreography offsets for this tick.
     *  Called on the PhysicsWorker thread at the start of each tick, before
     *  physics integration (ADR-0020 step 3).
     *
     *  numEmitters: count of currently active emitter slots (from PhysicsWorker).
     *  CL-P1: drains audio ring, zeroes offsets.
     *  CL-P2: runs FormationSystem, writes position offsets and spread delta. */
    void compute (float dt, int numEmitters) noexcept;

    /** Return the choreography offset for emitter index.
     *  Only valid on the PhysicsWorker thread after the current tick's compute(). */
    const ChoreographyOffset& getOffset (int index) const noexcept
    {
        if (index < 0 || index >= kMaxEmitters)
            return kZeroOffset;
        return offsets[static_cast<std::size_t> (index)];
    }

    //==========================================================================
    // Audio-thread interface

    /** Write audio block data into the ring buffer.
     *  Called from the audio thread per processBlock. Non-blocking (drops frames
     *  on overflow). Used by CL-P5 AudioFeatureExtractor. */
    void pushAudioBlock (const float* const* channelData,
                         int numChannels,
                         int numFrames) noexcept
    {
        audioRing.write (channelData, numChannels, numFrames);
    }

private:
    std::atomic<bool>                            enabledFlag { false };
    std::array<ChoreographyOffset, kMaxEmitters> offsets     {};

    AudioRingBuffer<4096, 2> audioRing {};

    // CL-P2: Formation system + params + morph state.
    // Written by audio thread (setFormation*), read by worker thread (compute()).
    FormationSystem formationSystem {};
    FormationParams formationParams {};

    float morphPhase    = 0.0f;    // [0..1] current morph position
    float morphRate     = 1.0f;    // Hz (cycles per second)
    float morphDir      = 1.0f;    // +1.0 or -1.0 (ping-pong direction)
    bool  morphLoop     = false;
    bool  morphPingpong = false;

    // CL-P3: Path system + params.
    // Written by audio thread (setPath*), read by worker thread (compute()).
    PathSystem pathSystem {};
    PathParams pathParams {};

    // CL-P4: Beat-sync system + params + transport state.
    // beatParams written by audio thread (setBeat*); transport atomics written
    // by audio thread (setTransportInfo); all read by worker thread in compute().
    bool          beatEnabled = false;
    BeatSyncParams beatParams {};
    BeatSyncSystem beatSyncSystem {};

    // CL-P7: Bake-to-timeline position recorder.
    // prepare() called from main thread; tick() called from worker thread in compute().
    BakeRecorder bakeRecorder {};

    std::atomic<double> transportPpq     { 0.0 };
    std::atomic<double> transportBpm     { 120.0 };
    std::atomic<bool>   transportPlaying { false };

    static const ChoreographyOffset kZeroOffset;
};
