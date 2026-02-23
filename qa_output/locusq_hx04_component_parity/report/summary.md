# QA Suite Summary: `locusq_hx04_component_parity_suite`

- Generated: `2026-02-23T17:23:12Z`
- Status: `PASS`
- Counts: total=`4` pass=`3` warn=`1` fail=`0` error=`0` skip=`0`

## Scenario Status

| Scenario | Status | Duration (ms) | Warnings |
|---|---:|---:|---:|
| `locusq_air_absorption_distance` | `PASS` | 4000.0 | 0 |
| `locusq_calibration_sweep_capture` | `PASS` | 6000.0 | 0 |
| `locusq_emit_dir_spatial_effect` | `WARN` | 6000.0 | 1 |
| `locusq_directivity_aim` | `PASS` | 6000.0 | 0 |

## Top Findings

- `WARN` `locusq_emit_dir_spatial_effect` - [aim_sweep_continuity] 2999 discontinuities (max: 2000)

## Contract Coverage

- Contract scenarios: `0`
- Latency-tagged: `0`
- Smoothing-tagged: `0`
- State-tagged: `0`

## Visual Assets

- Trend deltas: `N/A` (no previous suite result found)
- Status chart: `report/assets/charts/status_rollup.svg`
- Duration chart: `report/assets/charts/scenario_durations.svg`
- Status counts JSON: `report/assets/status_counts.json`
- Trend deltas JSON: `report/assets/trend_deltas.json`
- Metrics CSV: `report/assets/metrics.csv`

## Artifacts

- Suite JSON: `suite_result.json`
- Markdown report: `report/summary.md`
- CI summary: `report/ci_summary.md`
- HTML report: `report/index.html`
- Status counts JSON: `report/assets/status_counts.json`
- Trend deltas JSON: `report/assets/trend_deltas.json`
- Metrics CSV: `report/assets/metrics.csv`
- Status chart: `report/assets/charts/status_rollup.svg`
- Duration chart: `report/assets/charts/scenario_durations.svg`
- Scenario `locusq_air_absorption_distance`: `locusq_air_absorption_distance/result.json`, `locusq_air_absorption_distance/wet.wav`, `locusq_air_absorption_distance/dry.wav`
- Scenario `locusq_calibration_sweep_capture`: `locusq_calibration_sweep_capture/result.json`, `locusq_calibration_sweep_capture/wet.wav`
- Scenario `locusq_emit_dir_spatial_effect`: `locusq_emit_dir_spatial_effect/result.json`, `locusq_emit_dir_spatial_effect/wet.wav`, `locusq_emit_dir_spatial_effect/dry.wav`
- Scenario `locusq_directivity_aim`: `locusq_directivity_aim/result.json`, `locusq_directivity_aim/wet.wav`, `locusq_directivity_aim/dry.wav`
