---
Title: BL-013 HostRunner Feasibility Promotion
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-25
---

# BL-013: HostRunner Feasibility Promotion

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Done (2026-02-25) |
| Owner Track | Track D — QA Platform |
| Depends On | BL-012 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-013-hostrunner-feasibility-2026-02-23.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Low | S | Rerun existing feasibility probes |
| B | Med | M | Add CLAP backend probe (if VST3 passes) |

## Objective

Decide whether to promote HostRunner from feasibility status to a sustained validation lane. VST3 backend + skeleton probes currently pass. The open risk is CLAP backend parity — if that probe can be added and passes, HostRunner becomes a permanent CI lane.

## Scope & Non-Scope

**In scope:**
- Rerun VST3 feasibility probes
- Evaluate and add CLAP backend probe
- Promotion decision documentation

**Out of scope:**
- Full host automation suite (that's BL-024 territory)
- New test scenario design
- AU backend probes (future work)

## Architecture Context

- HostRunner is a lightweight host simulator for plugin validation without a full DAW
- VST3 backend passes; root causes previously fixed: null processData_ dereference, missing mock sendMidiEvents
- CLAP backend parity is the single remaining risk
- Annex spec has full feasibility decision and evidence: `Documentation/plans/bl-013-hostrunner-feasibility-2026-02-23.md`
- ADR-0009 (CLAP consolidation) is relevant for CLAP probe alignment

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Rerun VST3 + skeleton probes | `tests/`, `TestEvidence/` | BL-012 done | Probes pass, evidence captured |
| B | Add CLAP backend probe | `tests/`, host runner config | Slice A pass | CLAP probe passes or risk documented |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/test BL-013 Slice A: Rerun HostRunner feasibility probes
Load: $skill_test, $skill_testing, $skill_troubleshooting

Objective: Rerun HostRunner VST3 backend and skeleton fallback probes.
Capture fresh evidence for promotion decision.

Constraints:
- Do not modify HostRunner source code in this slice
- BL-012 must be done before starting

Validation:
- ctest --test-dir build -R HostRunner --output-on-failure
- Expected: all HostRunner probes pass

Evidence:
- TestEvidence/bl013_hostrunner_<timestamp>/vst3_probe.log
- TestEvidence/bl013_hostrunner_<timestamp>/status.tsv
```

### Slice A — Standalone Fallback Prompt

```
You are validating BL-013 Slice A for LocusQ, a JUCE-based spatial audio plugin.

PROJECT CONTEXT:
- HostRunner: lightweight host simulator for plugin validation without DAW
- Previous issues fixed: null processData_ dereference, missing mock sendMidiEvents
- Feasibility annex: Documentation/plans/bl-013-hostrunner-feasibility-2026-02-23.md

TASK:
1. Verify BL-012 is done (check status.json or backlog index)
2. Run HostRunner probes: ctest --test-dir build -R HostRunner --output-on-failure
3. Capture probe output to TestEvidence/bl013_hostrunner_<timestamp>/
4. Document pass/fail status in status.tsv
5. If all pass, recommend promotion to sustained lane

CONSTRAINTS:
- Do not modify HostRunner source in this slice
- Document any failures for Slice B investigation

EVIDENCE:
- TestEvidence/bl013_hostrunner_<timestamp>/vst3_probe.log
- TestEvidence/bl013_hostrunner_<timestamp>/status.tsv
```

### Slice B — Skill-Aware Prompt

```
/impl BL-013 Slice B: Add CLAP backend probe to HostRunner
Load: $skill_test, $clap-plugin-lifecycle, $skill_troubleshooting

Objective: Add a CLAP backend probe to HostRunner alongside the existing VST3 probe.
This determines if HostRunner can validate CLAP builds without a real DAW.

Constraints:
- Follow CLAP adapter patterns from BL-011 closeout
- Reference: Documentation/plans/LocusQClapContract.h
- Probe must follow same lifecycle as VST3 probe (load, process, unload)

Validation:
- ctest --test-dir build -R HostRunner --output-on-failure
- Expected: VST3 + CLAP probes both pass

Evidence:
- TestEvidence/bl013_hostrunner_<timestamp>/clap_probe.log
- Update status.tsv with CLAP probe result
```

### Slice B — Standalone Fallback Prompt

```
You are implementing BL-013 Slice B for LocusQ.

PROJECT CONTEXT:
- HostRunner has a working VST3 probe (load, process, unload lifecycle)
- CLAP adapter is built and validated (BL-011 done, see Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md)
- CLAP contract header: Documentation/plans/LocusQClapContract.h
- ADR-0009 governs CLAP documentation consolidation

TASK:
1. Review existing VST3 probe implementation in HostRunner
2. Add parallel CLAP backend probe following same lifecycle pattern
3. CLAP probe should: load CLAP plugin, call process with test buffer, unload
4. Add probe to ctest registration
5. Run: ctest --test-dir build -R HostRunner --output-on-failure
6. Capture results

CONSTRAINTS:
- CLAP probe must not share state with VST3 probe
- Follow CLAP lifecycle: clap_plugin_create -> activate -> start_processing -> process -> stop -> deactivate -> destroy
- If CLAP probe cannot be implemented (missing infrastructure), document the blocker

EVIDENCE:
- TestEvidence/bl013_hostrunner_<timestamp>/clap_probe.log
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-013-vst3 | Automated | `ctest --test-dir build -R HostRunner` | VST3 probes pass |
| BL-013-clap | Automated | `ctest --test-dir build -R HostRunner` | CLAP probe passes (or risk documented) |
| BL-013-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| CLAP backend infrastructure missing | Med | Med | Document blocker, defer to future slice |
| VST3 probe regression | High | Low | Rerun before CLAP work, isolate changes |

## Failure & Rollback Paths

- If VST3 probes regress: compare against last known-good evidence in annex spec, check for build changes
- If CLAP probe fails: document specific failure, create intake for CLAP HostRunner infrastructure
- If both fail: escalate to BL-012 owner, check environment drift

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| VST3 probe log | `TestEvidence/bl013_hostrunner_<timestamp>/vst3_probe.log` | probe lifecycle output |
| CLAP probe log | `TestEvidence/bl013_hostrunner_<timestamp>/clap_probe.log` | probe lifecycle output |
| Status TSV | `TestEvidence/bl013_hostrunner_<timestamp>/status.tsv` | lane, result, timestamp |
| Promotion decision | `TestEvidence/bl013_hostrunner_<timestamp>/decision.md` | recommendation, evidence summary |

## Slice D Promotion Packet (Worker Verification, 2026-02-25)

Worker bundle: `TestEvidence/bl013_done_promotion_20260225T170341Z/`

| Criterion | Result | Evidence |
|---|---|---|
| Build (`LocusQ_Standalone`, `locusq_qa`) | PASS | `TestEvidence/bl013_done_promotion_20260225T170341Z/build.log`, `TestEvidence/bl013_done_promotion_20260225T170341Z/status.tsv` |
| HostRunner feasibility lane | PASS (exit 0) | `TestEvidence/bl013_done_promotion_20260225T170341Z/hostrunner_lane.log`, `TestEvidence/bl013_done_promotion_20260225T170341Z/hostrunner_status.tsv` |
| RT safety audit | PASS | `TestEvidence/bl013_done_promotion_20260225T170341Z/rt_audit.tsv` |
| Docs freshness | PASS | `TestEvidence/bl013_done_promotion_20260225T170341Z/docs_freshness.log` |
| Decision rule (all required lanes pass in same run) | PASS (`PROMOTE_TO_DONE`) | `TestEvidence/bl013_done_promotion_20260225T170341Z/validation_matrix.tsv`, `TestEvidence/bl013_done_promotion_20260225T170341Z/promotion_decision.md` |

Note: HostRunner internal status includes a non-blocking `warn` row for skipped optional harness-host ctests (`LQ_BL013_RUN_HARNESS_HOST_TESTS=0`). Required promotion lanes above all passed.

## Owner Done Sync (2026-02-25)

- BL-013 is now promoted to Done across owner-authoritative surfaces:
  - `Documentation/backlog/index.md`
  - `status.json`
  - `TestEvidence/build-summary.md`
  - `TestEvidence/validation-trend.md`
  - `README.md`
  - `CHANGELOG.md`

## Closeout Checklist

- [x] VST3 probe rerun passes
- [x] CLAP probe implemented and tested (or risk documented)
- [x] Promotion decision documented
- [x] Evidence captured at designated paths
- [x] status.json updated
- [x] Documentation/backlog/index.md row updated
- [x] TestEvidence surfaces updated
- [x] ./scripts/validate-docs-freshness.sh passes
