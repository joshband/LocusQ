---
name: perceptual-listening-harness
description: Design and execute blinded perceptual listening studies for LocusQ (trial schema, randomization, metrics, statistical gating, reproducibility artifacts) aligned with BL-060 and BL-061 promotion decisions.
---

Title: Perceptual Listening Harness Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Perceptual Listening Harness

Use this skill for controlled listening-test design and evidence-driven promotion decisions.

## Scope
- Blind protocol design (2x2 and related variants).
- Trial schema and artifact contracts (`CSV/TSV/summary markdown`).
- Objective metric extraction (MAE, front/back confusion, externalization summary).
- Statistical gate decisions for backlog promotion.

## Workflow
1. Freeze protocol before collecting data.
   - Define participant count, scene count, condition matrix, and randomization policy.
2. Enforce blind run discipline.
   - Hide condition labels during response capture.
3. Capture machine-readable trial outputs.
   - Include true angle, response angle, error, condition, and reaction time.
4. Compute and publish gate metrics.
   - Per-condition MAE/confusion/externalization.
   - Statistical significance or predefined threshold outcome.
5. Publish reproducibility packet.
   - Re-run analysis and ensure stable summary outputs.

## Required Evidence
- `trial_log.csv`
- `metrics_summary.tsv`
- `stats_report.md`
- `gate_decision.md`
- `reproducibility_check.tsv`

## Cross-Skill Routing
- Pair with `hrtf-rendering-validation-lab` for offline/realtime render parity baselines.
- Pair with `spatial-audio-engineering` for coordinate/scene semantics and renderer assumptions.
- Pair with `skill_docs` for runbook/index promotion wording and evidence linkage.

## References
- `references/trial-schema.md`
- `references/gate-metrics.md`
- `references/reproducibility-contract.md`

## Deliverables
- Protocol summary and rationale.
- Explicit gate decision with evidence paths.
- Validation status: `tested`, `partially tested`, or `not tested`.
