Title: BL-057 Device Preset Library (AirPods Pro 1/2/3 + WH-1000XM5)
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-03

# BL-057 Device Preset Library (AirPods Pro 1/2/3 + WH-1000XM5)

## Plain-Language Summary

BL-057 in plain terms: Create validated YAML EQ presets for AirPods Pro 1, Pro 2, Pro 3 (ANC on/off/transparency) and WH-1000XM5 (ANC on/off). Current state: Open. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and maintainers using this runbook as source of truth. |
| What is changing? | Create validated YAML EQ presets for AirPods Pro 1, Pro 2, Pro 3 (ANC on/off/transparency) and WH-1000XM5 (ANC on/off). |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: Open. This item is done when required acceptance checks pass and promotion evidence is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-057-device-preset-library.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

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
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Create validated YAML EQ presets for AirPods Pro 1, Pro 2, Pro 3 (ANC on/off/transparency) and WH-1000XM5 (ANC on/off). Source from AutoEq/oratory1990. Add frequency sweep validation check per preset.

## Acceptance IDs

- one YAML file per (model, mode) pair
- preamp_db field present
- each preset passes frequency sweep validation (no resonance >±3dB at Nyquist)
- WH-1000XM5 ANC-on/off split is validated


## Validation Plan

QA harness script: `scripts/qa-bl057-device-preset-library-mac.sh` (authored 2026-03-03).
Evidence schema: `TestEvidence/bl057_*/status.tsv`.

### Preset Inventory (2026-03-03)

| Model | Mode | Preset file | Status |
|---|---|---|---|
| AirPods Pro 1 | anc_on | `Resources/eq_presets/airpods_pro_1_anc_on.yaml` | present |
| AirPods Pro 1 | anc_off | `Resources/eq_presets/airpods_pro_1_anc_off.yaml` | present |
| AirPods Pro 1 | transparency | `Resources/eq_presets/airpods_pro_1_transparency.yaml` | present |
| AirPods Pro 2 | anc_on | `Resources/eq_presets/airpods_pro_2_anc_on.yaml` | present |
| AirPods Pro 2 | anc_off | `Resources/eq_presets/airpods_pro_2_anc_off.yaml` | present |
| AirPods Pro 2 | transparency | `Resources/eq_presets/airpods_pro_2_transparency.yaml` | present |
| AirPods Pro 3 | anc_on | `Resources/eq_presets/airpods_pro_3_anc_on.yaml` | present |
| AirPods Pro 3 | anc_off | `Resources/eq_presets/airpods_pro_3_anc_off.yaml` | present |
| AirPods Pro 3 | transparency | `Resources/eq_presets/airpods_pro_3_transparency.yaml` | present |
| Sony WH-1000XM5 | anc_on | `Resources/eq_presets/sony_wh1000xm5_anc_on.yaml` | present |
| Sony WH-1000XM5 | anc_off | `Resources/eq_presets/sony_wh1000xm5_anc_off.yaml` | present |

### Validation Commands (Batch Contract)

- `./scripts/qa-bl057-device-preset-library-mac.sh --contract-only --runs 3`
- `./scripts/qa-bl057-device-preset-library-mac.sh --execute --runs 1`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | runbook primary lane command at dev-loop depth | validation matrix + replay summary |
| Candidate intake | T2 | 5 (or heavy-wrapper 2-run cap) | runbook candidate replay command set | contract/execute artifacts + taxonomy |
| Promotion | T3 | 10 (or owner-approved heavy-wrapper 3-run equivalent) | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run sentinel drill when explicitly requested | parity/sentinel artifacts |

### Cost/Flake Policy

- Diagnose failing run index before repeating full multi-run sweeps.
- Heavy wrappers (`>=20` binary launches per wrapper run) use targeted reruns, candidate at 2 runs, and promotion at 3 runs unless owner requests broader coverage.
- Document cadence overrides with rationale in `lane_notes.md` or `owner_decisions.md`.


## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Only add runbook-specific handoff fields if they differ from the canonical contract.

## Governance Alignment (2026-02-28)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.
