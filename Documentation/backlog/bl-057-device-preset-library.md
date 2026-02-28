Title: BL-057 Device Preset Library (AirPods Pro 1/2/3 + WH-1000XM5)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-057 Device Preset Library (AirPods Pro 1/2/3 + WH-1000XM5)

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-057 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-046 |
| Blocks | BL-058 |

## Objective

Create validated YAML EQ presets for AirPods Pro 1, Pro 2, Pro 3 (ANC on/off/transparency) and WH-1000XM5 (ANC on/off). Source from AutoEq/oratory1990. Add frequency sweep validation check per preset.

## Acceptance IDs

- one YAML file per (model, mode) pair
- preamp_db field present
- each preset passes frequency sweep validation (no resonance >±3dB at Nyquist)
- WH-1000XM5 ANC-on/off split is validated

## Validation Plan

QA harness script: `scripts/qa-bl057-device-preset-library-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl057_*/status.tsv`.
