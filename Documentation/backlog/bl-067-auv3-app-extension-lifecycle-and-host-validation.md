Title: BL-067 AUv3 App-Extension Lifecycle and Host Validation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-19

# BL-067 AUv3 App-Extension Lifecycle and Host Validation

## Plain-Language Summary

BL-067 in plain terms: Add production-ready AUv3 format support for LocusQ with deterministic extension lifecycle behavior, sandbox-safe runtime boundaries, and explicit parity validation against existing AU/VST3/CLAP formats. Current state: In Validation (2026-03-19 local runtime-access hardening replay: contract `1/1` PASS for profile/SOFA fallback paths; Apple signing and real host execution are still blocked). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA owners, release owners, and engineering maintainers who depend on deterministic evidence. |
| What is changing? | Add production-ready AUv3 format support for LocusQ with deterministic extension lifecycle behavior, sandbox-safe runtime boundaries, and explicit parity validation against existing AU/VST3/CLAP formats. |
| Why is this important? | It reduces risk and keeps related backlog lanes from being blocked by unclear behavior or missing evidence. |
| How will we deliver it? | Deliver in slices, run the required replay/validation lanes, and capture evidence in TestEvidence before owner promotion decisions. |
| When is it done? | Current state: In Validation (2026-03-19 local runtime-access hardening replay: profile/SOFA fallback checks PASS; Apple signing and host execution still blocked). This item is done when required acceptance checks pass and promotion evidence is complete. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |


## Visual Aid Index

Use visuals only when they materially improve understanding.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |
| Implementation slices | Clarifies execution sequence and ownership. | `## Implementation Slices` |
| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |

