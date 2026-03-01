---
name: apple-spatial-companion-platform
description: Implement and validate Apple companion-platform integrations for AirPods/head-tracking workflows, including CoreMotion motion ingest, ear-photo/depth capture pipelines, privacy-retention controls, and platform API boundary decisions.
---

Title: Apple Spatial Companion Platform Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Apple Spatial Companion Platform

Use this skill when work depends on Apple platform APIs in the companion path (Swift/macOS), especially BL-057/BL-058 class slices.

## Scope
- CoreMotion integration contracts for `CMHeadphoneMotionManager` (availability, auth, device connect/disconnect, fixed-rate behavior).
- Companion capture pipeline for guided left/right/frontal ear-photo inputs and optional depth acquisition when explicitly supported.
- Apple-framework boundary decisions for `CoreMotion`, `AVFoundation`, `Vision`, and optional `ARKit` lanes.
- Privacy and retention safeguards for biometric-adjacent image/depth inputs (local processing, no unintended persistence, no network calls).
- Platform capability constraints and non-goals (for example no direct third-party API access to Apple Personalized Spatial Audio profiles).

## Required References
1. `references/platform-api-boundaries.md`
2. `references/capture-and-privacy-contract.md`
3. `references/validation-and-evidence.md`

## Workflow
1. Lock the platform boundary first.
   - Confirm what is companion-owned vs plugin-owned for the current slice.
   - Keep plugin `processBlock()` free of Apple app-level renderer dependencies.
2. Confirm API availability and deployment gates.
   - Record macOS minimum version assumptions.
   - Validate authorization and graceful-degrade paths.
3. Define capture contract before implementation.
   - Capture sequence, quality gates, and deterministic fallback behavior.
   - Output contract for profile-selection inputs (`subject_id`, `sofa_ref`, confidence/fallback reason).
4. Implement deterministic processing path.
   - Keep embedding/matching behavior reproducible for fixed inputs.
   - Keep fallback selection explicit and observable.
5. Enforce privacy/retention rules.
   - No implicit network calls in capture/match path.
   - No raw image persistence beyond explicitly-approved temporary scope.
6. Align runbook evidence contracts.
   - BL-057: device preset/profile mapping assumptions.
   - BL-058: profile acquisition, readiness/sync gating, and axis/frame diagnostics.
7. Validate and publish evidence packet.
   - Capture runtime status, fallback outcomes, and privacy checks.
8. Sync routing/docs when capability claims change.
   - Update matrix/runbook references and canonical evidence links.

## Cross-Skill Routing
- Pair with `headtracking-companion-runtime` for readiness/sync gating and axis/frame diagnostics.
- Pair with `spatial-audio-engineering` for renderer/channel/coordinate contract alignment.
- Pair with `hrtf-rendering-validation-lab` and `perceptual-listening-harness` when promotion decisions depend on parity or listening-gate evidence.
- Pair with `skill_docs` when backlog/runbook/index wording or governance surfaces must be updated.

## Deliverables
- File-level change list with API-boundary rationale.
- Explicit list of privacy/retention guarantees and fallback behavior.
- Validation status: `tested`, `partially tested`, or `not tested`.
