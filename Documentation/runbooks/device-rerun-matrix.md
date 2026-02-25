Title: LocusQ Device Rerun Matrix
Document Type: Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# LocusQ Device Rerun Matrix (BL-030 Slice B)

## Purpose

Define a deterministic, repeatable release-device rerun matrix for `DEV-01..DEV-06` with explicit pass/fail policy, evidence paths, and controlled `N/A` handling.

## Contract References

- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`
- `Documentation/runbooks/release-checklist-template.md`
- `Documentation/testing/production-selftest-and-reaper-headless-smoke-guide.md`

## Execution Policy

1. Use one run directory per execution:
   - `RUN_DIR=TestEvidence/bl030_release_governance_<timestamp>`
2. Record every row as `PASS`, `FAIL`, or `N/A` in:
   - `"$RUN_DIR/device_matrix_results.tsv"`
3. No implicit skips:
   - every row requires an explicit result and evidence path.
4. `N/A` is allowed only where this matrix explicitly permits it.
5. `DEV-01..DEV-05` are required release gates.

## Preflight (Run Once Before DEV Rows)

1. Build standalone + QA runner:
   - `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
2. Create run directory:
   - `mkdir -p "$RUN_DIR"`
3. Baseline docs gate snapshot for the same run:
   - `./scripts/validate-docs-freshness.sh | tee "$RUN_DIR/dev00_docs_freshness.log"`

## DEV Matrix

| DEV ID | Device Profile | Deterministic Steps | Pass Criteria | Evidence Paths | N/A Policy |
|---|---|---|---|---|---|
| `DEV-01` | Quad studio reference (4-speaker) | 1) `./scripts/qa-bl018-profile-matrix-strict-mac.sh \| tee "$RUN_DIR/dev01_bl018_profile_matrix.log"`.<br>2) Verify latest BL-018 bundle `per_profile_results.tsv` row `quadraphonic` has `result=PASS`, `qa_status=PASS`, `diagnostics_match=true`, `rt_safe=true`.<br>3) `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap \| tee "$RUN_DIR/dev01_reaper_headless.log"`.<br>4) Perform fixed speaker ID spot-check on physical quad chain and record notes. | BL-018 `quadraphonic` row passes strict checks, REAPER smoke status is `pass`, and manual speaker-ID notes confirm expected speaker mapping. | `"$RUN_DIR/dev01_bl018_profile_matrix.log"`; `"$RUN_DIR/dev01_reaper_headless.log"`; `"$RUN_DIR/dev01_quad_manual_notes.md"`; referenced BL-018 `per_profile_results.tsv` path. | Not allowed |
| `DEV-02` | Laptop stereo downmix | 1) `./scripts/qa-bl009-headphone-contract-mac.sh \| tee "$RUN_DIR/dev02_bl009_headphone_contract.log"`.<br>2) Verify BL-009 contract `status.tsv` has `determinism_downmix=pass`.<br>3) In standalone, set stereo monitoring path and run fixed left/right orbit check on laptop speakers.<br>4) Capture manual listening notes. | BL-009 downmix determinism is `pass`, no contract failures in `status.tsv`, and manual check reports stable left/right image without dropout/clipping. | `"$RUN_DIR/dev02_bl009_headphone_contract.log"`; BL-009 `status.tsv` path; `"$RUN_DIR/dev02_laptop_manual_notes.md"`. | Not allowed |
| `DEV-03` | Headphone stereo (generic profile) | 1) `./scripts/qa-bl009-headphone-profile-contract-mac.sh \| tee "$RUN_DIR/dev03_bl009_headphone_profile.log"`.<br>2) Verify profile lane contains `determinism_generic_profile=pass` and `profile_airpods_divergence=pass` / `profile_sony_divergence=pass`.<br>3) In standalone, set `Headphone=Stereo Downmix`, `HP Profile=Generic` and run fixed orbit listening check.<br>4) Record manual notes. | BL-009 profile contract passes and manual generic-headphone listening check confirms stable localization and no periodic glitching/dropouts. | `"$RUN_DIR/dev03_bl009_headphone_profile.log"`; BL-009 profile `status.tsv` path; `"$RUN_DIR/dev03_headphone_generic_manual_notes.md"`. | Not allowed |
| `DEV-04` | Headphone stereo (Steam binaural path) | 1) `./scripts/qa-bl009-headphone-contract-mac.sh \| tee "$RUN_DIR/dev04_bl009_headphone_contract.log"`.<br>2) Verify BL-009 self-test detail reports `steamAvailable=true` and no BL-009 check failures.<br>3) In CALIBRATE, set `Monitoring Path=Steam Binaural` and confirm headphone profile stage reaches active state for target device profile.<br>4) Record manual notes with device/profile used. | BL-009 contract passes with Steam available and manual check confirms active Steam binaural path (no fallback stage for target profile/device pairing). | `"$RUN_DIR/dev04_bl009_headphone_contract.log"`; BL-009 contract `status.tsv` path; `"$RUN_DIR/dev04_steam_manual_notes.md"`; optional screenshot `"$RUN_DIR/dev04_steam_active.png"`. | Not allowed |
| `DEV-05` | Built-in mic calibration path | 1) `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh \| tee "$RUN_DIR/dev05_bl026_selftest.log"`.<br>2) In CALIBRATE, set built-in mic input/channel and execute one deterministic sweep cycle (`START MEASURE` to completion/abort policy).<br>3) Capture routing/validation state after run.<br>4) Record notes including mic channel and topology. | BL-026 scoped self-test passes and manual calibration run confirms built-in mic route is selectable, runnable, and reflected in run/validation status without crash. | `"$RUN_DIR/dev05_bl026_selftest.log"`; self-test JSON path; `"$RUN_DIR/dev05_builtin_mic_manual_notes.md"`; optional screenshot `"$RUN_DIR/dev05_builtin_mic_state.png"`. | Not allowed |
| `DEV-06` | External mic calibration path (USB/interface mic) | 1) `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh \| tee "$RUN_DIR/dev06_bl026_selftest.log"`.<br>2) Connect external mic/interface, select external mic channel in CALIBRATE, run deterministic sweep cycle.<br>3) Capture routing/validation state and notes.<br>4) If hardware is unavailable, create explicit waiver file. | Pass when external mic route is selectable/runnable and calibration status updates without crash.<br>If unavailable, row may be `N/A` only with explicit hardware-unavailable waiver note. | `"$RUN_DIR/dev06_bl026_selftest.log"`; self-test JSON path; `"$RUN_DIR/dev06_external_mic_manual_notes.md"`; waiver path `"$RUN_DIR/dev06_external_mic_waiver.md"` when `N/A`. | Allowed only for missing external mic hardware; waiver required |

