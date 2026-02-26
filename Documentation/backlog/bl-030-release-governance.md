Title: BL-030 Release Governance and Device Rerun
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-26

# BL-030: Release Governance and Device Rerun

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Validation (RL-09 PASS retained; RL-05 FAIL remains deterministic blocker per G5/G6/I1; RL-03 remains red after K1 hardening with residual `app_exited_before_result` flake while RL-04/RL-06 remain red through H2/H3 with I2 diagnostics; I3 consolidation packet remains NO-GO) |
| Owner Track | Track G — Release/Governance |
| Depends On | BL-024 (Done), BL-025 (Done), HX-06 |
| Blocks | — |
| Annex Spec | (inline) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Low | S | Release checklist template |
| B | Med | M | Device rerun matrix (DEV-01..DEV-06) |
| C | Med | M | CI integration for release-gate checks |
| D | Low | S | First execution with evidence |

## Objective

Operationalize a recurring release/device-rerun governance checklist that validates every release candidate across device profiles (quad studio, laptop stereo, headphone) before shipping. Success: every release has a repeatable checklist run with explicit pass/fail evidence per device profile and no implicit N/A handling.

## Scope & Non-Scope

**In scope:**
- Release checklist template with ordered gates
- Device rerun matrix covering ADR-0006 profiles (quad studio, laptop stereo downmix, headphone stereo)
- CI integration so release-gate checks run automatically
- First execution producing baseline evidence

