---
Title: ADR-0020 Four-Layer Authority Chain and Choreography Worker Arbitration
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-19
Last Modified Date: 2026-03-19
---

# ADR-0020: Four-Layer Authority Chain and Choreography Worker Arbitration

## Status

Accepted — supersedes ADR-0003 §Decision (three-layer model)

## Context

ADR-0003 established a three-layer precedence chain for spatial parameter authority:

```
APVTS base state → Timeline rest pose → Physics additive offset
```

The Choreography Lab feature (`.ideas/choreography-lab-spec.md`) introduces a fourth layer — a generative offset computed by a ChoreographyWorker from formation patterns, procedural paths, beat-sync rules, and audio-reactive mappings. This generative offset sits between the Timeline rest pose and the Physics additive offset. ADR-0003's three-layer model cannot express this without an authority conflict.

Two questions are left open by the specs and must be answered before Choreography Lab output may write to production `EmitterSlot` instances:

1. **Layer ordering:** where exactly does the Choreography generative offset sit and how is it composed?
2. **Thread arbitration:** do ChoreographyWorker and PhysicsWorker share a tick cycle or run independently, and who owns the final `EmitterSlot` write?

## Decision

### 1. Four-Layer Authority Chain

Adopt the following precedence chain for all `EmitterSlot` spatial parameters:

```
Layer 1 — APVTS base state (DAW automation, host recall)
  + Layer 2 — Timeline rest pose   (when anim_enable=true and anim_mode=Internal)
    + Layer 3 — Choreography Lab generative offset   (when choro_enable=true)
      + Layer 4 — Physics additive offset   (when phys_enable=true)
        = Final EmitterSlot position → DSP renderer
```

- **Layer 1** (APVTS) is the absolute authority floor. No other layer overrides DAW automation. When `anim_mode=DAW`, the internal Timeline does not evaluate; Layer 1 is sole spatial authority.
- **Layer 2** (Timeline) contributes a rest pose evaluated per `processBlock` on the audio thread. When `anim_enable=false` or `anim_mode=DAW`, Layer 2 contributes zero.
- **Layer 3** (Choreography Lab) contributes a generative offset computed by the ChoreographyWorker. This offset is **additive** on top of the Layer 1 + Layer 2 composed rest pose — it never overrides APVTS base state or Timeline rest pose values. When `choro_enable=false`, Layer 3 contributes zero.
- **Layer 4** (Physics) contributes an additive offset on top of Layers 1–3. This is unchanged from ADR-0003. When `phys_enable=false`, Layer 4 contributes zero.

### 2. EmitterSlot Field Authority by Layer

| EmitterSlot field | L1 | L2 | L3 | L4 |
|---|---|---|---|---|
| `position` (x/y/z) | base | rest pose | generative offset (additive) | physics offset (additive) |
| `spread` | base | value track | formation spread (additive) | physics spread (additive) |
| `gain` | base | value track | teleport dip envelope | collision/event transients |
| `velocity` | base | — | path velocity (Doppler source) | Boids/spring velocity (Doppler source) |
| `directivityAim` | base | — | — | angular physics output |

For `spread` and `gain`, all active additive contributions from Layers 3 and 4 are summed and clamped to [0..1] before the final write to `EmitterSlot`. No contribution from any layer may push `spread` or `gain` outside [0..1]. The APVTS base value is not itself clamped by this step — the sum of (base + all additive offsets) is clamped.

### 3. Worker Thread Arbitration: Combined Tick Model

ChoreographyWorker computation is **colocated within the PhysicsWorker tick** at the start of each cycle. The execution order within a single worker tick is:

```
[Worker tick N]
  1. Read DAW transport position atomically (beat-sync, Timeline sync)
  2. Read audio ring buffer → compute feature extracts (audio-reactive)
  3. Compute Choreography generative positions and DSP offsets (Layer 3)
  4. Read current APVTS base state snapshot
  5. Compose rest pose: APVTS base + Timeline rest pose + Choreography offset
  6. Compute Physics integration step (Layer 4)
  7. Sum and clamp all spread/gain contributions
  8. Atomic pointer swap → write final EmitterSlot
```

**Rationale for colocated model over separate threads:**
- Eliminates the race condition that arises when two threads both write to `EmitterSlot` via atomic swap: if ChoreographyWorker and PhysicsWorker are independent threads, PhysicsWorker cannot safely read Choreography output from `EmitterSlot` to compose its rest pose without a second mutex or barrier.
- Maintains the invariant that exactly one `EmitterSlot` write occurs per tick (step 8), preserving the existing ADR-0002 atomic swap contract unchanged.
- Choreography computation (analytical paths, formation interpolation, beat-sync) is CPU-light relative to physics integration; colocating it in the same tick does not require a separate scheduling primitive.
- Audio-reactive feature extraction (step 2) reads from a lock-free ring buffer written by the audio thread — this read is non-blocking and safe from the worker thread.

