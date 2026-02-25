Title: BL-034 Headphone Calibration Verification and Profile Governance
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-034: Headphone Calibration Verification and Profile Governance

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Planning |
| Owner Track | Track D — QA Platform |
| Depends On | BL-033 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-034-headphone-calibration-verification-spec-2026-02-25.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Device profile catalog and fallback taxonomy contract |
| B | Med | M | Perceptual verification workflow contract and score persistence |
| C | High | L | Deterministic QA lane set + replay hash contract |
| D | Med | M | Release-governance evidence linkage for headphone readiness |

## Objective

Define and enforce a deterministic verification + profile-governance layer for headphone calibration so profile selection, fallback behavior, and perceptual verification outcomes are reproducible, machine-readable, and release-governance ready.

## Scope & Non-Scope

**In scope:**
- Device/profile catalog contract (generic, known profiles, custom SOFA refs)
- Fallback taxonomy and deterministic reason publication when assets are missing/invalid
- Verification metric storage contract (front/back, elevation, externalization confidence)
- Deterministic QA lane definitions and replay evidence schema
- Release checklist integration points for headphone readiness evidence

**Out of scope:**
- New DSP algorithms beyond BL-033 core
- Personalized HRTF generation tooling
- Broad UI redesign beyond verification-state exposure
- Host-specific proprietary head-tracking integrations

## Architecture Context

- Upstream dependency: BL-033 core monitoring path and state contract
- Research origin:
  - `Documentation/research/LocusQ Headphone Calibration Research Outline.md`
  - `Documentation/research/Headphone Calibration for 3D Audio.pdf`
- Device compatibility contract: `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- Release governance reference: `Documentation/runbooks/release-checklist-template.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Profile library + fallback reason taxonomy | profile/state contracts, docs, QA schema | BL-033 at least Slice B complete | profile/fallback matrix deterministic and documented |
| B | Verification metric contract + persistence | processor diagnostics paths + docs | Slice A complete | verification metrics serialize/load without drift |
| C | QA lanes + replay hashes | `qa/scenarios/*`, `scripts/qa-*.sh`, evidence schemas | Slice B complete | deterministic replay and failure taxonomy green |
| D | Release-governance linkage | release docs and evidence pointers | Slice C complete | BL-030-compatible evidence contract published |

## Agent Mega-Prompt

### Slice C — Skill-Aware Prompt

```
/test BL-034 Slice C: deterministic headphone verification lane set
Load: $skill_testing, $skill_test, $skill_docs, $skill_troubleshooting

Objective:
- Implement and validate deterministic QA lane coverage for headphone profile + verification contracts.

Constraints:
- No source-side DSP redesign in this slice.
- Lane outputs must be machine-readable and replayable.
- Failure taxonomy must classify oversights (profile missing, fallback mismatch, score drift).

Validation:
- cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- ./scripts/qa-bl009-headphone-contract-mac.sh
- ./scripts/validate-docs-freshness.sh

Evidence:
- TestEvidence/bl034_headphone_verification_<timestamp>/
```

### Slice C — Standalone Fallback Prompt

```
You are implementing BL-034 Slice C for LocusQ.

TASK:
1) Define deterministic QA scenarios and scripts for headphone profile selection and verification score persistence.
2) Emit machine-readable results with failure taxonomy.
3) Ensure outputs are replayable and hash-stable for the same input set.

CONSTRAINTS:
- Keep schema additive and backward compatible.
- Do not introduce non-deterministic timestamps in hash-critical payloads.

VALIDATION:
- build, smoke suite, headphone contract lane, docs freshness.

EVIDENCE:
- status.tsv, replay_hashes.tsv, failure_taxonomy.tsv, diagnostics_snapshot.json.
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-034-build | Automated | `cmake --build build_local --config Release --target locusq_qa LocusQ_Standalone -j 8` | Exit 0 |
| BL-034-smoke | Automated | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | Suite pass |
| BL-034-hp-contract | Automated | `./scripts/qa-bl009-headphone-contract-mac.sh` | Exit 0 |
| BL-034-selftest | Automated | `LOCUSQ_UI_SELFTEST_SCOPE=bl026 ./scripts/standalone-ui-selftest-production-p0-mac.sh` | Exit 0 |
| BL-034-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Verification metrics become subjective/non-repeatable | High | Med | Define bounded score semantics and deterministic playback seeds |
| Profile fallback reasons are ambiguous | Med | Med | Publish explicit reason codes + expected fallback target |
| QA lanes become flaky across runs | High | Med | Add replay hash and per-lane strict exit semantics |

## Failure & Rollback Paths

- If verification replay hashes drift: freeze lane inputs and rebaseline only with explicit owner sign-off.
- If profile fallback mismatches occur: force conservative fallback profile and mark lane fail with reason code.
- If selftest instability appears: classify by taxonomy and isolate script/runtime serialization before code changes.

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Lane status | `TestEvidence/bl034_headphone_verification_<timestamp>/status.tsv` | lane, exit, result |
| Profile matrix | `TestEvidence/bl034_headphone_verification_<timestamp>/per_profile_results.tsv` | profileId, requested, active, fallbackReason, result |
| Replay hashes | `TestEvidence/bl034_headphone_verification_<timestamp>/replay_hashes.tsv` | scenario, run, hash |
| Failure taxonomy | `TestEvidence/bl034_headphone_verification_<timestamp>/failure_taxonomy.tsv` | class, count, note |

## Closeout Checklist

- [ ] BL-033 dependency conditions satisfied
- [ ] Profile catalog + fallback taxonomy contract landed
- [ ] Verification score persistence contract validated
- [ ] QA lanes deterministic with replay evidence
- [ ] Release-governance linkage evidence documented
- [ ] `Documentation/backlog/index.md` synchronized
- [ ] `./scripts/validate-docs-freshness.sh` passes

