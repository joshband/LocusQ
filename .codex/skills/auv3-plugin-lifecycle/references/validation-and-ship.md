Title: AUv3 Validation and Ship Checklist

## Validation Lanes
1. Build graph lane
   - Confirm AUv3 target exists and builds alongside AU/VST3/CLAP targets.
2. Lifecycle lane
   - Validate init, suspend/resume, state restore, and teardown behavior.
3. Contract lane
   - Validate parameter automation, channel layout behavior, and fallback observability.
4. Regression lane
   - Re-run existing non-AUv3 QA lanes to catch format regressions.

## Ship Readiness Evidence
- AUv3 artifact path(s) and version metadata.
- Host matrix outcomes (`PASS/FAIL`) with explicit environment notes.
- Regression summary for AU/VST3/CLAP parity.
- Updated routing docs when specialist scope changes.
