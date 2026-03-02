Title: BL-050 Partitioned FIR Migration Contract
Document Type: Annex Spec
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-050 Partitioned FIR Migration Contract

## Purpose

Define Slice B contract bounds for migrating BL-050 from current direct FIR behavior toward partitioned FIR scalability without regressing realtime safety, latency semantics, or perceptual parity.

Depends on: BL-050, BL-055.

## Baseline Snapshot (Owner T1 Replay)

Reference packet: `TestEvidence/bl050_owner_t1_20260301T234531Z/`.

- T1 replay result: `3/3 PASS` (`run_01..run_03`).
- Current direct path high-rate matrix: PASS across 44.1k/48k/88.2k/96k/192k.
- Current direct FIR profile (`run_01`) stayed allocation-free and deadline-safe at all tested rates.

## Migration Contract

### Engine Selection Contract

1. `<=256` taps: direct convolver path.
2. `>256` taps: partitioned FFT convolver path.
3. Engine/profile switches must be click-safe and deterministic.

### Latency Contract

1. Direct FIR latency: `0` samples.
2. Partitioned FIR latency: `nextPow2(blockSize)` samples.
3. Reported plugin latency (`setLatencySamples`) must update on every engine transition.

### Realtime Safety Contract

Hard fail gates:

1. `perf_meets_deadline == true` for all matrix points.
2. `non_finite == 0`.
3. `perf_allocation_free == true`.
4. `perf_total_allocations == 0`.

### Performance Bound Contract (Block Size 512, Stereo)

Direct path hard bounds:

1. `perf_avg_block_time_ms <= 0.50`.
2. `perf_p95_block_time_ms <= 0.75`.

Partitioned prototype hard bounds:

1. `perf_avg_block_time_ms <= 1.25`.
2. `perf_p95_block_time_ms <= 2.00`.

### Quality/Parity Contract

Against direct-path baseline at the same sample rate:

1. `abs(rms_energy_delta_db) <= 1.0 dB`.
2. `abs(rt60_delta_seconds) <= 0.25 s`.
3. clipping guard remains below hard-fail threshold.

## Validation Matrix

Minimum matrix for Slice B:

1. Sample rates: `44100`, `48000`, `88200`, `96000`, `192000`.
2. Block sizes: `256`, `512`.
3. FIR modes: direct threshold case (`<=256`) and partitioned case (`>256`).

## Evidence Contract (Slice B Additions)

Required additions under `TestEvidence/bl050_*`:

1. `partitioned_latency_contract.tsv`
2. `partitioned_performance_matrix.tsv`
3. `partitioned_quality_parity.tsv`
4. `engine_transition_safety.tsv`

Existing BL-050 lane outputs remain required:

1. `status.tsv`
2. `build.log`
3. `highrate_matrix.tsv`
4. `fir_profile.tsv`
5. `failure_taxonomy.tsv`
6. `docs_freshness.log`

## Exit Criteria (Slice B)

1. All hard gates pass on T1 replay (`3/3`) for direct and partitioned matrix rows.
2. Latency contract rows pass for direct and partitioned paths.
3. Transition safety shows no click/zipper contract failures.
4. Docs freshness passes in the same packet.

## Backlog References

1. `Documentation/backlog/bl-050-high-rate-delay-and-fir-hardening.md`
2. `Documentation/backlog/bl-055-fir-convolution-engine.md`
3. `Documentation/backlog/index.md`