**Out of scope:**
- Automated device switching (manual testing required for real hardware)
- New device profiles beyond ADR-0006 scope
- Signing, notarization, or distribution infrastructure (that's `$skill_ship`)

## Architecture Context

- Device profiles: ADR-0006 defines quad studio (reference), laptop stereo downmix, headphone stereo
- Release baseline: BL-024 established REAPER host automation and headless render pipeline
- Freshness gate: ADR-0005 requires synchronized Tier 0 surface updates on closeout
- Artifact tracking: ADR-0010 governs retention (promoted summaries tracked, run-scoped evidence local-only)
- RT audit: HX-06 must be active before release governance promotion (recurring safety guard)
- Ship workflow: `$skill_ship` handles packaging/distribution; this item governs pre-ship validation

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Release checklist template | `Documentation/runbooks/release-checklist-template.md` | BL-024, BL-025 done | Template reviewed and committed |
| B | Device rerun matrix | `Documentation/runbooks/device-rerun-matrix.md` | Slice A done | DEV-01..DEV-06 checks defined with pass criteria |
| C | CI integration | `.github/workflows/` or scripts | HX-06 active, Slice B done | Release-gate job runs on tag push |
| D | First execution | `TestEvidence/` | Slice C done | Baseline evidence captured for all device profiles |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/plan BL-030 Slice A: Release checklist template
Load: $skill_docs, $skill_plan, $skill_ship

Objective: Create Documentation/runbooks/release-checklist-template.md — a repeatable
release validation checklist with ordered gates.

Checklist gates (in order):
1. All P1 backlog items are Done
2. HX-06 RT audit lane is green
3. Production self-test passes (standalone)
4. REAPER headless render smoke passes (BL-024 baseline)
5. Device rerun matrix passes (DEV-01..DEV-06)
6. pluginval validation passes (VST3, AU)
7. CLAP validator passes (if CLAP build active)
8. ADR-0005 freshness gate passes
9. CHANGELOG.md updated with release notes
10. Build artifacts packaged (VST3/AU/Standalone)

Each gate must have: gate ID, description, command or manual steps, pass criteria,
evidence artifact path, N/A policy (explicit — no implicit skips).

Constraints:
- Template must be copy-pasteable for each release
- N/A handling must be explicit: if a gate is not applicable, document WHY
- Follow ADR-0010 artifact tracking: promoted summaries tracked, run-scoped evidence local

Evidence:
- Documentation/runbooks/release-checklist-template.md (the template itself)
```

### Slice A — Standalone Fallback Prompt

```
You are implementing BL-030 Slice A for LocusQ, a JUCE spatial audio plugin.

PROJECT CONTEXT:
- LocusQ ships as VST3, AU, and Standalone (optionally CLAP) on macOS
- Release validation currently relies on ad-hoc script runs and manual checks
- Existing validation infrastructure:
  - Production self-test: built into standalone app
  - REAPER headless render: scripts/qa-standalone-calibration-binaural-multichannel-mac.sh
  - pluginval: external validator for VST3/AU
  - CLAP validator: clap-validator tool
  - Docs freshness: scripts/validate-docs-freshness.sh
  - QA harness: ctest-based test suite (45+ tests)
- ADR-0005 requires synchronized updates to status.json, README, CHANGELOG,
  build-summary, validation-trend on closeout
- ADR-0006 defines 3 device profiles: quad studio, laptop stereo downmix, headphone stereo
- ADR-0010 governs artifact retention: promoted summaries tracked, generated/heavy local-only

TASK:
1. Create Documentation/runbooks/release-checklist-template.md with:
   - Metadata header (Title, Document Type, Author, dates)
   - Purpose section explaining this is a per-release validation template
   - Ordered gate table with 10 gates (listed above)
   - Each gate row: ID, Description, Command/Steps, Pass Criteria, Evidence Path, N/A Policy
   - Fill section for: release version, date, operator, notes
   - Closeout section: all gates must be PASS or explicit N/A with justification

CONSTRAINTS:
- Template format must be easy to copy for each release
- Every gate must have explicit pass/fail criteria (no "looks good")
- N/A must require written justification (not just a checkbox)

EVIDENCE:
- The template file itself: Documentation/runbooks/release-checklist-template.md
```

### Slice B — Skill-Aware Prompt

```
/plan BL-030 Slice B: Device rerun matrix
Load: $skill_docs, $skill_test, $spatial-audio-engineering

Objective: Create Documentation/runbooks/device-rerun-matrix.md defining DEV-01..DEV-06
device validation checks.

Matrix rows:
- DEV-01: Quad studio (4-speaker reference) — verify spatial accuracy, speaker panning
- DEV-02: Laptop stereo downmix — verify downmix preserves spatial intent
- DEV-03: Headphone stereo (generic HRTF) — verify binaural rendering
- DEV-04: Headphone stereo (Steam Audio) — verify Steam HRTF path
- DEV-05: Built-in mic calibration — verify calibration routing for laptop mic
- DEV-06: External mic calibration — verify calibration routing for USB/interface mic

Each check: ID, device profile, test description, commands, pass criteria, evidence path.

Evidence:
- Documentation/runbooks/device-rerun-matrix.md
```

### Slice C — Skill-Aware Prompt

```
/impl BL-030 Slice C: CI release-gate integration
Load: $skill_impl, $skill_docs

Objective: Add CI job that runs automated release-gate checks on tag push (or manual trigger).

Automated gates: build, ctest, production self-test, pluginval, CLAP validator, docs freshness.
Manual gates: device rerun matrix (flagged as manual-required in CI output).

Evidence:
- CI workflow file
- TestEvidence/bl030_release_governance_<timestamp>/ci_integration.log
```

### Slice D — Skill-Aware Prompt

```
/test BL-030 Slice D: First release governance execution
Load: $skill_test, $skill_ship

Objective: Execute the release checklist for current codebase state as a dry run.
Capture baseline evidence for all gates. Document any failures or N/A items.

Evidence:
- TestEvidence/bl030_release_governance_<timestamp>/release_checklist_run.md
- TestEvidence/bl030_release_governance_<timestamp>/device_matrix_results.tsv
- TestEvidence/bl030_release_governance_<timestamp>/status.tsv
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-030-template | Manual | Review checklist template | All 10 gates defined with criteria |
| BL-030-matrix | Manual | Review device matrix | DEV-01..DEV-06 defined |
| BL-030-ci | Automated | CI pipeline run on tag | Automated gates pass |
| BL-030-dryrun | Mixed | Full checklist execution | All gates PASS or explicit N/A |
| BL-030-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Execution Snapshot (2026-02-24)

- Slice A complete:
  - `Documentation/runbooks/release-checklist-template.md`
- Slice B complete:
  - `Documentation/runbooks/device-rerun-matrix.md`
  - `TestEvidence/bl030_slice_b_20260224T203052Z/status.tsv`
- Slice C complete:
  - `.github/workflows/release-governance.yml`
  - `TestEvidence/bl030_slice_c_20260224T203848Z/status.tsv`
  - `TestEvidence/bl030_slice_c_20260224T203848Z/ci_integration.log`
- Slice D complete (dry-run baseline executed):
  - `TestEvidence/bl030_release_governance_20260224T204022Z/release_checklist_run.md`
  - `TestEvidence/bl030_release_governance_20260224T204022Z/device_matrix_results.tsv`
  - `TestEvidence/bl030_release_governance_20260224T204022Z/status.tsv`
  - Release decision from dry-run: `BLOCKED` (blocking gates `RL01`, `RL05`, `RL09`).

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Manual device checks hard to automate | Med | High | Accept manual for DEV-01..06, automate the rest |
| Device hardware unavailable on CI | Med | High | CI runs automated gates only, flag manual as required |
| Checklist too rigid for edge cases | Low | Med | N/A policy with justification covers exceptions |

## Failure & Rollback Paths

- If automated gate fails: fix the issue before release, do not override gate
- If device check fails: document failure, create BL item for fix, block release until resolved
- If CI integration fails: fall back to local script execution, fix CI separately

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Checklist template | `Documentation/runbooks/release-checklist-template.md` | all 10 gates |
| Device matrix | `Documentation/runbooks/device-rerun-matrix.md` | DEV-01..06 |
| Dry run results | `TestEvidence/bl030_release_governance_<timestamp>/release_checklist_run.md` | gate_id, result, evidence_path |
| Device results | `TestEvidence/bl030_release_governance_<timestamp>/device_matrix_results.tsv` | dev_id, result, notes |
| Status TSV | `TestEvidence/bl030_release_governance_<timestamp>/status.tsv` | lane, result, timestamp |

## Slice E Release Unblock Packet (2026-02-25)

- Packet directory: `TestEvidence/bl030_unblock_slice_e_20260225T170350Z`
- Worker command lane results:
  - `./scripts/standalone-ui-selftest-production-p0-mac.sh`: `FAIL` (exit `1`, app `ABRT` before result emit)
  - `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`: `FAIL` (exit `1`, bootstrap/render `ABRT`)
  - `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3`: `FAIL` (exit `134`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Owner replay directory: `TestEvidence/owner_bl030_unblock_replay_20260225T170650Z`
- Owner replay command lane results (authoritative on current branch):
  - `./scripts/standalone-ui-selftest-production-p0-mac.sh`: `PASS`
  - `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`: `PASS`
  - `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 build_local/LocusQ_artefacts/Release/VST3/LocusQ.vst3`: `PASS`
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Current RL blocker replay:
  - `RL-01`: `PASS` (`BL-013` promoted to `Done`)
  - `RL-05`: `FAIL` (fresh passing DEV-01..DEV-06 rerun matrix still required)
  - `RL-09`: `FAIL` (release-note closeout wording still not finalized for unblock promotion)
- Owner disposition: worker Slice E fail was non-authoritative (runtime flake/stale branch behavior); BL-030 remains **In Validation** pending `RL-05` + `RL-09`.

## Slice F RL-05/RL-09 Decision Packet (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl05_rl09_slice_f_20260225T174918Z`
- Fresh command-lane replay outcomes:
  - `RL-03` (`./scripts/standalone-ui-selftest-production-p0-mac.sh`): `FAIL` (exit `1`, `app_exited_before_result`, `ABRT`)
  - `RL-04` (`./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap`): `FAIL` (exit `1`, bootstrap/render `ABRT`)
  - `RL-06` (`pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 30000 ...`): `FAIL` (exit `134`, no validator output captured)
  - `RL-08` (`./scripts/validate-docs-freshness.sh`): `PASS` (exit `0`, zero freshness warnings on final Slice F replay)
- Deterministic vs transient classification:
  - Deterministic blockers: `RL-05` (fresh DEV-01..DEV-06 rerun packet still missing), `RL-09` (release-note closeout wording/evidence not finalized).
  - Transient flakes: `RL-03`, `RL-04`, `RL-06` (abort-path runtime failures; prior owner replay on current branch recorded PASS for same commands).
- Decision: `NO-GO` until `RL-05` + `RL-09` are closed; then rerun runtime lanes to replace transient abort outcomes with fresh PASS artifacts.
- Required unblock steps:
  1. Produce fresh DEV-01..DEV-06 evidence packet to satisfy `RL-05`.
  2. Finalize `CHANGELOG.md` release-note closeout with evidence links for `RL-09`.
  3. Replay `RL-03`, `RL-04`, and `RL-06` in stable host/AppKit session and archive passing logs.

## Slice G2 RL-09 Closeout Packet (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl09_closeout_g2_20260225T180904Z`
- RL-09 verification criteria (authoritative):
  1. `CHANGELOG.md` includes an explicit BL-030 release-governance closeout note for the current window.
  2. The closeout note references canonical BL-030 evidence paths for both RL-09 traceability and remaining RL-05 state.
  3. `rg -n "RL-09|release note|release governance" CHANGELOG.md Documentation/backlog/bl-030-release-governance.md Documentation/testing/bl-030-release-governance-qa.md` returns expected traceability lines.
- RL-09 result in Slice G2 packet: `PASS` (release-note closeout entry present and linked).
- RL-05 result in Slice G2 packet: `FAIL` (fresh matrix replay remains blocked per `TestEvidence/bl030_rl05_clean_replay_g1_20260225T175856Z/dev_matrix_results.tsv`).
- Overall release decision remains `NO-GO` until RL-05 passes with fresh DEV-01..DEV-06 evidence.

## Slice G3 RL-05 Device Matrix Capture Harness (2026-02-25)

- Harness script: `scripts/qa-bl030-device-matrix-capture-mac.sh`
- Purpose: deterministic DEV-01..DEV-06 capture with machine-readable blocker classification and strict RL-05 gate exit semantics.
- Harness outputs:
  - `status.tsv`
  - `dev_matrix_results.tsv`
  - `blocker_taxonomy.tsv`
  - `harness_contract.md`
- Fixed blocker taxonomy categories:
  - `deterministic_missing_manual_evidence`
  - `runtime_flake_abrt`
  - `not_applicable_with_waiver`
- Exit semantics:
  - exit `0` only when `DEV-01..DEV-05` are `PASS` and `DEV-06` is `PASS` or allowed `N/A` with waiver.
  - exit `1` otherwise.
  - exit `2` for usage/invocation errors.
- RL-09 state remains `PASS` from Slice G2; this slice does not alter RL-09 closeout determination.

## Slice G4 RL-05 Device Matrix Closure Replay (2026-02-25)

- Replay packet directory: `TestEvidence/bl030_rl05_replay_g4_20260225T205724Z`
- Validation replay commands:
  - `bash -n scripts/qa-bl030-device-matrix-capture-mac.sh`: `PASS` (`exit 0`)
  - `./scripts/qa-bl030-device-matrix-capture-mac.sh --help`: `PASS` (`exit 0`)
  - `./scripts/qa-bl030-device-matrix-capture-mac.sh --out-dir TestEvidence/bl030_rl05_replay_g4_20260225T205724Z`: `FAIL` (`exit 1`, RL-05 criteria not met)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`exit 0`)
- Replay decision (`release_decision.md`): `NO-GO`.
- RL-05 result: `FAIL` with deterministic blockers still present.
  - Deterministic blockers: `deterministic_missing_manual_evidence` on `DEV-01..DEV-06`.
  - Runtime flakes: `runtime_flake_abrt` on `DEV-01`, `DEV-02`, `DEV-04`, `DEV-05`, `DEV-06`.
- Exact missing actions to close RL-05:
  1. Provide the six required manual evidence notes in the replay packet (`dev01_*` through `dev06_*` manual notes files).
  2. Re-run failing DEV automation lanes in a stable host/AppKit session and capture green logs for DEV-01..DEV-06.
  3. If external mic hardware is unavailable, apply an explicit `DEV-06` waiver via `--dev06-waiver <path>` to classify as `not_applicable_with_waiver`.
- RL-09 state remains `PASS` from Slice G2 and is unchanged by this G4 replay.

## Slice G5 RL-05 Manual Evidence Closure Packet (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z`
- Validation commands:
  - `./scripts/qa-bl030-device-matrix-capture-mac.sh --out-dir TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z`: `FAIL` (`exit 1`, RL-05 criteria not met)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`exit 0`)
  - `rg -n "RL-05|RL-09|DEV-0[1-6]" Documentation/backlog/bl-030-release-governance.md Documentation/testing/bl-030-release-governance-qa.md`: `PASS`
