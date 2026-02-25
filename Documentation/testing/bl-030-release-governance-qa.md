Title: BL-030 Release Governance QA
Document Type: Testing Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

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
- `terminalReasonDetail` (human-readable detail)

Bootstrap retry contract:
- One bounded retry is allowed for bootstrap transport/setup failures (`LQ_REAPER_BOOTSTRAP_RETRY_ONCE=1` default).
- Retry must perform cleanup before second attempt.
- Retry must not mask deterministic install failures (for example missing required LocusQ FX).

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

Strict exit semantics:
- `0` only when RL-05 pass criteria are met (`DEV-01..DEV-05=PASS` and `DEV-06=PASS|N/A with waiver`).
- `1` when RL-05 criteria are not met.
- `2` for usage/invocation errors.

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

Blocker categories (fixed):
- `deterministic_missing_manual_evidence`
- `runtime_flake_abrt`
- `not_applicable_with_waiver`

Exit semantics:
- `0` = RL-05 manual evidence complete.
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
- `terminal_reason`: parsed reason string with ABRT markers when present
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