**Worker thread identity:** the existing `PhysicsWorker` thread is extended to execute Choreography steps 2–3 before Physics steps 4–7. No new OS thread is introduced. The thread continues to run at `rend_phys_rate` Hz.

**ChoreographyWorker as a logical module:** `ChoreographyWorker` remains a distinct module (class/component) that the extended worker thread calls at step 3. It owns its internal state (path phase, formation geometry, leader/follower history ring buffer, audio feature state). The module boundary is preserved for testability; the threading boundary is not.

### 4. Authority Conflict Resolution

**Same field, same tick, multiple layers:** the composition in steps 4–7 above is deterministic and ordered. Layer 3 is read before Layer 4 integration; Layer 4 never reads Layer 4's own prior-tick output as the rest pose.

**Timeline overlap validation:** if two Timeline tracks address the same parameter for the same emitter at the same playhead position, the last-added track wins (existing Timeline rule, unchanged).

**choro_enable while anim_enable=false:** Layer 3 uses the APVTS base state alone as its rest pose input. Choreography offsets apply additively on top of APVTS.

**phys_enable=false while choro_enable=true:** Layer 4 is skipped. Choreography output passes through to EmitterSlot without physics integration. The EmitterSlot write still occurs at step 8.

**Both choro_enable=false and phys_enable=false:** steps 2–7 are skipped. EmitterSlot receives the Layer 1 + Layer 2 composed rest pose directly (Timeline-only path, identical to ADR-0003 behavior).

### 5. Bake-to-Timeline Output

When Choreography Lab bakes a session to Timeline keyframes (`.ideas/choreography-lab-spec.md §6`), the baked output becomes Layer 2 (Timeline) assets. After bake, the Choreography Lab contribution to that session is embedded in the Timeline rest pose. This is the graduation path for all Choreography Lab features to production.

## Rationale

- **Colocated tick** eliminates the need for a second atomic write path into `EmitterSlot`, preserving the ADR-0002 swap contract with zero structural change.
- **Additive-only Layers 3 and 4** ensure DAW recall fidelity: session recall restores APVTS base state; physics and choreography states re-emerge from simulation and are never persisted as canonical position data.
- **Single final write per tick** keeps the audio thread's read path unchanged: it reads `EmitterSlot` once per block, after all layers have been composed.
- **Explicit conflict rules** allow QA to write deterministic acceptance tests for each layer-combination scenario without ambiguity.

## Consequences

### Positive

- Extends ADR-0003 cleanly; the audio thread and EmitterSlot read path require no changes.
- ChoreographyWorker module can be unit-tested in isolation from the physics engine by calling its `compute()` method directly.
- Bake graduation path preserves full determinism: baked Choreography output becomes a standard Timeline asset subject to Layer 2 rules.
- Authority conflict rules are enumerable; QA can test each combination.

### Costs

- The PhysicsWorker tick is longer by the cost of Choreography computation. Worst-case CPU must be re-measured with all Tier A physics features and all Choreography Lab production-candidate features enabled simultaneously at 64 emitters before ship.
- Audio-reactive feature extraction (step 2) runs every tick even when no reactive mapping is active; this is bounded and cheap but must be confirmed in profiling.
- Future alternate blend modes (e.g. Choreography overrides rather than offsets) require an additional ADR update before implementation.

## Guardrails

1. No Choreography Lab output may be written to production `EmitterSlot` instances before this ADR is merged.
2. The `EmitterSlot` atomic swap in step 8 remains the single write point per tick; no other code path may write to `EmitterSlot` outside the worker tick sequence.
3. All Layer 3 and Layer 4 contributions to `spread` and `gain` must be clamped to finite [0..1] before step 8; no NaN or out-of-range values may reach the audio thread.
4. Worst-case CPU re-measurement (all Tier A + production-candidate Choreography features, 64 emitters, 240Hz rate) is a hard acceptance gate before ship.
5. Any change to the tick execution order (steps 1–8) requires an ADR update.

## Supersedes

ADR-0003 §Decision (three-layer model). ADR-0003's §Rationale, §Consequences, and §Guardrails remain in force and are extended by this ADR.

## Related

- `Documentation/adr/ADR-0002-routing-model-v1.md` — atomic pointer swap contract
- `Documentation/adr/ADR-0003-automation-authority-precedence.md` — superseded §Decision
- `.ideas/choreography-lab-spec.md` — Choreography Lab feature spec and pending ADR reference
- `.ideas/physics-simulation-spec.md` — Physics spec and pending ADR reference
- `.ideas/timeline-spec.md` — Timeline spec and authority chain diagram
- `Documentation/invariants.md`
- `.ideas/architecture.md`
