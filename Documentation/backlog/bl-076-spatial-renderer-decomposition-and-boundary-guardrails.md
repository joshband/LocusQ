Title: BL-076 SpatialRenderer Decomposition and Boundary Guardrails
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-02

# BL-076 SpatialRenderer Decomposition and Boundary Guardrails

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-076 |
| Priority | P1 |
| Status | Open (owner planning-packet replay blocked by global-lock workspace hygiene stop) |
| Track | F - Hardening |
| Effort | High / L |
| Depends On | BL-050, BL-069, BL-070 |
| Blocks | — |
| Annex Spec | `(pending annex spec)` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard (apply heavy-wrapper containment when wrapper cost is high) |

## Objective

Decompose `Source/SpatialRenderer.h` into cohesive renderer modules with explicit ownership boundaries so the runtime can evolve without a single giant multipurpose header becoming a merge-risk and defect hotspot.

## Acceptance IDs

- `BL076-A-001`: `SpatialRenderer` responsibilities are split into named modules (for example: routing/mode orchestration, binaural/HRTF path, delay/FIR path, diagnostics snapshot publication, and format/profile contracts).
- `BL076-A-002`: `Source/SpatialRenderer.h` becomes a bounded orchestration/public-contract surface rather than a multipurpose implementation container.
- `BL076-A-003`: Structure guardrail lane enforces line-count and forbidden dependency rules for SpatialRenderer module boundaries.
- `BL076-A-004`: Existing smoke and RT-safety lanes remain green (`non_allowlisted=0`) after decomposition.
- `BL076-A-005`: Scene-state/bridge payload contracts remain parity-stable with deterministic replay evidence.

## Scope

In scope:
- `Source/SpatialRenderer.h` decomposition into focused `Source/spatial_renderer/*` units.
- Deterministic module-boundary and dependency guardrails for new SpatialRenderer units.
- Behavior-parity validation for existing rendering modes and bridge payloads.

Out of scope:
- New DSP features unrelated to decomposition.
- UI feature redesign work (handled in separate UI track runbooks).
- Runtime policy changes already owned by BL-069/BL-070 except where needed for structural extraction wiring.

## Validation Plan

QA harness script: `scripts/qa-bl076-spatial-renderer-structure-guardrails-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl076_*/status.tsv`.

Minimum evidence additions:
- `spatial_renderer_structure_guardrails.tsv`
- `spatial_renderer_module_dependency_matrix.tsv`
- `rt_audit.tsv`
- `smoke_parity_matrix.tsv`
- `bridge_payload_parity.tsv`

## Owner Intake Blocker Snapshot (2026-03-02)

- Handoff replay attempt for decomposition planning packet stopped before execution.
- Blocker: global-lock guard detected unrelated workspace edits outside task ownership:
  - `TestEvidence/locusq_production_p0_selftest_20260302T035100Z.attempts.tsv`
  - `TestEvidence/locusq_production_p0_selftest_20260302T035100Z.failure_taxonomy.tsv`
  - `TestEvidence/locusq_production_p0_selftest_20260302T035100Z.meta.json`
- No scoped BL-076 files were changed and no validation artifacts were produced for that attempt.

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

All worker and owner handoffs for this runbook must include:
- `SHARED_FILES_TOUCHED: no|yes`

Required return block:
```
HANDOFF_READY
TASK: <BL ID + Title>
RESULT: PASS|FAIL
FILES_TOUCHED: ...
VALIDATION: ...
ARTIFACTS: ...
SHARED_FILES_TOUCHED: no|yes
BLOCKERS: ...
```

## Governance Alignment (2026-03-01)

This additive section aligns the runbook with current backlog lifecycle and evidence governance without altering historical execution notes.

- Done transition contract: when this item reaches Done, move the runbook from `Documentation/backlog/` to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
- Evidence localization contract: canonical promotion and closeout evidence must be repo-local under `TestEvidence/` (not `/tmp`-only paths).
- Ownership safety contract: worker/owner handoffs must explicitly report `SHARED_FILES_TOUCHED: no|yes`.
- Cadence authority: replay tiering and overrides are governed by `Documentation/backlog/index.md` (`Global Replay Cadence Policy`).
