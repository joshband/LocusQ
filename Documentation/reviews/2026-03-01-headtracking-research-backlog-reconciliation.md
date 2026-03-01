Title: Head-Tracking Research-to-Backlog Reconciliation (BL-053..BL-061)
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Head-Tracking Research-to-Backlog Reconciliation (BL-053..BL-061)

## Purpose

Consolidate the pasted head-tracking + binaural research into one implementation-facing reconciliation, then map it to in-flight backlog runbooks and done artifacts without forcing unnecessary architecture churn.

## Skills Used (Order + Timing)

| Stage | Skills | Why |
|---|---|---|
| Scope framing and sequencing | `skill_plan` | Build a stable, low-risk execution order across backlog + evidence + docs surfaces. |
| Canonical document and runbook alignment | `skill_docs` | Normalize research into backlog-ready language, acceptance IDs, and evidence contracts. |
| Spatial math and renderer contracts | `spatial-audio-engineering` | Verify quaternion/orientation, FIR/partitioned convolver, and listening-test methodology alignment. |
| Steam runtime integration constraints | `steam-audio-capi` | Confirm monitoring-path fallback/activation behavior and orientation-consumption expectations. |
| Companion visualization and frame behavior | `threejs` | Validate axis semantics, view behavior, and diagnostic instrumentation requirements. |
| Repro and anomaly capture | `skill_troubleshooting` | Convert observed runtime anomalies into deterministic checks and acceptance language. |

## Inputs Reviewed

- `Documentation/research/`
- `Documentation/Calibration POC/`
- `Documentation/backlog/`
- `Documentation/backlog/done/`
- `Documentation/backlog/index.md`
- `Documentation/backlog/_template-intake.md`
- `Documentation/backlog/_template-runbook.md`
- `Documentation/reviews/`
- `Documentation/runbooks/`
- `Documentation/reports/`
- `Documentation/plans/`
- `TestEvidence/`

## Reconciliation Matrix

| Research Theme | Backlog Mapping | Current State | Required Adjustment |
|---|---|---|---|
| Orientation pipeline integrity (companion -> bridge -> plugin monitor path) | BL-053, BL-052 (done), BL-045 (done) | Core path implemented; structural lanes pass; manual checklist previously marked pending. | Keep BL-053 `In Validation`; add explicit manual acceptance evidence sync and owner-promotion preconditions. |
| Adaptive quaternion smoothing (SLERP low-pass + One-Euro cutoff + spike clamp) | BL-053, BL-059 | Methodology captured; production rollout still not explicitly acceptance-bound. | Add explicit optional v1.2 acceptance language and telemetry requirements to BL-053/BL-059 scope notes. |
| Stale packet guard + sequence/age observability | BL-053, BL-058 | Stale fallback exists; observability requirements are scattered. | Consolidate freshness/sequence diagnostics into BL-058 acceptance and evidence schema. |
| Companion readiness gating (`disabled_disconnected`, `active_not_ready`, `active_ready`) + user sync/center flow | BL-058, BL-053 | Runtime behavior implemented and user-validated; backlog wording under-specifies the state machine. | Promote this to explicit BL-058 acceptance IDs and test checklist entries. |
| Three.js/companion axis sanity (yaw/pitch/roll principal movement correctness) | BL-058 | Mentioned, but not fully formalized as deterministic QA gates. | Add synthetic axis sweep contract and per-view diagnostics to BL-058 validation plan. |
| Offline deterministic SOFA reference lane | BL-055, BL-060, Calibration POC | POC exists and is strong; runbook linkage is weak. | Add BL-055 and BL-060 references to offline truth-render parity artifacts as mandatory pre-promotion evidence. |
| Real-time partitioned convolution + crossfaded filter switching | BL-055, BL-061 | Strategy captured in research; backlog acceptance still generic. | Tighten BL-055/BL-061 acceptance to require crossfade/no-zipper/no-RT-alloc/no-lock guarantees. |
| Device preset + profile acquisition path | BL-057, BL-058 | Current plan says MobileNet path; research baseline favors deterministic nearest-neighbor first. | Reframe BL-058 to ship deterministic nearest-neighbor baseline first; defer heavier ML variants behind follow-up intake. |
| Controlled listening test harness (blind protocol) | BL-060 | Gate criteria exist, but protocol details are too sparse. | Add explicit trial schema, objective metrics, and evidence outputs (CSV/statistics/artifacts). |
| Conditional interpolation/personalization phase | BL-061 | Conditional status is correct. | Keep conditional gate; require BL-060 objective delta before promotion. |

## Recommendation: Integration Strategy

1. Keep current architecture and integrate the new research as scoped backlog acceptance updates.
2. Do not launch a large refactor until BL-053..BL-060 promotion evidence converges.
3. Build standalone/POC slices only where they de-risk high-complexity work:
   - offline truth renderer parity (BL-055),
   - listening harness analytics and protocol repeatability (BL-060).
4. Keep ML-heavy personalization beyond deterministic nearest-neighbor as follow-on intake (post BL-060 gate).

## Prioritized TODOs

### P0 (stability + correctness)

1. Complete BL-053 manual acceptance evidence packet using live operator checklist and runtime notes.
2. Formalize BL-058 readiness-state + sync/center gating acceptance and synthetic axis sweep checks.
3. Preserve stale-pose fallback and orientation activation checks as hard failures in BL-053/BL-058 lanes.

### P1 (methodology-to-runbook alignment)

1. Tighten BL-055 acceptance around partitioned-convolver + crossfade correctness and RT-safety invariants.
2. Tighten BL-060 protocol requirements (blind trial schema, MAE/confusion/externalization metrics, reproducible stats outputs).
3. Tighten BL-061 conditional gate language to require demonstrated BL-060 gain before interpolation promotion.

### P2 (post-gate expansion)

1. Add One-Euro adaptive smoothing mode as an optional, measured variant (no default flip without evidence).
2. Add deterministic nearest-neighbor vs interpolated profile A/B evidence lane before any ML-generated HRTF path.

## Change-Set Notes (This Reconciliation Pass)

- Updated backlog runbooks to encode the methodology details above where they were previously underspecified.
- Kept backlog lifecycle contract unchanged (`Documentation/backlog/index.md` remains canonical authority).
- Added/updated repo-local evidence notes under `TestEvidence/` for this reconciliation pass.

## Validation Gate

- Required docs gate: `./scripts/validate-docs-freshness.sh`

