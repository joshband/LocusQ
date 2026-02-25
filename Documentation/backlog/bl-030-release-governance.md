Title: BL-030 Release Governance and Device Rerun
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-25

# BL-030: Release Governance and Device Rerun

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Validation (Slice E worker FAIL not reproduced on owner replay; remaining blockers RL-05/RL-09) |
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
