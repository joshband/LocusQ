Title: ADR-0016 Head-Tracking Wire Protocol Compatibility and Sunset Policy
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# ADR-0016: Head-Tracking Wire Protocol Compatibility and Sunset Policy

## Status
Accepted

## Context

LocusQ plugin runtime currently decodes two companion pose packet formats in
`Source/HeadTrackingBridge.h`:

1. v1 packets (`36` bytes) with quaternion + timestamp + sequence.
2. v2 packets (`52` bytes) adding angular-velocity and sensor-location flags.

Architecture and backlog records show a transitional state where companion work is
v2-oriented while compatibility with earlier packet producers is still required.
Without an explicit policy, v1/v2 support can drift and break migration planning.

## Decision

Adopt a versioned compatibility-and-sunset policy for plugin head-tracking ingest:

1. Plugin runtime continues to accept both v1 and v2 packet formats.
2. Companion/runtime producers must prefer v2 by default for new integrations.
3. Unknown packet versions or malformed payloads are rejected deterministically
   without crashes or non-finite orientation output.
4. Sequence monotonicity and stale-timeout safeguards remain mandatory for accepted
   packets (no hidden replay of stale pose streams).
5. v1 removal is blocked until a promotion packet explicitly shows:
   - zero required v1 senders in supported workflows, and
   - green deterministic v2-only replay evidence across declared host matrix lanes.

## Rationale

1. Preserves migration safety while v2 fields are still propagating through companion lanes.
2. Prevents accidental hard breaks for existing local tooling or older companion builds.
3. Makes eventual v1 retirement a deliberate, evidence-backed governance action.

## Consequences

### Positive

1. Clear contract for compatibility during migration.
2. Better alignment between architecture docs, bridge code, and validation lanes.
3. Lower risk of silent protocol regressions.

### Costs

1. Transitional dual-format support increases validation surface.
2. v1 deprecation now requires explicit promotion evidence, not ad-hoc cleanup.

## Guardrails

1. Packet parsing and pose publication must stay off the plugin audio thread.
2. Runtime pose consumption in audio/render paths remains lock-free and allocation-free.
3. Any packet-size/version contract change requires synchronized updates to:
   - `ARCHITECTURE.md`
   - `Documentation/scene-state-contract.md`
   - companion protocol runbook/spec surfaces.

## Related

- `Source/HeadTrackingBridge.h`
- `Source/HeadPoseInterpolator.h`
- `ARCHITECTURE.md`
- `Documentation/backlog/done/bl-072-companion-runtime-protocol-parity-and-bl058-qa-harness.md`
- `Documentation/backlog/bl-058-companion-profile-acquisition.md`