- Outcome matrix: `DEV-01..DEV-06 = FAIL` (see `dev_matrix_results.tsv`).
- Manual evidence checklist: `manual_evidence_checklist.tsv` indicates all six required manual notes are currently missing.
- Manual evidence checklist (G5 replay):

| DEV | Manual Note Path | Present | Status |
|---|---|---|---|
| DEV-01 | `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/dev01_quad_manual_notes.md` | no | missing |
| DEV-02 | `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/dev02_laptop_manual_notes.md` | no | missing |
| DEV-03 | `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/dev03_headphone_generic_manual_notes.md` | no | missing |
| DEV-04 | `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/dev04_steam_manual_notes.md` | no | missing |
| DEV-05 | `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/dev05_builtin_mic_manual_notes.md` | no | missing |
| DEV-06 | `TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/dev06_external_mic_manual_notes.md` | no | missing |
- Blocker taxonomy split:
  - `deterministic_missing_manual_evidence`: `6`
  - `runtime_flake_abrt`: `5`
  - `not_applicable_with_waiver`: `0`
- Decision (`release_decision.md`): `NO-GO`.
- Exact next actions:
  1. Add all six manual evidence note files listed in `manual_evidence_checklist.tsv`.
  2. Re-run failing automation lanes for DEV-01..DEV-06 and capture green logs.
  3. If DEV-06 hardware is unavailable, apply explicit waiver (`--dev06-waiver <path>`) and rerun.
