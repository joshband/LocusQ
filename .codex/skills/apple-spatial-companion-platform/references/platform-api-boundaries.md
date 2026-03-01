Title: Apple Platform API Boundaries
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Platform API Boundaries

## Companion-Owned APIs
- `CoreMotion` (`CMHeadphoneMotionManager`) for AirPods head-tracking motion.
- `AVFoundation` capture surfaces for guided photo/video inputs when in scope.
- `Vision` feature extraction/quality gates for deterministic image-processing pipelines.
- `ARKit` only for explicitly scoped optional depth/mesh experiments.

## Explicit Non-Goals
- No plugin audio-thread integration of Apple app-level render frameworks.
- No assumption of third-party API access to Apple Personalized Spatial Audio profiles.
- No architecture claims that require unsupported host/plugin runtime privileges.

## Canonical Local Anchors
- `Documentation/invariants.md`
- `Documentation/backlog/index.md`
- `Documentation/backlog/bl-057-device-preset-library.md`
- `Documentation/backlog/bl-058-companion-profile-acquisition.md`
- `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`
- `Documentation/reviews/2026-03-01-headtracking-research-backlog-reconciliation.md`
