Title: LocusQ Testing Surface Index
Document Type: Testing Index
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# Testing Surface Index

## Purpose

Keep `Documentation/testing/` lean.
This folder holds reusable testing contracts, a small number of active BL-specific QA packets, and short operator-facing guides.
One-off evidence packets move to `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/`.

## Classification Legend

- `canonical reusable guide`: reusable operator flow or triage guide.
- `supporting contract`: stable harness, retry, or failure-taxonomy contract.
- `BL-specific packet to keep active`: active QA packet tied to a live backlog item.
- `archive candidate`: one-off evidence packet or superseded audit note.

## Active Surfaces

| File | Classification | Role |
|---|---|---|
| `README.md` | supporting contract | folder index and classification contract |
| `production-selftest-and-reaper-headless-smoke-guide.md` | canonical reusable guide | explains the two main validation lanes |
| `reaper-manual-qa-session.md` | canonical reusable guide | manual host-session template for REAPER checks |
| `selftest-abrt-triage.md` | canonical reusable guide | deterministic selftest ABRT diagnosis |
| `selftest-stability-contract.md` | supporting contract | selftest output, retry, and lock taxonomy |
| `pluginval-stability-contract.md` | supporting contract | RL-06 pluginval stability and taxonomy contract |
| `bl-030-release-governance-qa.md` | BL-specific packet to keep active | release-governance replay and blocker taxonomy |
| `bl-039-parameter-relay-spec-generation-qa.md` | BL-specific packet to keep active | parameter-relay contract authority and replay checks |
| `bl-040-ui-modularization-and-authority-status-qa.md` | BL-specific packet to keep active | authority-status replay and diagnostics contract |
| `bl-037-emitter-snapshot-cpu-budget-qa.md` | BL-specific packet to keep active | snapshot CPU budget and replay contract |
| `bl-038-calibration-threading-and-telemetry-qa.md` | BL-specific packet to keep active | calibration threading and telemetry evidence |
| `bl-049-unit-test-framework-and-tracker-automation-qa.md` | BL-specific packet to keep active | unit-test framework and tracker automation |
| `bl-041-doppler-v2-and-vbap-geometry-validation-qa.md` | BL-specific packet to keep active | Doppler/VBAP geometry validation |
| `bl-036-dsp-finite-output-guardrails-qa.md` | BL-specific packet to keep active | finite-output guardrail validation |
| `bl-046-sofa-hrtf-binaural-expansion-qa.md` | BL-specific packet to keep active | SOFA/HRTF binaural expansion validation |
| `bl-048-cross-platform-shipping-hardening-qa.md` | BL-specific packet to keep active | shipping-hardening replay and evidence |
| `bl-023-resize-dpi-hardening-qa.md` | BL-specific packet to keep active | resize and DPI hardening evidence |
| `bl-020-confidence-masking-qa.md` | BL-specific packet to keep active | confidence-masking validation |
| `bl-021-room-story-overlays-qa.md` | BL-specific packet to keep active | room-story overlay validation |
| `bl-044-quality-tier-seamless-switching-qa.md` | BL-specific packet to keep active | quality-tier switching validation |
| `bl-029-audition-platform-qa.md` | BL-specific packet to keep active | audition platform validation |
| `bl-042-qa-ci-regression-gates-qa.md` | BL-specific packet to keep active | QA/CI regression gate contract |
| `bl-028-spatial-output-matrix-qa.md` | BL-specific packet to keep active | spatial output matrix validation |
| `bl-032-modularization-qa.md` | BL-specific packet to keep active | modularization validation |
| `bl-034-headphone-verification-qa.md` | BL-specific packet to keep active | headphone verification checks |
| `bl-051-ambisonics-and-adm-roadmap-qa.md` | BL-specific packet to keep active | ambisonics and ADM roadmap checks |
| `bl-033-headphone-core-qa.md` | BL-specific packet to keep active | headphone core validation |
| `bl-035-rt-lock-free-registration-qa.md` | BL-specific packet to keep active | realtime lock-free registration checks |
| `bl-031-tempo-token-scheduler-qa.md` | BL-specific packet to keep active | tempo token scheduler validation |
| `bl-026-calibrate-uiux-v2-qa.md` | BL-specific packet to keep active | CALIBRATE v2 QA contract |
| `bl-045-headtracking-fidelity-qa.md` | BL-specific packet to keep active | head-tracking fidelity QA |
| `bl-053-head-tracking-orientation-injection-qa.md` | BL-specific packet to keep active | head-tracking orientation injection QA |

## Archived Off-Ramp

These packets were one-off evidence notes and now live in the archive set:

| File | Classification | Archive Path |
|---|---|---|
| `bl-025-emitter-resize-manual-qa-2026-02-23.md` | archive candidate | `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/bl-025-emitter-resize-manual-qa-2026-02-23.md` |
| `hx-04-scenario-coverage-audit-2026-02-23.md` | archive candidate | `Documentation/archive/2026-03-18-doc-surface-consolidation/testing/hx-04-scenario-coverage-audit-2026-02-23.md` |

## Follow-Up Archive Candidates

If we do another pass, the next strongest archive candidates are the largest BL-specific QA packets that duplicate runbook or `TestEvidence` truth:

- `bl-039-parameter-relay-spec-generation-qa.md`
- `bl-030-release-governance-qa.md`
- `bl-040-ui-modularization-and-authority-status-qa.md`
- `bl-037-emitter-snapshot-cpu-budget-qa.md`
- `bl-038-calibration-threading-and-telemetry-qa.md`
- `bl-049-unit-test-framework-and-tracker-automation-qa.md`
- `bl-041-doppler-v2-and-vbap-geometry-validation-qa.md`
- `bl-036-dsp-finite-output-guardrails-qa.md`

## Note

The active folder should stay short.
Prefer keeping reusable guides and contracts here.
Prefer moving evidence-heavy, one-off QA packets to the archive set once they are no longer the active source of truth.
