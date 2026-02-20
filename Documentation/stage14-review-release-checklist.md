Title: LocusQ Stage 14 Review and Release Checklist
Document Type: Stage Checklist
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# LocusQ Stage 14 Review and Release Checklist

## Purpose
Define one canonical closeout checklist for:

1. comprehensive architecture/code/design/QA review,
2. portable device-profile validation (laptop speakers, mic input, headphones),
3. release decision (`hold`, `draft-pre-release`, `ga`).

## Normative Inputs
- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Documentation/invariants.md`
- `Documentation/implementation-traceability.md`
- `Documentation/adr/ADR-0002-routing-model-v1.md`
- `Documentation/adr/ADR-0003-automation-authority-precedence.md`
- `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`
- `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`

## Stage 14A - Contract Alignment

### Checklist
- [x] Align `.ideas` docs to as-built parameter/runtime behavior.
- [x] Record device-profile contract in ADR + invariants.
- [x] Refresh traceability rows for Stage 12 renderer bindings and deferred parameters.
- [ ] Resolve all remaining spec/implementation drifts or mark each as intentionally deferred.

## Stage 14B - Comprehensive Review

### Architecture Review
- [ ] Re-verify renderer scene contract and output-layout mapping assumptions (`mono/stereo/quad`) against current source.
- [ ] Re-verify calibration routing assumptions for built-in/external mic paths.
- [ ] Reconfirm ADR alignment for any behavior changed since Stage 13.

### Code Review
- [ ] Findings-first review of `Source/PluginProcessor.cpp` hot paths for deterministic behavior and no-op/dead parameter risks.
- [ ] Findings-first review of `Source/PluginEditor.cpp` and incremental UI bridge wiring for control coverage gaps.
- [ ] Findings-first review of key scripts under `scripts/` for release/validation drift.

### Design Review
- [ ] Validate Stage 12 UI against `Design/v3-ui-spec.md`, `Design/v3-style-guide.md`, and `Design/HANDOFF.md`.
- [ ] Confirm portable-device UX clarity (output profile visibility, mic routing status, calibration state messaging).
- [ ] Confirm viewport behavior remains coherent under fallback/degraded paths.

### QA Review
- [ ] Re-run manual DAW checklist and attach evidence updates.
- [ ] Confirm non-manual matrix remains green on targeted suites.
- [ ] Confirm pluginval + standalone smoke remain green on release candidate artifacts.

## Stage 14C - Portable Device Validation

### Required Manual Checks
- [ ] Standalone: laptop speakers playback sanity (stereo output).
- [ ] Standalone: headphone playback sanity (stereo output).
- [ ] Host DAW: laptop speakers playback sanity.
- [ ] Host DAW: headphone playback sanity.
- [ ] Calibration flow with built-in mic routing.
- [ ] Calibration flow with external mic routing (if available).

### Evidence Targets
- `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

## Stage 14D - Release Decision

### Gate Checks
- [ ] Manual DAW checklist status: PASS.
- [ ] Stage 13/14 automated checks status: PASS (or documented warn-only exceptions).
- [ ] Distribution artifacts present and reproducible.
- [ ] GitHub release plan selected:
  - [ ] `draft-pre-release` (state lock before manual signoff), or
  - [ ] `ga` (all gates complete).

### Commands
- `./scripts/validate-docs-freshness.sh`
- `gh release list --limit 5`
- `git status --short --branch`

## Deferred Until Explicit Approval
- Implementing runtime behavior for `rend_phys_interact`.
- Adding new UI controls for currently unbound params (`emit_dir_azimuth`, `emit_dir_elevation`, `phys_vel_x/y/z`, `rend_phys_interact`).
