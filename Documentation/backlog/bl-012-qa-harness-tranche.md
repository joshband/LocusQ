---
Title: BL-012 QA Harness Tranche Closeout
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# BL-012: QA Harness Tranche Closeout

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | In Validation |
| Owner Track | Track D — QA Platform |
| Depends On | — |
| Blocks | BL-013 |
| Annex Spec | (inline — no separate annex) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Harness rerun + HX-04 parity embed |

## Objective

Complete QA harness tranche closeout with embedded HX-04 scenario parity guard, producing deterministic evidence that the harness test suite remains green and scenario coverage has not drifted.

## Scope & Non-Scope

**In scope:**
- Full ctest harness rerun (45+ tests)
- HX-04 parity audit embedded in each rerun
- Evidence capture and Tier 0 surface sync

**Out of scope:**
- Adding new harness test cases (that's future work)
- HostRunner promotion (that's BL-013)
- Modifying harness test code

## Architecture Context

- The QA harness is ctest-based with a custom runner app under `tests/`
- HX-04 parity guard audits AirAbsorption, CalibrationEngine, and directivity paths
- Scenario audit evidence: `Documentation/testing/hx-04-scenario-coverage-audit-2026-02-23.md`
- Invariants: `Documentation/invariants.md` — State/Traceability (new assertions must be logged)

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Rerun full harness + HX-04 parity | `tests/`, `TestEvidence/` | None | ctest 45/45 pass, parity audit green |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/test BL-012: QA harness tranche closeout with HX-04 parity guard
Load: $skill_test, $skill_testing, $skill_troubleshooting

Objective: Rerun the full QA harness suite (ctest) and embed HX-04 scenario parity audit.
Capture deterministic evidence of all-pass status.

Constraints:
- Do not modify harness test source code
- HX-04 parity check must remain green in every rerun
- Preserve existing scenario coverage — do not remove or skip tests

Validation:
- cmake --build build --target all
- ctest --test-dir build --output-on-failure
- Expected: 45/45 tests pass

Evidence:
- Capture ctest output to TestEvidence/bl012_harness_tranche_<timestamp>/ctest.log
- Capture HX-04 parity status to TestEvidence/bl012_harness_tranche_<timestamp>/hx04_parity.tsv
- Update TestEvidence/validation-trend.md with result row
- Update TestEvidence/build-summary.md snapshot
```

### Slice A — Standalone Fallback Prompt

```
You are validating BL-012 for LocusQ, a JUCE-based spatial audio plugin.

PROJECT CONTEXT:
- Repository: LocusQ at /Users/artbox/Documents/Repos/LocusQ
- QA harness: ctest-based test suite with 45+ tests under tests/ directory
- Build system: CMake, build directory at build/
- HX-04: Scenario coverage parity audit that checks AirAbsorption, CalibrationEngine,
  and directivity path scenarios remain covered. Reference:
  Documentation/testing/hx-04-scenario-coverage-audit-2026-02-23.md

TASK:
1. Build the project: cmake --build build --target all
2. Run the full harness: ctest --test-dir build --output-on-failure
3. Verify 45/45 tests pass (or document any failures)
4. Run HX-04 parity check if a separate script exists, or verify scenario coverage
   from the test output
5. Capture evidence:
   - ctest log -> TestEvidence/bl012_harness_tranche_<timestamp>/ctest.log
   - Create status.tsv with columns: lane, result, timestamp
6. Append a row to TestEvidence/validation-trend.md:
   | <date> | BL-012 harness tranche | <PASS/FAIL> | <notes> |
7. Update TestEvidence/build-summary.md with latest build snapshot

CONSTRAINTS:
- Do not modify test source code
- Do not skip or disable failing tests — document them instead
- HX-04 parity must remain green

EVIDENCE:
- TestEvidence/bl012_harness_tranche_<timestamp>/ctest.log
- TestEvidence/bl012_harness_tranche_<timestamp>/status.tsv
- TestEvidence/validation-trend.md (appended row)
- TestEvidence/build-summary.md (updated snapshot)
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-012-harness | Automated | `ctest --test-dir build --output-on-failure` | 45/45 pass |
| BL-012-hx04 | Automated | HX-04 parity audit | No coverage drift |
| BL-012-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Environment drift causes harness failures | Med | Med | Pin CMake/dependency versions, check build/CMakeCache.txt |
| HX-04 scenario coverage drifts silently | High | Low | Embed parity check in every rerun |

## Failure & Rollback Paths

- If ctest fails: run failing test individually with `ctest --test-dir build -R <test_name> -V`, check `.codex/troubleshooting/known-issues.yaml`
- If HX-04 parity drifts: compare current coverage matrix against `Documentation/testing/hx-04-scenario-coverage-audit-2026-02-23.md`
- If build fails: check recent commits for breaking changes, rebuild from clean

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| ctest log | `TestEvidence/bl012_harness_tranche_<timestamp>/ctest.log` | full ctest output |
| Status TSV | `TestEvidence/bl012_harness_tranche_<timestamp>/status.tsv` | lane, result, timestamp |
| Validation trend | `TestEvidence/validation-trend.md` | date, lane, result, notes |
| Build summary | `TestEvidence/build-summary.md` | date, build_type, result |

## Closeout Checklist

- [ ] Full harness rerun passes (45/45)
- [ ] HX-04 parity audit green
- [ ] Evidence captured at designated paths
- [ ] status.json updated
- [ ] Documentation/backlog/index.md row updated
- [ ] TestEvidence surfaces updated
- [ ] ./scripts/validate-docs-freshness.sh passes
