Title: BL-060 Phase B Statistical Report
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-060 Phase B Statistical Report

- analysis_version: 1.0.0
- timestamp_utc: 2026-03-17T17:40:26.459701+00:00

## Per-Condition Summary

| Condition | N | MAE (°) | FB Confusion | Ext Mean |
|---|---|---|---|---|
| generic_device_eq | 40 | 24.6 | 0.0% | 2.54 |
| generic_no_eq | 40 | 26.0 | 0.0% | 2.57 |
| personalized_device_eq | 40 | 14.1 | 0.0% | 3.68 |
| personalized_no_eq | 40 | 14.2 | 0.0% | 3.76 |

## Gate Metrics

- externalization_improvement_pct: 45.5%
- p_value (Welch t-test, generic vs personalized): 0.0000
- n_generic_trials: 80
- n_personal_trials: 80

## Gate Decision: PASS

- ext_gate (>=20.0%): PASS
- loc_gate (p<0.05): PASS
