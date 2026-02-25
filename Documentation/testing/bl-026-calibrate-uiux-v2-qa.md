Title: BL-026 CALIBRATE UI/UX v2 QA Contract
Document Type: Testing Contract
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-026 CALIBRATE UI/UX v2 QA Contract

## Scope

Defines acceptance for BL-026 Slice D+E:
- deterministic CALIBRATE diagnostics cards
- host/WebView resize reliability under compact and tight panel widths
- regression safety for BL-029 scoped self-test lanes

## Deterministic Diagnostics Contract

All CALIBRATE validation cards must publish one of these states only:
- `PASS`
- `FAIL`
- `UNTESTED`

State labels above are contract strings and are consumed by scoped self-tests.

## QA Matrix

| ID | Surface | Expected Result | Validation Method |
|---|---|---|---|
| `UI-P1-026D` | Profile Activation diagnostic card (`cal-validation-profile-chip`) | Card state resolves to `PASS` or `FAIL` after monitoring-path/device-profile checks; detail text is populated | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` |
| `UI-P1-026E` | Downmix Path diagnostic card (`cal-validation-downmix-chip`) | Card state resolves to `PASS` or `FAIL` when downmix is required; detail text is populated | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` |
| `UI-P1-026E` | Compact/tight CALIBRATE layout | Required controls and diagnostics cards remain non-clipped and usable for `layout-compact` and `layout-tight` variants | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` |
| `UI-P1-026C` (guard) | Calibration lifecycle | Start/abort flow remains deterministic with no side-effect regressions introduced by Slice D+E UI changes | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` |
| `UI-P1-029A/B/C` (regression guard) | BL-029 reactive lanes | BL-029 scoped self-test remains passing after CALIBRATE UI changes | `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` |

## Manual Spot Checks

1. Open CALIBRATE tab in standalone and resize host/editor window from wide to narrow.
2. Confirm diagnostics cards remain readable and controls remain clickable.
3. Trigger start/abort once; verify no duplicate starts, no stuck `ABORT` state.
4. Switch monitoring path (`Speakers`, `Stereo Downmix`, `Steam Binaural`) and verify Profile Activation/Downmix diagnostics change deterministically.

## Evidence Contract

Evidence bundle path:
- `TestEvidence/bl026_slice_de_<timestamp>/`

Required artifacts:
- `status.tsv`
- `node_check.log`
- `build.log`
- `selftest_bl026_runs.tsv`
- `selftest_bl029_regression.tsv`
- `resize_diagnostics_notes.md`
- `docs_freshness.log`
