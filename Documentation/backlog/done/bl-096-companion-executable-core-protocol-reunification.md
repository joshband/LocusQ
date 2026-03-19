Title: BL-096 Companion executable/core protocol reunification
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19

# BL-096 Companion executable/core protocol reunification

## Plain-Language Summary

BL-096 in plain terms: reunify the shipping LocusQ Headtrack Companion executable with the tested core runtime so there is one real packet contract instead of parallel truths. Current state: In Validation. The local drift is now fixed: the shipping executable routes live and synthetic sends through the canonical core `MotionSample -> PosePacket` path, and the companion test suite covers the shared helper. Remaining work is closeout-quality parity evidence on the plugin decode side plus final owner sync.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Companion maintainers, plugin bridge owners, QA/release owners, and operators who depend on trustworthy live headtracking behavior. |
| What is changing? | The companion executable, core library, tests, docs, and plugin integration are brought back to one canonical transport/runtime contract. |
| Why is this important? | Right now the tested path is not the shipping path, which means green tests can still leave field behavior drifting. |
| How will we deliver it? | Freeze one canonical packet/runtime contract first, migrate the executable onto that path or remove duplicate transport code, then prove executable-to-plugin parity with integration evidence. |
| When is it done? | This item is done when the executable, core library, tests, and docs all describe and exercise the same packet/runtime truth. |
| Where is the source of truth? | This runbook, the 2026-03-17 review report, BL-072 historical surfaces, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Acceptance + slice tables | Clarify the source-of-truth decision and migration path. | `## Acceptance IDs`, `## Implementation Slices` |
| Protocol evidence | Keeps executable/core/plugin parity measurable. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-096 |
| Priority | P1 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | Medium / M |
| Depends On | BL-045 (Done), BL-072 (Done) |
| Blocks | — |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Choose one canonical pose-packet/runtime source of truth and make every relevant surface follow it: the shipping executable, `LocusQHeadTrackerCore`, tests, docs, and the plugin-side decode expectations. BL-096 is complete only when a runtime change cannot silently land in one path while the other stays green and stale.

## Source Inputs

- `Documentation/reviews/2026-03-17-comprehensive-code-dsp-review.md`
- `Documentation/backlog/done/bl-072-companion-runtime-protocol-parity-and-bl058-qa-harness.md`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`
- `companion/Sources/LocusQHeadTrackerCore/PosePacket.swift`
- `companion/Sources/LocusQHeadTrackerCore/TrackerApp.swift`
- `companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift`
- `companion/README.md`
- `Source/HeadTrackingBridge.h`

## Acceptance IDs

- `BL096-A1` One canonical pose-packet schema and serializer owner are explicitly chosen and documented.
- `BL096-A2` The shipping companion executable uses that same canonical path, or any legacy path is explicitly removed/quarantined from the shipping build.
- `BL096-A3` Tests cover the executable-relevant transport path, not just an internal library path.
- `BL096-A4` Plugin decode compatibility is verified against executable-generated payloads.
- `BL096-A5` README/operator docs reflect the actual shipping packet/runtime contract.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Freeze the canonical packet/runtime decision and update docs/contracts accordingly. | `companion/README.md`, runbook/index surfaces, companion protocol docs | one explicit runtime truth is documented |
| B | Migrate the executable onto the canonical core path, or remove duplicate transport code from shipping scope. | `companion/Sources/LocusQHeadTrackingCompanion/main.swift`, `companion/Sources/LocusQHeadTrackerCore/*` | executable and core no longer diverge |
| C | Add executable-to-plugin parity evidence and regression protection. | companion tests, plugin decode checks, `TestEvidence/bl096_*` | integration evidence proves shipping path parity |

## Latest Validation Snapshot

- 2026-03-19 local reunification slice: `PosePacketV1` removed from the shipping executable path.
- Live send path, synthetic send path, and `TrackerApp` now serialize through the same core helper: `MotionSample.posePacket(sequence:)`.
- Shared helper added at `companion/Sources/LocusQHeadTrackerCore/PosePacket+MotionSample.swift`.
- Regression coverage expanded in `companion/Tests/LocusQHeadTrackerTests/PosePacketTests.swift`.
- Current evidence:
  - `TestEvidence/bl096_companion_runtime_reunification_20260319T045834Z/status.tsv`
  - `TestEvidence/bl096_companion_runtime_reunification_20260319T045834Z/summary.md`

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL096-SWIFT | Automated | `cd companion && swift test` plus `cd companion && swift build` | exit 0, no protocol drift between executable and core |
| BL096-CONTRACT | Automated | `scripts/qa-bl096-companion-runtime-reunification-mac.sh --contract-only` | docs, executable path, core path, and plugin decode contract align |
| BL096-EXECUTE | Automated | `scripts/qa-bl096-companion-runtime-reunification-mac.sh --execute --runs 3` | executable/core/plugin parity checks pass with zero `TODO` rows |
| BL096-INTEGRATION | Focused validation | representative executable-generated packet replay into plugin decode path | packet version/size/field expectations match shipping behavior |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | companion build/test + contract/execute parity lane | status table + replay notes |
| Candidate intake | T2 | 5 | owner-selected parity replay | replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected executable/core/plugin parity set | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-096 is a corrective follow-on for BL-072. BL-072 remains valuable as the historical harness closeout, but it can no longer be read as proof that the current shipping executable and tested core runtime are unified until BL-096 closes.
