Title: LocusQ Release Checklist Template
Document Type: Runbook Template
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# LocusQ Release Checklist Template

## Purpose

Repeatable pre-release validation template for LocusQ release candidates.
This checklist enforces BL-030 governance gates and ADR contracts before packaging/distribution.

## Contract References

- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`
- `Documentation/backlog/index.md`
- `status.json`

## Run Metadata (Fill Before Execution)

| Field | Value |
|---|---|
| Release version | `<vX.Y.Z>` |
| Candidate tag/branch | `<tag-or-branch>` |
| Operator | `<name>` |
| Date (UTC) | `<YYYY-MM-DD>` |
| Build host | `<machine-id>` |
| Commit SHA | `<git-sha>` |
| Evidence directory | `TestEvidence/bl030_release_governance_<timestamp>/` |
| Run notes | `<release context / exceptions / links>` |

## Result Policy

1. Allowed gate results: `PASS`, `FAIL`, `N/A`.
2. No implicit skips: every gate must have an explicit result.
3. `N/A` is valid only if the gate row explicitly allows it and a written justification is recorded.
4. Any required gate with `FAIL` blocks release.
5. For device matrix gate, `DEV-06` may be `N/A` only when external mic hardware is unavailable and rationale is logged.

## Gate Table (Ordered, Do Not Reorder)

| Gate ID | Gate | Command / Steps | Pass Criteria | Evidence Path | N/A Policy |
|---|---|---|---|---|---|
| RL-01 | All P1 backlog items are Done | Review `Documentation/backlog/index.md` Active Queue. Confirm no P1 rows remain in non-done states for release scope. | All P1 backlog items required for the release are `Done` with evidence pointers. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl01_p1_backlog_check.md` | Not allowed |
| RL-02 | HX-06 RT audit lane is green | Verify latest HX-06 bundle (`status.tsv`, baseline report) and confirm CI wiring is active. | Latest HX-06 baseline row is `PASS` and CI fail-fast RT audit step is present. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl02_hx06_check.md` | Not allowed |
| RL-03 | Production self-test passes (standalone) | `./scripts/standalone-ui-selftest-production-p0-mac.sh` | Self-test result JSON reports `status=pass` and `ok=true`. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl03_selftest.log` and produced JSON path | Not allowed |
| RL-04 | REAPER headless render smoke passes | `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap` | Status JSON indicates plugin found and render output detected. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl04_reaper_smoke.log` and status JSON path | Not allowed |
| RL-05 | Device rerun matrix passes (DEV-01..DEV-06) | Execute `Documentation/runbooks/device-rerun-matrix.md` rows in order and record each row outcome. | `DEV-01..DEV-05` are `PASS`; `DEV-06` is `PASS` or explicit justified `N/A`. | `TestEvidence/bl030_release_governance_<timestamp>/device_matrix_results.tsv` | Allowed only for rows explicitly marked optional (normally `DEV-06`) with justification |
| RL-06 | pluginval validation passes (VST3, AU) | Run pluginval for release artifacts (strictness level per release policy) for VST3 and AU. | pluginval exits success for both required formats in this release. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl06_pluginval_vst3.log`, `.../gate_rl06_pluginval_au.log` | Allowed only if a format is intentionally not part of this release scope; justification required |
| RL-07 | CLAP validator passes (if CLAP build active) | If CLAP is enabled for this release, run `clap-info` and `clap-validator` on CLAP artifact. | CLAP checks pass when CLAP is in release scope. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl07_clap.log` | Allowed only when CLAP is not in release scope; justification required |
| RL-08 | ADR-0005 freshness gate passes | `./scripts/validate-docs-freshness.sh` | Exit code `0` with no freshness warnings. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl08_docs_freshness.log` | Not allowed |
| RL-09 | `CHANGELOG.md` updated for release | Confirm release notes entry exists for target version/date and includes key lane outcomes. | Target version entry present and references canonical evidence. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl09_changelog_check.md` | Not allowed |
| RL-10 | Build artifacts packaged (VST3/AU/Standalone) | Build release artifacts and verify expected output bundle contents for release scope. | Required artifacts exist and checksums/manifest are recorded. | `TestEvidence/bl030_release_governance_<timestamp>/gate_rl10_packaging.log` and manifest path | Not allowed |

## Execution Log Template

| Gate ID | Result (`PASS`/`FAIL`/`N/A`) | Timestamp (UTC) | Evidence Artifact | N/A Justification (required if `N/A`) | Notes |
|---|---|---|---|---|---|
| RL-01 |  |  |  |  |  |
| RL-02 |  |  |  |  |  |
| RL-03 |  |  |  |  |  |
| RL-04 |  |  |  |  |  |
| RL-05 |  |  |  |  |  |
| RL-06 |  |  |  |  |  |
| RL-07 |  |  |  |  |  |
| RL-08 |  |  |  |  |  |
| RL-09 |  |  |  |  |  |
| RL-10 |  |  |  |  |  |

## Closeout Decision

| Field | Value |
|---|---|
| Release decision | `<READY / BLOCKED>` |
| Blocking gates (if any) | `<gate IDs or none>` |
| Approved by | `<name>` |
| Approval timestamp (UTC) | `<timestamp>` |
| Closeout rule check | `All required gates PASS; any N/A has explicit written justification` |

## Artifact Retention Notes

1. Follow ADR-0010: keep generated/heavy raw outputs local-only unless promoted as decision-grade evidence.
2. Record canonical evidence pointers in:
   - `TestEvidence/build-summary.md`
   - `TestEvidence/validation-trend.md`
   - `status.json` (for status changes)
3. If release-state claims change, apply ADR-0005 closeout bundle synchronization in the same changeset.
