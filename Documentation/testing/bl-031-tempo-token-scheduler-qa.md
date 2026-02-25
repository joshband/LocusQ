Title: BL-031 Tempo Token Scheduler QA Lane
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-031 Tempo Token Scheduler QA Lane

## Purpose
Define a deterministic QA lane for BL-031 Slice D that validates tempo-token scheduler behavior under:
1. Fixed tempo.
2. Tempo ramps.
3. Transport stop/start transitions.
4. High-tempo bounded-capacity conditions.
5. Missing host-time fallback.

This lane combines:
1. `locusq_qa` scenario execution (`qa/scenarios/locusq_bl031_tempo_ramp_suite.json`).
2. Source-contract checks against `Source/VisualTokenScheduler.h`.
3. Deterministic token-model checks emitted as machine-readable evidence.

## Acceptance IDs

| Acceptance ID | Validation Contract | Evidence Signal |
|---|---|---|
| `UI-P2-031A` | Token sequence monotonicity and bounded tokens-per-block at high tempo | `token_monotonicity.tsv` (`high_tempo_300`) |
| `UI-P2-031B` | Beat/downbeat interval stability at fixed 120 BPM | `token_monotonicity.tsv` (`constant_120`) |
| `UI-P2-031C` | Deterministic ramp density growth + transport stop/start resume behavior | `token_monotonicity.tsv` (`tempo_ramp_60_to_180`, `transport_stop_start`) |
| `UI-P2-031D` | Host-time unavailable path emits zero tokens | `token_monotonicity.tsv` (`missing_host_time`) |

## Scenario and Lane

Scenario file:
- `qa/scenarios/locusq_bl031_tempo_ramp_suite.json`

Lane script:
- `scripts/qa-bl031-tempo-token-lane-mac.sh`

Key lane outputs (under `BL031_OUT_DIR`):
1. `status.tsv`
2. `qa_lane.log`
3. `scenario_result.log`
4. `token_monotonicity.tsv`
5. `token_summary.json`
6. `scenario_result.json` (copy of QA `result.json`)

## Validation Commands

```bash
cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
./scripts/qa-bl031-tempo-token-lane-mac.sh
./scripts/validate-docs-freshness.sh
```

Recommended explicit evidence path:

```bash
BL031_OUT_DIR="TestEvidence/bl031_slice_d_$(date -u +%Y%m%dT%H%M%SZ)" \
./scripts/qa-bl031-tempo-token-lane-mac.sh
```

## Pass Criteria

`PASS` requires all of the following:
1. Build step succeeds (`locusq_qa` + `LocusQ_Standalone`).
2. Scenario run succeeds and produces `result.json` with `status=PASS`.
3. Source-contract tokens required by scenario are present in `Source/VisualTokenScheduler.h`.
4. All deterministic token cases in `token_monotonicity.tsv` return `PASS`.

## Failure Triage

1. `build_targets=FAIL`:
   - Inspect `build.log`.
2. `scenario_exec` or `scenario_status=FAIL`:
   - Inspect `scenario_run.log` and copied `scenario_result.json`.
3. `source_contract_tokens=FAIL`:
   - Check missing token markers from `status.tsv` against scheduler source.
4. `token_monotonicity=FAIL`:
   - Inspect `token_monotonicity.tsv` `details` column and `token_model.log`.

## Notes

The deterministic token-model checks are intentionally bounded and reproducible from scenario-defined timing cases. They enforce monotonic and capacity contracts required for BL-031 promotion while remaining independent of non-deterministic host scheduling behavior.
