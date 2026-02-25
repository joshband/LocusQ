Title: BL-033 Headphone Calibration Core Path
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-033: Headphone Calibration Core Path

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | In Planning |
| Owner Track | Track A — Runtime Formats |
| Depends On | BL-009, BL-017, BL-026, BL-028 |
| Blocks | BL-034 |
| Annex Spec | `Documentation/plans/bl-033-headphone-calibration-core-spec-2026-02-25.md` |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Headphone profile/state contract and migration wiring (`hp_*` parameters + blob refs) |
| B | High | L | Steam virtual surround monitoring path integration with requested/active diagnostics |
| C | High | L | PEQ/FIR post-chain integration with deterministic engine selection and latency reporting |
| D | Med | M | QA lane and RT safety validation contract closeout for headphone monitoring path |

## Objective

Implement an RT-safe, deterministic internal headphone calibration monitoring path for LocusQ using Steam binaural rendering plus optional EQ/FIR compensation, while preserving existing monitoring modes and publishing explicit diagnostics (`requested`, `active`, `stage`, fallback reason) for CALIBRATE and scene-state consumers.

## Scope & Non-Scope

**In scope:**
- Add/solidify headphone calibration contract for APVTS + state payload (`hp_*` family, SOFA/FIR references)
- Integrate `steam_binaural` chain ordering and fallback semantics
- Integrate PEQ/FIR post-binaural compensation stages with explicit latency handling
- Publish diagnostics parity across requested vs active monitoring path/profile
- Add deterministic QA and RT-safety evidence requirements

**Out of scope:**
- Full personalized HRTF generation pipeline
- New UI redesign beyond necessary diagnostics/controls binding
- Replacing internal renderer architecture with full ambisonic-first rewrite
- Device-specific OS binaural stack integration inside plugin process

## Architecture Context

- Research origin:
  - `Documentation/research/LocusQ Headphone Calibration Research Outline.md`
  - `Documentation/research/Headphone Calibration for 3D Audio.pdf`
- Invariants: `Documentation/invariants.md` (RT safety, deterministic scene-state publication)
- Device/monitoring contract: `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
- Scene diagnostics contract: `Documentation/scene-state-contract.md`
- Existing headphone baseline: BL-009 closeout in `Documentation/backlog/done/bl-009-steam-headphone-contract.md`

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Headphone profile/state contract and migration | `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, state migration docs/contracts | BL-009/BL-026 complete | deterministic `hp_*` contract landed with migration + idempotence checks |
| B | Steam virtual surround monitoring path integration | `Source/SpatialRenderer.h`, `Source/PluginProcessor.cpp`, diagnostics contracts | Slice A complete | `steam_binaural` path active with explicit requested/active/fallback diagnostics |
| C | PEQ/FIR post-chain + latency contract | DSP modules + `Source/PluginProcessor.cpp` + QA assertions | Slice B complete | PEQ/FIR chain deterministic, no RT safety regressions, latency reporting verified |
| D | QA lane + evidence hardening | `qa/scenarios/*`, `scripts/qa-*.sh`, docs/evidence contracts | Slice C complete | lane replay pass with machine-readable evidence + docs freshness pass |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl BL-033 Slice A: Headphone profile/state contract and migration
Load: $skill_impl, $steam-audio-capi, $spatial-audio-engineering, $skill_docs

Objective:
- Add deterministic headphone calibration parameter/state contract (`hp_*`) with migration safety.

Constraints:
- No allocations/locks/blocking in processBlock.
- Preserve backward compatibility for existing state payloads.
- Publish additive diagnostics only.

Validation:
- cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8
- ./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json
- ./scripts/rt-safety-audit.sh --print-summary --output TestEvidence/bl033_slice_a_<timestamp>/rt_audit.tsv
- ./scripts/validate-docs-freshness.sh

Evidence:
- TestEvidence/bl033_slice_a_<timestamp>/
```

### Slice A — Standalone Fallback Prompt

```
You are implementing BL-033 Slice A in LocusQ.

CONTEXT:
- JUCE plugin with CALIBRATE/EMITTER/RENDERER modes.
- Existing headphone baseline from BL-009.
- New contract must be additive and migration-safe.

TASK:
1) Add headphone calibration parameter/state contract (`hp_*`) for monitoring profile, HRTF mode, EQ mode, FIR metadata.
2) Add migration path for existing state snapshots.
3) Publish requested/active diagnostics fields without breaking existing scene-state consumers.

CONSTRAINTS:
- RT-safe processBlock only (no allocations/locks/I/O).
- Maintain deterministic serialization order.

VALIDATION:
- build + smoke suite + RT safety audit + docs freshness.

EVIDENCE:
- TestEvidence/bl033_slice_a_<timestamp>/status.tsv and diagnostic snapshot.
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-033-build | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone locusq_qa -j 8` | Exit 0 |
| BL-033-smoke | Automated | `./build_local/locusq_qa_artefacts/Release/locusq_qa --spatial qa/scenarios/locusq_smoke_suite.json` | Suite pass |
| BL-033-hp-contract | Automated | `./scripts/qa-bl009-headphone-contract-mac.sh` | Exit 0; no contract regressions |
| BL-033-rt | Automated | `./scripts/rt-safety-audit.sh --print-summary --output <out>/rt_audit.tsv` | `non_allowlisted=0` |
| BL-033-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Monitoring mode routing regressions | High | Med | Preserve explicit requested/active diagnostics and add per-mode assertions |
| RT regressions from FIR/HRTF swaps | High | Med | Enforce non-RT build/swap path with atomic pointer handoff |
| Latency drift across modes | Med | Med | Add explicit latency assertions for bypass/direct/partitioned branches |

## Failure & Rollback Paths

- If headphone contract lane fails: freeze new `hp_*` fields behind fallback defaults and re-run BL-009 lane.
- If RT safety audit fails: isolate offending lines, apply minimal allowlist only for intentional non-RT context.
- If latency mismatch appears: force known-safe bypass profile and keep diagnostics reporting explicit degraded mode.

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Lane status | `TestEvidence/bl033_headphone_core_<timestamp>/status.tsv` | lane, exit, result, notes |
| Diagnostics snapshot | `TestEvidence/bl033_headphone_core_<timestamp>/diagnostics_snapshot.json` | requested, active, stage, fallbackReason |
| RT audit report | `TestEvidence/bl033_headphone_core_<timestamp>/rt_audit.tsv` | non_allowlisted, rule hits |
| Validation trend row | `TestEvidence/validation-trend.md` | date, lane, result, BL ID |

## Closeout Checklist

- [ ] All slices (A-D) implemented and validated
- [ ] Monitoring mode contract (`speakers`, `steam_binaural`, `virtual_binaural`) verified
- [ ] Latency reporting verified for FIR mode transitions
- [ ] RT safety lane is green with explicit evidence
- [ ] `Documentation/backlog/index.md` and runbook status are synchronized
- [ ] `./scripts/validate-docs-freshness.sh` passes

