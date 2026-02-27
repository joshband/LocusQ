Title: BL-030 Release Governance QA
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-26

# BL-030 Release Governance QA

## Purpose

Define the command-lane replay contract used for release-governance decision packets, with explicit blocker taxonomy and deterministic versus transient classification guidance.

## Command Lane Contract

| Lane | Command | Primary Gate |
|---|---|---|
| RL-03 | `./scripts/standalone-ui-selftest-production-p0-mac.sh` | Standalone production self-test |
| RL-04 | `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap` | REAPER host smoke |
| RL-06 | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3` | pluginval strict validation |
| RL-08 | `./scripts/validate-docs-freshness.sh` | docs metadata/freshness gate |
| RL-09 | `rg -n "RL-09|release note|release governance" CHANGELOG.md Documentation/backlog/bl-030-release-governance.md Documentation/testing/bl-030-release-governance-qa.md` | Release-note closeout traceability |
| RL-05-HARNESS | `./scripts/qa-bl030-device-matrix-capture-mac.sh` | Deterministic DEV-01..DEV-06 capture with machine-readable blocker taxonomy |
| RL-05-MANUAL-INTAKE | `./scripts/qa-bl030-manual-evidence-validate-mac.sh --input <checklist.tsv> --out-dir <artifact_dir>` | Deterministic manual-evidence completeness validation |
| RL-04-DIAG | `./scripts/diagnose-reaper-bootstrap-abrt-mac.sh --runs 10 --out-dir <artifact_dir>` | RL-04 bootstrap ABRT diagnostics with crash-report linkage |

Decision packets targeting RL-05 and RL-09 must include these command-lane replays plus explicit governance-state checks for:
- RL-05 device rerun matrix freshness (`DEV-01..DEV-06` evidence recency and pass state)
- RL-09 release-note closeout completeness (`CHANGELOG.md` entry and evidence linkage)

## RL-04 Stage and Terminal Taxonomy Contract (Slice H2)

Canonical command:
- `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`

Required stage classification fields in RL-04 status JSON:
- `stageBootstrapResult`
- `stageInstallResult`
- `stageRenderResult`
- `stageOutputResult`

Required terminal fields in RL-04 status JSON:
- `terminalStage` (`bootstrap|install|render|output|unknown|none`)
- `terminalReasonCode` (deterministic reason token)
- `terminalReasonClass` (`bootstrap_failed|install_failed|render_failed|output_failed|signal|abrt|other|none`)
- `terminalSignalNumber` (numeric signal when available, otherwise `0`)
- `terminalReasonDetail` (human-readable detail)

Bootstrap retry contract:
- Retry attempts are deterministic and bounded:
  - `LQ_REAPER_BOOTSTRAP_MAX_ATTEMPTS` (default derives from `LQ_REAPER_BOOTSTRAP_RETRY_ONCE`: `2` when enabled, `1` otherwise).
  - `LQ_REAPER_BOOTSTRAP_BACKOFF_BASE_SEC`
  - `LQ_REAPER_BOOTSTRAP_BACKOFF_STEP_SEC`
  - `LQ_REAPER_BOOTSTRAP_BACKOFF_MAX_SEC`
- Retry must perform cleanup before each retry attempt.
- Retry must not mask deterministic install failures (for example missing required LocusQ FX).
- Terminal taxonomy for bootstrap must use `bootstrap_failed_*`; signal-path failures must classify under `signal`/`abrt`.

RL-04 strict exit semantics:
- `0` only when all required stages pass and no terminal failure reason is set.
- `1` on any lane failure; terminal reason fields must explain the failure.
- `2` only for invocation/preflight errors (usage, missing executable, etc.).

## Required Evidence Bundle

The packet directory must include:
- `status.tsv`
- `rl_gate_matrix.tsv`
- `release_decision.md`
- `selftest.log`
- `reaper_smoke.log`
- `pluginval.log`
- `docs_freshness.log`
- `blocker_taxonomy.tsv`

## Blocker Classification Rules

### Deterministic Blocker
Use when one of the following is true:
- Governance state is incomplete (for example RL-05/RL-09 missing required artifacts/content).
- Docs freshness fails reproducibly from a known file contract violation.
- Any gate has a stable, repeatable fail condition not tied to process startup race.

### Transient Flake
Use when all of the following are true:
- Failure mode is runtime-process abort/startup instability.
- Earlier authoritative replay on current branch passed the same command.
- No code-path contract change in this slice explains the failure.

## Triage Notes

1. Capture exact exit code for each command even when logs are sparse.
2. For abort-class failures, record failure signature (`app_exited_before_result`, `Abort trap`, no output, and any crash report pointer if emitted).
3. Keep governance-state blockers (RL-05/RL-09) separate from runtime-lane flakes in both `rl_gate_matrix.tsv` and `blocker_taxonomy.tsv`.
4. Release decision is `NO-GO` while any deterministic blocker remains open.

## RL-09 Verification Contract

RL-09 is `PASS` only when all checks are true:
1. `CHANGELOG.md` has an explicit BL-030 release-governance closeout entry for the active window.
2. The closeout entry references the RL-09 evidence packet path.
3. The closeout entry also states current RL-05 state (must not imply RL-05 pass when blocked).
4. The RL-09 grep traceability command returns matches across:
   - `CHANGELOG.md`
   - `Documentation/backlog/bl-030-release-governance.md`
   - `Documentation/testing/bl-030-release-governance-qa.md`

## RL-05 Clean Replay (Slice G1)

Packet root:
- `TestEvidence/bl030_rl05_clean_replay_g1_<timestamp>/`

Required command replay set for DEV matrix packet:
1. `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8`
2. `./scripts/validate-docs-freshness.sh`
3. `./scripts/qa-bl018-profile-matrix-strict-mac.sh`
4. `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`
5. `./scripts/qa-bl009-headphone-contract-mac.sh` (DEV-02)
6. `./scripts/qa-bl009-headphone-profile-contract-mac.sh` (DEV-03)
7. `./scripts/qa-bl009-headphone-contract-mac.sh` (DEV-04)
8. `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` (DEV-05)
9. `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` (DEV-06 pre-waiver capture)

Per-device recording contract:
- Emit one `dev_matrix_results.tsv` row for each `DEV-01..DEV-06`.
- Record both automated command outcomes and manual-check completion state.
- If a row contains both runtime abort and missing manual validation:
  - set `classification=deterministic_missing_manual_evidence` in `dev_matrix_results.tsv` (deterministic precedence),
  - emit both blocker rows in `blocker_taxonomy.tsv` (`runtime_flake_abrt` and `deterministic_missing_manual_evidence`).
- `DEV-06` may be `N/A` only when `dev06_external_mic_waiver.md` exists and states hardware unavailability.

Decision rule for RL-05 packet:
- PASS only when `DEV-01..DEV-05=PASS` and `DEV-06=PASS|N/A-with-waiver`.
- Any missing required manual check on `DEV-01..DEV-05` remains a deterministic blocker even if automated checks pass.

## RL-05 Harness Contract (Slice G3)

Canonical command:
- `./scripts/qa-bl030-device-matrix-capture-mac.sh`

Optional parameters:
- `--out-dir <path>`
- `--dev01-manual-notes <path>`
- `--dev02-manual-notes <path>`
- `--dev03-manual-notes <path>`
- `--dev04-manual-notes <path>`
- `--dev05-manual-notes <path>`
- `--dev06-manual-notes <path>`
- `--dev06-waiver <path>`
- `--skip-build`

DEV-06 waiver preflight contract:
- If `--dev06-waiver` is provided, the path must exist, be a regular file, be readable, and be non-empty.
- Invalid waiver-path input is a deterministic RL-05 failure and must not fall back to DEV-06 selftest replay.
- Invalid waiver-path failures are emitted as:
  - `blocker_taxonomy.tsv` row with `category=deterministic_missing_manual_evidence`
  - machine-readable reason code in `detail`:
    - `dev06_waiver_path_missing`
    - `dev06_waiver_path_not_file`
    - `dev06_waiver_path_unreadable`
    - `dev06_waiver_path_empty`
  - `status.tsv` step `dev06_waiver_preflight` with matching `reason_code=...`

Valid waiver-path behavior:
- `status.tsv` emits `dev06_waiver_preflight` with `reason_code=dev06_waiver_path_valid`.
- Existing `DEV-06` `N/A` waiver flow remains unchanged (`classification=not_applicable_with_waiver`).

Machine-readable outputs:
- `status.tsv` (`step`, `result`, `exit_code`, `detail`, `artifact`)
- `dev_matrix_results.tsv` (`dev_id`, `result`, `timestamp_utc`, `classification`, `evidence_path`, `notes`)
- `blocker_taxonomy.tsv` (`blocker_id`, `dev_id`, `category`, `detail`, `evidence_path`)
- `replay_transcript.log` (command chronology)
- `command_transcript.log` (compat copy)
- `harness_contract.md`

Blocker category schema (fixed):
- `deterministic_missing_manual_evidence`
- `runtime_flake_abrt`
- `not_applicable_with_waiver`

## RL-05 Deterministic Capture Hardening (Slice N12)

Preflight execution order (before lane steps):
1. `dev06_waiver_preflight` (when `--dev06-waiver` is provided)
2. `preflight_process_drain`
3. `preflight_warmup`
4. `preflight_build` (unless `--skip-build`)
5. `preflight_selftest_warmup` (bounded BL-009 scoped bootstrap selftest; required when build succeeds)

Bounded preflight process-drain contract:
- Stale process drain executes against lane-related stale commands/binaries only.
- TERM window: `4` seconds.
- KILL window: `4` seconds.
- Poll interval: `1` second.
- Total bound: `8` seconds.
- If drain cannot clear remaining PIDs within bound, lane fails closed with:
  - `status.tsv` step `preflight_process_drain` = `FAIL`
  - `blocker_taxonomy.tsv` row category `runtime_flake_abrt` detail token `preflight_process_drain_timeout`
  - overall RL-05 decision `FAIL` (exit `1`)

Deterministic warmup contract:
- `preflight_warmup` is a bounded fixed sleep (`2` seconds) after successful drain.
- Warmup emits machine-readable `status.tsv` detail `warmup_sleep_seconds=2`.
- `preflight_selftest_warmup` is a bounded bootstrap selftest gate before DEV lane commands:
  - command: `LOCUSQ_UI_SELFTEST_BL009=1 LOCUSQ_UI_SELFTEST_SCOPE=bl009 ./scripts/standalone-ui-selftest-production-p0-mac.sh`
  - max attempts: `3`
  - per-attempt timeout: `90` seconds
  - retry barrier between attempts: same bounded drain + fixed warmup sleep (`2` seconds)
- If `preflight_selftest_warmup` never passes within bound, lane fails closed with:
  - `status.tsv` step `preflight_selftest_warmup` = `FAIL`
  - `blocker_taxonomy.tsv` row category `runtime_flake_abrt` detail token `preflight_selftest_warmup_failed`
  - overall RL-05 decision `FAIL` (exit `1`)

Deterministic retry contract:
- Lane command execution uses bounded retry (`max_attempts=2`) with fixed recovery barrier between attempts:
  - stale-process drain (same bounded rules as preflight)
  - fixed retry warmup sleep (`2` seconds)
- Lane commands are executed with deterministic selftest override env to reduce first-run payload flake without bypassing assertions:
  - `LOCUSQ_UI_SELFTEST_LAUNCH_READY_DELAY_SECONDS=2`
  - `LOCUSQ_UI_SELFTEST_PROCESS_DRAIN_TIMEOUT_SECONDS=18`
  - `LOCUSQ_UI_SELFTEST_RESULT_JSON_SETTLE_TIMEOUT_SECONDS=4`
  - `LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_MAX_ATTEMPTS=3`
  - `LOCUSQ_UI_SELFTEST_AUTO_ASSERTION_RETRY_DELAY_SECONDS=2`
  - `LOCUSQ_UI_SELFTEST_TARGETED_CHECK_MAX_ATTEMPTS=6`
- BL-009 lane calls (`DEV-02..DEV-04`) pin `LOCUSQ_UI_SELFTEST_SCOPE=bl009` to keep BL-009 checks strict while excluding unrelated full-scope payload assertions from those lanes.
- Retry does not change strict pass criteria; final lane result remains derived from final command exit and manual evidence rules.
- `status.tsv` command steps remain machine-readable and include `attempts=<N>` in `detail`.

Deterministic machine-readable status requirements (N12 additive):
- `status.tsv` must include:
  - `preflight_process_drain` with detail keys:
    - `result`
    - `initial_pids`
    - `remaining_pids`
    - `forced_kill`
    - `elapsed_seconds`
    - `timeout_seconds`
  - `preflight_warmup` with detail key `warmup_sleep_seconds`
  - `preflight_selftest_warmup` with detail keys:
    - `attempts`
    - `max_attempts`
    - `timeout_seconds`
    - `retry_sleep_seconds`
    - `launch_ready_delay_seconds`
    - `targeted_check_max_attempts`
- Existing strict exit semantics remain unchanged:
  - `0` pass
  - `1` gate fail
  - `2` usage/invocation error

Strict exit semantics:
- `0` only when RL-05 pass criteria are met (`DEV-01..DEV-05=PASS` and `DEV-06=PASS|N/A with waiver`).
- `1` when RL-05 criteria are not met.
- `2` for usage/invocation errors.

## RL-05 Replay Reconcile Wrapper (Slice N5)

Canonical command:
- `./scripts/qa-bl030-rl05-replay-reconcile-mac.sh --notes-dir <dir> [--dev06-waiver <path>] [--out-dir <path>]`

Purpose:
- Execute a deterministic RL-05 reconciliation chain across existing scripts and publish one unified machine-readable closure verdict.

Required wrapper sequence (fixed order):
1. `qa-bl030-manual-evidence-pack-mac.sh`
2. `qa-bl030-manual-evidence-validate-mac.sh`
3. `qa-bl030-device-matrix-capture-mac.sh`

Wrapper inputs:
- `--notes-dir <dir>` (required)
- `--dev06-waiver <path>` (optional pass-through to capture lane)
- `--out-dir <path>` (optional)

Wrapper machine-readable outputs:
- `status.tsv`
- `validation_matrix.tsv`
- `rl05_reconcile_summary.tsv`
- `blocker_taxonomy.tsv` (aggregated from pack/validate/capture packets)

Wrapper strict exit semantics:
- `0`: RL-05 green (`pack=0`, `validate=0`, `capture=0`)
- `1`: RL-05 blocker remains
- `2`: usage/preflight error (wrapper or child invocation contract)

## RL-05 Authoritative Replay Closure (Slice N10)

Canonical fixture path contract:
- Create DEV-06 waiver fixture under the slice-local packet:
  - `TestEvidence/bl030_rl05_authoritative_n10_<timestamp>/fixtures/dev06_external_mic_waiver.md`
- All replay invocations in this slice must reference that exact slice-local waiver path.

Canonical replay sequence:
1. `./scripts/qa-bl030-manual-evidence-pack-mac.sh --notes-dir TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes --out-dir TestEvidence/bl030_rl05_authoritative_n10_<timestamp>/pack`
2. `./scripts/qa-bl030-manual-evidence-validate-mac.sh --input TestEvidence/bl030_rl05_authoritative_n10_<timestamp>/pack/manual_evidence_checklist.tsv --out-dir TestEvidence/bl030_rl05_authoritative_n10_<timestamp>/validate`
3. `./scripts/qa-bl030-device-matrix-capture-mac.sh --out-dir TestEvidence/bl030_rl05_authoritative_n10_<timestamp>/capture_run1 --dev01-manual-notes TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes/dev01_quad_manual_notes.md --dev02-manual-notes TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes/dev02_laptop_manual_notes.md --dev03-manual-notes TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes/dev03_headphone_generic_manual_notes.md --dev04-manual-notes TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes/dev04_steam_manual_notes.md --dev05-manual-notes TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes/dev05_builtin_mic_manual_notes.md --dev06-manual-notes TestEvidence/bl030_rl05_real_closure_m2_20260226T155558Z/manual_notes/dev06_external_mic_manual_notes.md --dev06-waiver TestEvidence/bl030_rl05_authoritative_n10_<timestamp>/fixtures/dev06_external_mic_waiver.md`
4. Repeat step 3 for `capture_run2` and `capture_run3` using the same `--dev06-waiver` path.

Strict closure semantics for N10:
- `PASS` only when `pack=0`, `validate=0`, and all three capture runs exit `0` with `rl05_gate_decision=PASS`.
- Any mixed capture outcome across run1/run2/run3 is a deterministic closure failure for this slice and must be recorded in:
  - `validation_matrix.tsv` (overall `FAIL`)
  - `rl05_replay_summary.tsv` (`replay_consistency=MIXED`)
  - `blocker_taxonomy.tsv` (`category=replay_nondeterminism`)

## RL-05 Closure Replay Packet (Slice G4)

Canonical replay command:
- `./scripts/qa-bl030-device-matrix-capture-mac.sh --out-dir TestEvidence/bl030_rl05_replay_g4_<timestamp>`

Required replay artifacts:
- `status.tsv`
- `dev_matrix_results.tsv`
- `blocker_taxonomy.tsv`
- `release_decision.md` (explicit RL-05 PASS/FAIL and GO/NO-GO rationale)
- `replay_transcript.log`
- `docs_freshness.log`

## RL-05 Manual Evidence Closure Packet (Slice G5)

Canonical command:
- `./scripts/qa-bl030-device-matrix-capture-mac.sh --out-dir TestEvidence/bl030_rl05_manual_closure_g5_<timestamp>`

Required artifacts:
- `status.tsv`
- `dev_matrix_results.tsv`
- `blocker_taxonomy.tsv`
- `manual_evidence_checklist.tsv`
- `release_decision.md`
- `docs_freshness.log`

Manual evidence checklist contract:
- `manual_evidence_checklist.tsv` must include one row per `DEV-01..DEV-06`.
- Columns: `dev_id`, `manual_evidence_path`, `present`, `status`, `notes`.
- `present` values are restricted to `yes|no`.
- Any `present=no` row must produce `status=missing` with an exact missing artifact path.

Decision contract:
- `release_decision.md` must explicitly state:
  - RL-05 state (`PASS|FAIL`)
  - RL-09 state (`PASS`, unchanged)
  - overall `GO|NO-GO`
  - exact next actions when RL-05 is `FAIL`

## RL-05 Manual Evidence Intake Harness (Slice G6)

Canonical command:
- `./scripts/qa-bl030-manual-evidence-validate-mac.sh --input <manual_evidence_checklist.tsv> --out-dir TestEvidence/bl030_rl05_manual_intake_g6_<timestamp>/run_*`

Checklist schema (required columns, exact names):
- `device_id`
- `evidence_status`
- `artifact_path`
- `operator`
- `timestamp_iso8601`
- `notes`

Required device rows:
- `DEV-01`
- `DEV-02`
- `DEV-03`
- `DEV-04`
- `DEV-05`
- `DEV-06`

Manual intake outputs:
- `status.tsv`
- `manual_evidence_validation.tsv`
- `blocker_taxonomy.tsv`
- `harness_contract.md`

Validation rules:
- Fail when any required row is missing.
- Fail when any required field is empty.
- Fail when `artifact_path` does not exist.
- Fail on unknown/duplicate `device_id`.
- Fail on invalid `timestamp_iso8601` format (required: `YYYY-MM-DDTHH:MM:SSZ`).
- DEV-06 waiver handling:
  - `evidence_status=waived|not_applicable_with_waiver` is valid only for `DEV-06` when `artifact_path` exists.
  - A valid DEV-06 waiver is non-failing (`manual_evidence_validation.tsv` row reason `waiver_applied`).
  - Waiver status on `DEV-01..DEV-05` is invalid and fails intake.

Blocker categories (fixed):
- `deterministic_missing_manual_evidence`
- `runtime_flake_abrt`
- `not_applicable_with_waiver` (invalid waiver usage only)

Exit semantics:
- `0` = RL-05 manual evidence complete (`DEV-01..DEV-05` complete and `DEV-06` complete or valid waiver).
- `1` = incomplete/invalid manual evidence.

## RL-04 Bootstrap ABRT Diagnostics Harness (Slice I2)

Canonical command:
- `./scripts/diagnose-reaper-bootstrap-abrt-mac.sh --runs 10 --out-dir TestEvidence/bl030_rl04_abrt_diag_i2_<timestamp>`

Purpose:
- Re-run RL-04 bootstrap lane repeatedly and separate terminal signatures into machine-readable taxonomy for deterministic vs transient analysis.

Required diagnostics outputs:
- `status.tsv`
- `replay_runs.tsv`
- `crash_taxonomy.tsv`
- `top_hypotheses.md`
- `repro_commands.md`
- `command_transcript.log`

Per-run capture contract:
- `exit_code`: wrapper script exit for the replay lane
- `stage`: failing stage (`bootstrap`, `render`, `post_render`, `complete`, `unknown`)
- `terminal_reason`: deterministic token from RL-04 lane (`bootstrap_failed_*`, `install_failed_*`, `render_failed_*`, `output_failed_*`, etc.)
- `terminal_reason_class`: normalized class (`bootstrap_failed|install_failed|render_failed|output_failed|signal|abrt|other|none`)
- `terminal_signal_number`: signal number when present (for example `6` for ABRT), otherwise `0`
- `classification`: deterministic bucket (`pass`, `transient_runtime_abort`, `transient_runtime_signal`, `deterministic_*`)
- `crash_report_present`: `yes|no`
- `crash_report_path`: path when a new matching crash report is detected

Use:
- This lane is diagnostics-first and should still emit complete artifacts in failing environments.
- GO/NO-GO remains derived from RL-04 gate criteria in release governance packets; this diagnostics lane provides failure signatures and repro anchors.

## RL Gate Consolidation Packet (Slice I3)

Purpose:
- Produce one authoritative release-gate decision packet that consolidates prior lane outputs into a single RL-03..RL-09 matrix.

Canonical consolidation inputs:
- `TestEvidence/bl030_rl05_manual_closure_g5_<timestamp>/`
- `TestEvidence/bl030_rl05_manual_intake_g6_<timestamp>/`
- `TestEvidence/bl030_rl05_manual_notes_g7_<timestamp>/`
- `TestEvidence/bl030_runtime_replay_g8_<timestamp>/`
- `TestEvidence/bl030_rl04_reaper_stability_h2_<timestamp>/`
- `TestEvidence/bl030_rl06_pluginval_h3_<timestamp>/`

Consolidation outputs:
- `status.tsv`
- `rl_gate_matrix.tsv`
- `blocker_taxonomy.tsv`
- `release_decision.md`
- `unblock_checklist.md`
- `docs_freshness.log`

Consolidation rules:
- Matrix rows must include `RL-03..RL-09` with current status and evidence pointers.
- Blockers must be split into deterministic vs transient/runtime classes.
- Release decision must explicitly state GO/NO-GO and list exact unblock criteria for each failing gate.

## RL-05 Manual Evidence Authoring Pack (Slice G7)

### Operator Fill Procedure (Compact)

1. Copy the DEV-specific template (`dev01`..`dev06`) into the active RL-05 packet directory and keep the filename unchanged.
2. Fill all required fields exactly once: `device_id`, `evidence_status`, `artifact_path`, `operator`, `timestamp_iso8601`, `notes`.
3. Set `artifact_path` to an existing artifact inside the same evidence packet; use repository-relative paths.
4. Set `timestamp_iso8601` using UTC format `YYYY-MM-DDTHH:MM:SSZ` at the time of manual verification.
5. Transcribe the six field values into the G6 intake checklist TSV (`device_id`, `evidence_status`, `artifact_path`, `operator`, `timestamp_iso8601`, `notes`) and run:
   - `./scripts/qa-bl030-manual-evidence-validate-mac.sh --input <checklist.tsv> --out-dir <run_dir>`

### Template Field Semantics

| Field | Required Value Rule | G6 Mapping |
|---|---|---|
| `device_id` | One of `DEV-01..DEV-06` matching the template file | `device_id` |
| `evidence_status` | `pass`, `fail`, or `not_applicable_with_waiver` | `evidence_status` |
| `artifact_path` | Existing repository-relative artifact path | `artifact_path` |
| `operator` | Human operator identifier | `operator` |
| `timestamp_iso8601` | UTC timestamp in `YYYY-MM-DDTHH:MM:SSZ` | `timestamp_iso8601` |
| `notes` | Short deterministic observation summary | `notes` |

## RL-05 Manual Evidence Packet Compiler (Slice I1)

Canonical command:
- `./scripts/qa-bl030-manual-evidence-pack-mac.sh --notes-dir <manual_notes_dir> --out-dir <artifact_dir>`

Input note contract:
- Exactly one note file for each DEV pattern:
  - `dev01_*manual_notes.md`
  - `dev02_*manual_notes.md`
  - `dev03_*manual_notes.md`
  - `dev04_*manual_notes.md`
  - `dev05_*manual_notes.md`
  - `dev06_*manual_notes.md`
- Each note must include fields:
  - `device_id`
  - `evidence_status`
  - `artifact_path`
  - `operator`
  - `timestamp_iso8601`
  - `notes`

Compiled checklist schema (exact columns, tab-separated):
- `device_id`
- `evidence_status`
- `artifact_path`
- `operator`
- `timestamp_iso8601`
- `notes`

Normalization rule:
- Compiler accepts authoring synonyms from manual notes:
  - `pass` -> `complete`
  - `fail` -> `incomplete`
- All emitted checklist rows must use G6-compatible status tokens.

Compiler outputs:
- `status.tsv`
- `manual_evidence_checklist.tsv`
- `pack_validation.tsv`
- `blocker_taxonomy.tsv`
- `harness_contract.md`

Compiler exit semantics:
- `0` checklist complete/valid.
- `1` missing/invalid note inputs.
- `2` usage error.

## RL-05 Closure Evidence Packaging (Slice M1)

Purpose:
- Build an auditor-facing RL-05 closure packet from prior deterministic artifacts only (no new runtime lane executions).

Canonical input packets:
- `TestEvidence/bl030_rl05_manual_closure_g5_<timestamp>/`
- `TestEvidence/bl030_rl05_manual_intake_g6_<timestamp>/`
- `TestEvidence/bl030_rl05_manual_pack_i1_<timestamp>/`
- `TestEvidence/bl030_gate_consolidation_i3_<timestamp>/`

Required M1 outputs:
- `status.tsv`
- `validation_matrix.tsv`
- `rl05_closure_index.tsv` (artifact inventory with source packet and checksum)
- `rl05_operator_chain_of_custody.tsv`
- `rl05_blocker_statement.md`
- `promotion_decision_draft.md`
- `docs_freshness.log`

Proof-point classification tokens (required):
- `PRESENT_VALID`
- `PRESENT_INCOMPLETE`
- `MISSING`

Decision rule:
- If any required RL-05 proof point is `PRESENT_INCOMPLETE` or `MISSING`, packet result is `FAIL`.
- `FAIL` packets must enumerate exact blocker IDs in both `validation_matrix.tsv` and `rl05_blocker_statement.md`.

## RL-05 Closure Evidence Packaging Snapshot (Slice M1, 2026-02-26)

- Packet directory: `TestEvidence/bl030_rl05_closure_pack_m1_20260226T155124Z`
- Result: `FAIL`
- Deterministic blocker IDs:
  - `BL030-M1-001`
  - `BL030-M1-002`
  - `BL030-M1-003`
  - `BL030-M1-004`
  - `BL030-M1-005`
  - `BL030-M1-006`
  - `BL030-M1-007`
- Key M1 determinations:
  1. Authoritative RL-05 closure packet (`G5`) still reports `rl05_gate_decision=FAIL`.
  2. G5 manual checklist remains incomplete (`present_yes=0`, `present_no=6`).
  3. G6 intake remains invalid (`header_schema=FAIL`; unresolved required rows).
  4. Consolidated I3 packet still reports `RL-05=FAIL` with deterministic blockers retained.
  5. Compiler packet I1 is valid but does not clear authoritative G5/G6 closure blockers by itself.

## RL-05 Closure Evidence Packaging Snapshot (Slice M1 rerun, 2026-02-26)

- Packet directory: `TestEvidence/bl030_rl05_closure_pack_m1_20260226T155213Z`
- Result: `FAIL`
- Deterministic blocker IDs:
  - `BL030-M1-001`
  - `BL030-M1-002`
  - `BL030-M1-003`
  - `BL030-M1-004`
  - `BL030-M1-005`
  - `BL030-M1-006`
  - `BL030-M1-007`
  - `BL030-M1-008`
- Key M1 rerun determinations:
  1. G5 remains authoritative and still reports `rl05_gate_decision=FAIL`.
  2. G5 checklist remains non-intake schema with `present=no` for `DEV-01..DEV-06`.
  3. G6 real-input intake remains `FAIL` (`header_schema=FAIL`, `manual_evidence_gate=FAIL`).
  4. I1 compiler output remains fixture-valid only and cannot close RL-05 without authoritative note linkage.
  5. I3 consolidated matrix remains `RL-05=FAIL`; release posture stays `NO-GO`.
