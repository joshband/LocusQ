Title: LocusQ Scene State Contract
Document Type: Interface Contract
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-19

# Scene State Contract

## Purpose

Define the single source-of-truth contract between DSP runtime, physics, and UI so `skill_plan`, `skill_design`, and `skill_impl` execute against the same state model.

## Normative Decisions

- Routing model: `Documentation/adr/ADR-0002-routing-model-v1.md`
- Automation precedence: `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- AI gating for v1: `Documentation/adr/ADR-0004-v1-ai-deferral.md`

## Scope

Applies to:

1. Emitter/Renderer state exchange through `SceneGraph`
2. Physics-to-audio position/velocity handoff
3. UI snapshot publication and command ingestion

## State Domains

### Audio Domain (Real-Time)

Owned by audio thread in `processBlock`:

- per-emitter metadata state (active, position, size, gain, spread, directivity, velocity, labels, flags)
- ephemeral emitter audio block pointer fast path (v1)
- renderer accumulation/output state

Constraints:

- no locks
- no heap allocation
- deterministic per-block behavior for identical input/state

### Physics Domain (Worker Thread)

Owned by physics worker:

- body state (position, velocity)
- force integration state

Handoff:

- lock-free double-buffer/atomic publication to audio domain
- additive offset semantics against rest pose (per ADR-0003)

### Message/UI Domain

Owned by message thread/WebView bridge:

- JSON snapshots for UI rendering
- incoming control commands (parameter edits, timeline edits, preset actions)

Constraints:

- never call WebView from audio thread
- snapshot publication rate bounded (for example 30-60 Hz)
- bridge commands apply as parameter/state updates, not direct DSP mutation

## Source Of Truth Model

1. APVTS/host parameter state is base authority.
2. Internal timeline can define rest pose for animated tracks when enabled.
3. Physics applies additive offset.
4. SceneGraph publishes the resulting emitter state for renderer consumption.

Any conflict resolution beyond this contract requires ADR update.

## Routing Contract (V1)

1. Emitter publishes metadata each block.
2. Emitter also publishes an ephemeral audio pointer fast path for same-block renderer consumption.
3. Renderer consumes scene state within the same callback cycle.
4. If fast-path assumptions fail in a host/runtime context, runtime must degrade safely (no crash/non-finite output), and host-edge acceptance evidence must capture behavior.

## Serialization Contract

UI snapshot payloads are derived from stable copies of runtime state and include:

- emitter transforms and labels
- velocity vectors and animation state indicators
- room/speaker/listener snapshots
- coarse performance telemetry needed by UI

Serialization is message-thread work; audio thread remains non-blocking.

## Determinism Contract

For identical:

- input audio
- parameter timeline
- transport/time source
- physics configuration and seed state

output behavior must be reproducible within expected floating-point tolerance.

## Design Integration Contract (`skill_design`)

1. Persistent viewport is required across Calibrate/Emitter/Renderer.
2. Mode switching changes overlays and controls, not scene continuity.
3. Draft/Final visual cues must communicate quality tier without implying different semantic scene state.

## Implementation Integration Contract (`skill_impl`)

1. Maintain one-to-one parameter coverage and traceability updates.
2. Preserve thread-domain boundaries and RT-safety constraints.
3. Treat acceptance evidence logging as mandatory before marking phase completion.

## Validation Hooks

Required evidence classes:

1. smoke/runtime stability
2. full-system CPU/deadline behavior
3. host edge-case lifecycle behavior
4. state/traceability doc synchronization

## Related

- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
- `Documentation/lessons-learned.md`
