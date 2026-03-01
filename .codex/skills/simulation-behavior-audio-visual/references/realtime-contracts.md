Title: Simulation Realtime Contracts

## Dataflow Contract
- Simulation thread publishes bounded snapshots.
- Audio thread consumes latest safe snapshot without waiting.
- UI/render thread consumes normalized snapshot stream with bounded cadence.

## Safety Contract
- `NaN`/`Inf` and out-of-range values must be clamped before use.
- Snapshot age and cadence must be observable in diagnostics.
- Missing or invalid simulation data must trigger deterministic fallback.

## Evidence Contract
- Deterministic replay hash for representative scenarios.
- CPU and payload budget metrics.
- Failure taxonomy entries for simulation stalls/invalid payloads.
