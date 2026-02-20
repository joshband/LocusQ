Title: Reactive Mapping Contracts
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Mapping Contracts

## Goal
Keep reactive mappings explicit, testable, and stable across updates.

## Required Fields
- Feature name and sample rate
- Normalization window and clamp range
- Mapping curve (linear, log, sigmoid, custom)
- Smoothing and hysteresis
- Output parameter and units

## Recommended Pattern
1. Normalize feature to 0..1.
2. Apply optional deadband.
3. Apply curve.
4. Smooth with configured attack/release.
5. Clamp output and publish.