- RL-09 state remains explicitly `PASS` and unchanged by this G5 packet.

## Slice G6 RL-05 Manual Evidence Intake Validation (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl05_manual_intake_g6_20260225T211311Z`
- Validation commands:
  - `bash -n scripts/qa-bl030-device-matrix-capture-mac.sh`: `PASS` (`exit 0`)
  - `./scripts/qa-bl030-device-matrix-capture-mac.sh --help`: `PASS` (`exit 0`)
  - `./scripts/qa-bl030-device-matrix-capture-mac.sh --out-dir TestEvidence/bl030_rl05_manual_intake_g6_20260225T211311Z/run_real --manual-evidence-tsv TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/manual_evidence_checklist.tsv`: `FAIL` (`exit 1`)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`exit 0`)
- Intake schema gate result:
  - `header_schema`: `FAIL` (`missing_required_columns` in manual evidence checklist input)
  - `manual_evidence_gate`: `FAIL` (`rl05_manual_evidence_incomplete_or_invalid`)
- Deterministic blocker summary:
  1. Manual evidence intake TSV schema invalid (`manual_evidence_checklist.tsv`).
  2. DEV-01..DEV-06 manual evidence rows remain missing.
- RL-05 state: `FAIL` (unchanged).
- RL-09 state: `PASS` (unchanged, retained from G2 closeout packet).