## Manual Evidence Note Templates

Create these note files per row to keep evidence deterministic:

- `dev01_quad_manual_notes.md`
- `dev02_laptop_manual_notes.md`
- `dev03_headphone_generic_manual_notes.md`
- `dev04_steam_manual_notes.md`
- `dev05_builtin_mic_manual_notes.md`
- `dev06_external_mic_manual_notes.md` or `dev06_external_mic_waiver.md`

Each note should include:
1. Hardware used
2. Exact app/profile settings
3. Observed result
4. PASS/FAIL decision rationale
5. Operator + UTC timestamp

## Device Matrix Result Ledger Template

Use this exact schema for `"$RUN_DIR/device_matrix_results.tsv"`:

```tsv
dev_id	result	timestamp_utc	evidence_path	notes
DEV-01	PASS	<timestamp>	<path>	<summary>
DEV-02	PASS	<timestamp>	<path>	<summary>
DEV-03	PASS	<timestamp>	<path>	<summary>
DEV-04	PASS	<timestamp>	<path>	<summary>
DEV-05	PASS	<timestamp>	<path>	<summary>
DEV-06	PASS|N/A	<timestamp>	<path>	<summary>
```

## Gate Decision Rule

Release-device rerun gate passes only when:

1. `DEV-01..DEV-05` are `PASS`.
2. `DEV-06` is `PASS` or explicit allowed `N/A` with waiver.
3. Every row has at least one evidence artifact path.
