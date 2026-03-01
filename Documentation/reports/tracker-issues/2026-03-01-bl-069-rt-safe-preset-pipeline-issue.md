Title: Tracker Issue Draft - BL-069 RT-Safe Headphone Preset Pipeline and Failure Backoff
Document Type: Tracker Issue Draft
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# BL-069 Tracker Issue Draft

## Proposed Title

BL-069: RT-safe headphone preset pipeline and failure backoff

## Summary

Move headphone preset loading/parsing out of the audio callback path and add deterministic retry backoff when preset files are missing/invalid.

## Evidence

- `Documentation/reviews/2026-03-01-code-review-backlog-reprioritization.md` (Finding #1)
- `Source/PluginProcessor.cpp:2049`
- `Source/PluginProcessor.cpp:2318`
- `Source/SpatialRenderer.h:644`
- `Source/SpatialRenderer.h:679`
- `Source/SpatialRenderer.h:682`
- `Source/SpatialRenderer.h:690`

## Acceptance Checklist

- [ ] `processBlock()` performs no preset filesystem or parse work.
- [ ] Missing/invalid preset assets cannot retrigger every callback block.
- [ ] Prepared coefficients are atomically swapped into RT path.
- [ ] Runtime status exposes failure/backoff diagnostics.