## Slice G7 RL-05 Manual Evidence Authoring Pack (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl05_manual_notes_g7_20260225T212529Z`
- Scope completed:
  - DEV-01..DEV-06 manual note templates authored.
  - Operator completion procedure authored.
  - BL-030 QA doc updated with manual intake instructions.
- Validation commands:
  - `./scripts/qa-bl030-manual-evidence-validate-mac.sh --input TestEvidence/bl030_rl05_manual_closure_g5_20260225T210303Z/manual_evidence_checklist.tsv --out-dir TestEvidence/bl030_rl05_manual_notes_g7_20260225T212529Z/validate_real`: `PASS` (expected gate fail path observed, exit `1`)
  - `./scripts/validate-docs-freshness.sh`: `PASS` (`exit 0`)
- Result interpretation:
  - G7 provides deterministic authoring templates and operator workflow.
  - RL-05 gate state remains `FAIL` until filled manual evidence rows are produced and validated.

## Slice G8 RL-03/04/06 Runtime Abort Replay Matrix (2026-02-25)

- Packet directory: `TestEvidence/bl030_runtime_replay_g8_20260225T212654Z`
- Replay set:
  - RL-03: `./scripts/standalone-ui-selftest-production-p0-mac.sh` x5 -> `0/5` pass.
  - RL-04: `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap` x5 -> `0/5` pass.
  - RL-06: `pluginval ... LocusQ.vst3` x5 -> `0/5` pass.
- Failure taxonomy summary:
  - RL-03: `app_exited_before_result`, `signal_ABRT`.
  - RL-04: `bootstrap_failed_134`, `abrt`.
  - RL-06: `pluginval_exit_134`.
- Decision: runtime stability lanes RL-03/RL-04/RL-06 remain `NO-GO` in this replay window.

## Slice H1 RL-03 Selftest Stability Hardening (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl03_stability_h1_20260225T213459Z`
- Updated lane surfaces:
  - `scripts/standalone-ui-selftest-production-p0-mac.sh`
  - `Documentation/testing/selftest-stability-contract.md`
