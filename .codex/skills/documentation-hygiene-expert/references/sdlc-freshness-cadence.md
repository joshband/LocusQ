Title: SDLC Freshness Cadence Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# SDLC Freshness Cadence

## Cadence Model
- `Plan docs`: review at phase transitions or architecture changes.
- `Design docs`: review on UI/UX scope changes and before implementation handoff.
- `Implementation docs`: review on merged behavior changes.
- `Testing docs`: review on lane changes, gate changes, or failing replays.
- `Release docs`: review before promotion and shipping decisions.

## Freshness Trigger Conditions
- New ADR created or ADR superseded.
- Invariant changed.
- Backlog state changes (`todo` -> `in_progress` -> `done`).
- Validation workflow or evidence contract changes.
- Ownership transfer for a subsystem.

## Freshness Contract
For each critical canonical doc, define:
- owner,
- review cadence,
- trigger conditions,
- escalation path when stale.
