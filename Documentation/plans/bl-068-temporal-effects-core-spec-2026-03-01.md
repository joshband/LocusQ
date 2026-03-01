Title: BL-068 Temporal Effects Core Spec (Delay/Echo/Looper/Frippertronics)
Document Type: Annex Spec
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-068 Temporal Effects Core Spec (Delay/Echo/Looper/Frippertronics)

## Purpose

Define implementation boundaries and acceptance contracts for temporal DSP expansion in LocusQ (delay, echo, looper, and frippertronics-style long-feedback behavior) with deterministic realtime safety.

Depends on: BL-050, BL-055.

## DSP Contract

1. Delay/loop buffers must be preallocated and bounded.
2. Feedback networks must enforce explicit gain ceilings and non-finite guards.
3. Transport/tempo behavior must be deterministic for identical timeline input.
4. Automation transitions must be click-safe and zipper-safe.
5. Temporal effects must remain compatible with FIR and spatial-monitoring lanes.

## Slice Plan

### Slice A: Delay/Echo + Feedback Safety
- Implement bounded delay and feedback architecture.
- Add finite-output and runaway-guard checks.
- Validate high-rate operation assumptions.

### Slice B: Looper + Frippertronics Layering
- Implement deterministic overdub/clear/feedback behavior.
- Define transport-start and recall semantics.
- Validate long-feedback musical stability and guardrails.

### Slice C: Evidence + Visualization Contract
- Export temporal-state telemetry contract for UI/visualization lanes.
- Capture CPU/latency budget snapshots per profile.
- Prepare promotion packet with replay and failure taxonomy.

## Acceptance Contract

- Delay/echo behavior remains stable across 44.1kHz to 192kHz.
- Feedback safety lane prevents runaway and non-finite output.
- Session recall reproduces loop state deterministically.
- Automation and mode switches are free of audible clicks/zipper artifacts.

## Validation and Evidence

Primary lane:
- `scripts/qa-bl068-temporal-effects-mac.sh`

Required evidence bundle (`TestEvidence/bl068_*/`):
- `status.tsv`
- `temporal_matrix.tsv`
- `runaway_guard.tsv`
- `transport_recall.tsv`
- `cpu_latency_budget.tsv`

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Feedback runaway in long-layer modes | High | Hard gain ceiling + finite-output guards + dedicated stress lane |
| Drift across transport/recall edges | High | Deterministic replay checks on transport start/stop and session restore |
| CPU spikes at high sample rates | Med | Budget snapshots at profile/sample-rate matrix and quality-tier gating |

## Backlog References

- Runbook: `Documentation/backlog/bl-068-temporal-effects-delay-echo-looper-frippertronics.md`
- Index: `Documentation/backlog/index.md`