## Delivery Flow Diagram

Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.

Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`).

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-067 |
| Priority | P1 |
| Status | In Validation (2026-03-19 runtime-access contract replay PASS for profile/SOFA fallback paths; Apple signing and real host execution remain blocked) |
| Track | A - Runtime Formats |
| Effort | High / L |
| Depends On | BL-048 |
| Blocks | — |
| Annex Spec | `Documentation/plans/bl-067-auv3-app-extension-lifecycle-and-host-validation-spec-2026-03-01.md` |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | High-cost wrapper (host matrix + extension lifecycle sweeps) |

## Objective

Add production-ready AUv3 format support for LocusQ with deterministic extension lifecycle behavior, sandbox-safe runtime boundaries, and explicit parity validation against existing AU/VST3/CLAP formats.

## Acceptance IDs

- AUv3 build target and packaging pipeline are reproducible and code-signed for host execution.
- Extension lifecycle transitions (cold start, reload, suspend/resume, state restore) complete without crashes or stale state.
- Audio-thread invariants remain intact (no allocation/locks/blocking I/O in realtime callbacks).
- Profile, SOFA, and related runtime resource access paths are validated as extension-safe and do not rely on standalone-style user-home or desktop-dialog assumptions inside AUv3 hosts.
- AUv3-specific constraints degrade deterministically without host-name branching behavior.
- AU/VST3/CLAP regression lanes remain green after AUv3 enablement.
- Execute-mode QA evidence contains zero `TODO` rows (BL-073 scaffold-truthfulness gate).

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | AUv3 target wiring and build/packaging contracts | AUv3 target builds and launches in baseline host smoke lane |
| B | Extension-safe runtime boundaries and lifecycle handling, including profile/SOFA/runtime resource access | lifecycle transition matrix passes with deterministic state restore and extension-safe file/resource assumptions are explicitly proven |
| C | Cross-format parity and ship evidence packet | AUv3 + AU/VST3/CLAP parity matrix is green and evidence-complete |

## Validation Plan

QA harness script: `scripts/qa-bl067-auv3-lifecycle-mac.sh`.
Evidence schema: `TestEvidence/bl067_*/status.tsv`.

Lane behavior contract:
- `--contract-only` validates doc/build contracts and emits deterministic matrices.
- `--execute` enforces zero-`TODO` rows across `host_matrix.tsv`, `lifecycle_transitions.tsv`, and `parity_regression.tsv`.
- `--runs <n>` performs deterministic replay and writes per-run artifacts plus `run_summary.tsv`.

Minimum evidence additions:
- `host_matrix.tsv` (AUv3 host coverage and outcomes)
- `lifecycle_transitions.tsv` (cold/warm/reload/suspend-resume results)
- `parity_regression.tsv` (AUv3 vs AU/VST3/CLAP contract outcomes)
- `sandbox_runtime_access.tsv` (profile/SOFA/runtime resource access outcomes under AUv3-safe assumptions)
- `packaging_manifest.md` (targets, signing, packaging notes)
- `run_summary.tsv` (replay run-level pass/fail and `TODO` row counts)

## Validation Intake Snapshot (2026-03-17)

- `bash -n scripts/qa-bl067-auv3-lifecycle-mac.sh` -> `PASS`
- `./scripts/qa-bl067-auv3-lifecycle-mac.sh --contract-only --runs 3 --out-dir /Users/artbox/Documents/Repos/LocusQ-bl067-intake/TestEvidence/bl067_auv3_lifecycle_intake_20260317T191247Z_contract --build-root /Users/artbox/Documents/Repos/LocusQ-bl067-intake/build_bl067_auv3_lane_intake_20260317T191247Z_contract` -> `PASS`
- `./scripts/qa-bl067-auv3-lifecycle-mac.sh --execute --runs 1 --out-dir /Users/artbox/Documents/Repos/LocusQ-bl067-intake/TestEvidence/bl067_auv3_lifecycle_intake_20260317T191247Z_execute --build-root /Users/artbox/Documents/Repos/LocusQ-bl067-intake/build_bl067_auv3_lane_intake_20260317T191247Z_execute` -> `PASS`
- contract artifact root: `/Users/artbox/Documents/Repos/LocusQ-bl067-intake/TestEvidence/bl067_auv3_lifecycle_intake_20260317T191247Z_contract`
- execute artifact root: `/Users/artbox/Documents/Repos/LocusQ-bl067-intake/TestEvidence/bl067_auv3_lifecycle_intake_20260317T191247Z_execute`
- execute `TODO` rows: `0` (`/Users/artbox/Documents/Repos/LocusQ-bl067-intake/TestEvidence/bl067_auv3_lifecycle_intake_20260317T191247Z_execute/run_summary.tsv`)
- signing probe: `/Users/artbox/Documents/Repos/LocusQ-bl067-intake/TestEvidence/bl067_auv3_lifecycle_intake_20260317T191247Z_execute/signed_build_probe.log` shows Xcode can only sign for local execution; the AUv3 bundle and embedding app still report `Signature=adhoc`
- `code blockers`: none observed in Slice A/B intake evidence
- `signing blockers`: Apple-host-ready signing is still unmet (`Signature=adhoc`; execute capture has no TeamIdentifier)
- `host-inventory blockers`: Logic Pro is present but inventory-only; GarageBand and MainStage are not installed
- `runtime-access blockers`: the 2026-03-17 review identified unresolved AUv3-specific follow-up around user-home/app-data-style profile and SOFA access assumptions; explicit host evidence for extension-safe runtime access was still required
- `recommendation`: move BL-067 to In Validation; do not promote until Apple signing, host-execution inventory, and extension-safe runtime-access blockers are cleared

## Runtime-Access Hardening Snapshot (2026-03-19)

- `cmake --build build_local --config Release --target LocusQ_Standalone -j8` -> `PASS`
- `bash -n scripts/qa-bl067-auv3-lifecycle-mac.sh` -> `PASS`
- `./scripts/qa-bl067-auv3-lifecycle-mac.sh --contract-only --runs 1 --out-dir TestEvidence/bl067_runtime_access_20260319T034500Z --build-root build_bl067_runtime_access_20260319T034500Z` -> `PASS`
- new contract artifact: `TestEvidence/bl067_runtime_access_20260319T034500Z/sandbox_runtime_access.tsv`
- source-level result:
  - companion calibration-profile fallback now uses the LocusQ user-data directory only
  - custom SOFA fallback now uses the LocusQ user-data directory only
  - env overrides for explicit profile injection remain available
- remaining blockers:
  - Apple host-ready signing is still blocked
  - real AUv3 host execution is still blocked by missing host inventory/coverage
  - this slice proves the local runtime-access contract, not final AUv3 promotion readiness

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

## Governance Alignment (2026-03-01)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook should list only item-specific exceptions or additions.