- Validation commands:
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `PASS` (`10/10`)
  - `LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x5: `FAIL` (`4/5`; run `5` terminal reason `selftest_payload_not_ok`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Result interpretation:
  - RL-03 scoped hardening improved BL-029 stability but did not produce a clean BL-009 scoped set.
  - RL-03 remains non-green in this replay window because required scoped matrix is incomplete.

## Slice H3 RL-06 Pluginval Reliability Harness (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl06_pluginval_h3_20260225T213706Z`
- New harness:
  - `scripts/qa-bl030-pluginval-stability-mac.sh`
  - Contract doc: `Documentation/testing/pluginval-stability-contract.md`
- Validation commands:
  - `bash -n scripts/qa-bl030-pluginval-stability-mac.sh`: `PASS`
  - `./scripts/qa-bl030-pluginval-stability-mac.sh --help`: `PASS`
  - `./scripts/qa-bl030-pluginval-stability-mac.sh --runs 5 --out-dir TestEvidence/bl030_rl06_pluginval_h3_20260225T213706Z`: `PASS` (harness execution), strict gate verdict `FAIL` (`0/5` pass, exit `1`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Result interpretation:
  - RL-06 now has deterministic replay tooling and taxonomy evidence.
  - RL-06 runtime lane remains red until pluginval replay achieves stable pass runs.

## Slice H2 RL-04 REAPER Smoke Stabilization (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl04_reaper_stability_h2_20260225T213823Z`
- Updated lane surfaces:
  - `scripts/reaper-headless-render-smoke-mac.sh`
  - `Documentation/testing/bl-030-release-governance-qa.md`
- Validation commands:
  - `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap` x5: `FAIL` (`5/5` fail)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Deterministic taxonomy:
  - `BL030-RL04-001`
  - `stage=bootstrap`
  - `reason=bootstrap_command_failed`
  - `count=5`
- Result interpretation:
  - RL-04 remains a hard-failing runtime lane in this replay window (`Abort trap`, exit `134`).
  - H2 improves stage classification and repeatable diagnostics but does not clear the release gate.

## Slice I1 RL-05 Manual Evidence Packet Compiler (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl05_manual_pack_i1_20260225T214357Z`
- New compiler harness:
  - `scripts/qa-bl030-manual-evidence-pack-mac.sh`
- Validation commands:
  - `bash -n scripts/qa-bl030-manual-evidence-pack-mac.sh`: `PASS`
  - `./scripts/qa-bl030-manual-evidence-pack-mac.sh --help`: `PASS`
  - complete fixture compile: `PASS` (`exit 0`)
  - missing-field fixture compile: `PASS` (`expected exit 1`)
  - compiled TSV through G6 validator: `PASS` (`exit 0`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Result interpretation:
  - RL-05 manual evidence schema compilation and validation tooling is now deterministic.
  - RL-05 release gate remains `FAIL` until real DEV-01..DEV-06 operator evidence is produced (not fixture data).

## Slice I2 RL-04 Bootstrap ABRT Diagnostics (2026-02-25)

- Packet directory: `TestEvidence/bl030_rl04_abrt_diag_i2_20260225T214404Z`
- Diagnostic harness:
  - `scripts/diagnose-reaper-bootstrap-abrt-mac.sh`
- Validation commands:
  - `bash -n scripts/diagnose-reaper-bootstrap-abrt-mac.sh`: `PASS`
  - `./scripts/diagnose-reaper-bootstrap-abrt-mac.sh --help`: `PASS`
  - `./scripts/diagnose-reaper-bootstrap-abrt-mac.sh --runs 10 --out-dir TestEvidence/bl030_rl04_abrt_diag_i2_20260225T214404Z`: `PASS` (diagnostics run complete; `10/10` failures captured)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Taxonomy summary:
  - `stage=bootstrap`: `10`
  - `terminal_reason=bootstrap command failed (exitCode=134)|abrt`: `10`
  - crash reports present: `10/10`
- Result interpretation:
  - RL-04 is reproducibly failing at bootstrap with deterministic ABRT signature in this replay window.

## Slice I3 Release Gate Consolidation Packet (2026-02-25)

- Packet directory: `TestEvidence/bl030_gate_consolidation_i3_20260225T214847Z`
- Validation command:
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Packet outputs:
  - `rl_gate_matrix.tsv`
  - `blocker_taxonomy.tsv`
  - `release_decision.md`
  - `unblock_checklist.md`
- Consolidated decision:
  - `NO-GO`
- failing gates remain `RL-03`, `RL-04`, `RL-05`, `RL-06`
  - packet generated cleanly and is owner-ready for next unblock tranche execution.

## Slice J1 RL-03 Selftest Payload Determinism Remediation (2026-02-25)

- Worker packet directory: `TestEvidence/bl030_rl03_payload_j1_20260225T222049Z`
- Worker validation summary:
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `FAIL` (`10/10`, `app_exited_before_result`)
  - `LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `FAIL` (`10/10`, `app_exited_before_result`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Worker packet conclusion:
  - process-level aborts prevent payload-level proof in that worker environment.
  - payload-specific failure class (`selftest_payload_not_ok`) remains `0/10` in both scopes for this packet.

## Owner Recheck for J1 (2026-02-25)

- Owner replay packet directory: `TestEvidence/owner_bl030_j1_recheck_20260225T223738Z`
- Owner replay commands and outcomes:
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x3: `PASS` (`3/3`)
  - `LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x3: `FAIL` (`3/3`, `selftest_payload_not_ok`, failing check `UI-P1-025E`)
- Owner interpretation (authoritative):
  - J1 worker `app_exited_before_result` signature is not reproduced on owner replay.
  - RL-03 remains red because BL-009 scoped runs fail deterministically on payload assertion `UI-P1-025E` rather than process abort.
  - release decision remains `NO-GO` with unchanged failing gates `RL-03`, `RL-04`, `RL-05`, `RL-06`.

## Slice J2 RL-03 Payload Determinism Remediation (2026-02-25)

- Worker packet directory: `TestEvidence/bl030_rl03_payload_j2_20260225T224646Z`
- Worker validation summary:
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `FAIL` (`10/10`, `app_exited_before_result`)
  - `LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `FAIL` (`10/10`, `app_exited_before_result`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
- Worker packet conclusion:
  - payload taxonomy enhancements are present, but process-level abort prevents lane pass in worker environment.
  - payload-specific failure class (`selftest_payload_not_ok`) remains `0/10` in both scopes for this worker packet.

## Owner Recheck for J2 (2026-02-25)

- Owner replay packet directory: `TestEvidence/owner_bl030_j2_recheck_20260225T225134Z`
- Owner replay commands and outcomes:
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x3: `PASS` (`3/3`)
  - `LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x3: `FAIL` (`3/3`, `selftest_payload_not_ok`, failing check `UI-P1-025E`)
- Owner interpretation (authoritative):
  - J2 worker `app_exited_before_result` signature is not reproduced on owner replay.
  - RL-03 classification is unchanged from J1: deterministic BL-009 payload assertion failure (`UI-P1-025E`) while BL-029 lane is green.
  - release decision remains `NO-GO` with unchanged failing gates `RL-03`, `RL-04`, `RL-05`, `RL-06`.

## Slice K1 RL-03 Selftest Stability Hardening (2026-02-26)

- Worker packet directory: `TestEvidence/bl030_rl03_stability_k1_20260226T043756Z`
- Updated lane surfaces:
  - `scripts/standalone-ui-selftest-production-p0-mac.sh`
  - `Documentation/testing/selftest-stability-contract.md`
- Validation commands:
  - `LOCUSQ_UI_SELFTEST_SCOPE=bl029 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `FAIL` (`9/10`; one `app_exited_before_result`, BUS signal)
  - `LOCUSQ_UI_SELFTEST_BL009=1 ./scripts/standalone-ui-selftest-production-p0-mac.sh` x10: `PASS` (`10/10`)
  - `./scripts/validate-docs-freshness.sh`: `PASS`
  - `bash -n scripts/standalone-ui-selftest-production-p0-mac.sh`: `PASS`
- Result interpretation:
  - K1 materially improved RL-03 stability compared to J1/J2 (BL-009 scoped runs now fully green).
  - RL-03 remains red because strict all-runs-pass criteria is not met (`BL-029` scope still has a residual `1/10` process-exit flake).

## Closeout Checklist

- [x] Release checklist template committed and reviewed
- [x] Device rerun matrix defined (DEV-01..DEV-06)
- [x] CI release-gate job functional
- [x] First dry run executed with evidence
- [ ] All gates PASS or documented N/A
- [x] Evidence captured at designated paths (including Slice E unblock packet)
- [x] status.json updated
- [x] Documentation/backlog/index.md row updated
- [x] TestEvidence surfaces updated
- [x] ./scripts/validate-docs-freshness.sh passes
