Title: Physics Model Selection Guide
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Model Selection Guide

## Goal
Match simulation complexity to audible value and runtime budget.

## Selection Criteria
- Behavioral objective (local motion, emergent group behavior, medium dynamics)
- State dimensionality and update frequency
- Determinism requirements
- Degradation path when CPU budget is exceeded

## Practical Guidance
- Start with reduced-order models and add complexity only after audible wins are verified.
- Prefer robust approximations over unstable high-fidelity models in real-time contexts.
