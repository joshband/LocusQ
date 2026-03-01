Title: BL-067 AUv3 App-Extension Lifecycle and Host Validation Spec
Document Type: Annex Spec
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-067 AUv3 App-Extension Lifecycle and Host Validation Spec

## Purpose

Define architecture and acceptance contracts for adding AUv3 support to LocusQ while preserving deterministic behavior and parity across AU/VST3/CLAP lanes.

Depends on: BL-048 (Done).

## Architecture Boundaries

1. Treat AUv3 as app-extension runtime with stricter lifecycle and sandbox constraints.
2. Keep DSP core format-agnostic and realtime-safe regardless of host format.
3. Keep app-only services out of extension DSP path.
4. Degrade unavailable capabilities deterministically, without host-name branching.

## Slice Plan

### Slice A: Target + Packaging Wireup
- Add/verify AUv3 target generation and packaging contract.
- Preserve AU/VST3/CLAP build outputs in the same pipeline.
- Define signing/entitlement contract for extension runtime.

### Slice B: Lifecycle + Runtime Safety
- Validate cold start, reload, suspend/resume, and state restore semantics.
- Confirm no allocation/locks/blocking I/O in realtime callbacks.
- Verify extension-safe state and asset access boundaries.

### Slice C: Cross-Format Parity Evidence
- Execute AUv3 host matrix.
- Execute AU/VST3/CLAP regression matrix.
- Capture promotion evidence packet with parity outcomes.

## Acceptance Contract

- AUv3 target builds and packages reproducibly.
- Lifecycle transition matrix passes for declared host set.
- Realtime safety invariants remain green.
- Cross-format parity matrix has no new regressions.

## Validation and Evidence

Primary lane:
- `scripts/qa-bl067-auv3-lifecycle-mac.sh`

Required evidence bundle (`TestEvidence/bl067_*/`):
- `status.tsv`
- `host_matrix.tsv`
- `lifecycle_transitions.tsv`
- `parity_regression.tsv`
- `packaging_manifest.md`

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Extension lifecycle drift between hosts | High | Host matrix + deterministic fallback assertions |
| Format-specific behavior leaks into DSP | High | Enforce format-agnostic DSP boundary in code review and lane checks |
| Packaging/signing fragility | Med | Keep packaging manifest and explicit signing checklist in evidence |

## Backlog References

- Runbook: `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md`
- Index: `Documentation/backlog/index.md`
