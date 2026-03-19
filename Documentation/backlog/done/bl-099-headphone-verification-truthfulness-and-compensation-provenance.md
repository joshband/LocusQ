Title: BL-099 Headphone verification truthfulness and compensation provenance
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19 (Done — contract 3/3 PASS, execute 3/3 PASS; provenance fields live in HeadphoneVerificationContract.h + bridge)

# BL-099 Headphone verification truthfulness and compensation provenance

## Plain-Language Summary

BL-099 in plain terms: make headphone-verification scores and headphone-compensation behavior honest about what is actually measured, what is only estimated, and what is merely a generic placeholder. Current state: Done-candidate. Runtime verification stages still publish, but synthetic score tables no longer masquerade as evidence: operator-visible score provenance now falls back to `unavailable`, generic compensation stays explicitly generic, and the dedicated BL-099 execute lane is green.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin operators, DSP maintainers, QA/release owners, and trust-language/UI owners who need calibration diagnostics they can believe. |
| What is changing? | Verification-score publication gains explicit provenance, and named headphone-compensation behavior either becomes evidence-backed or is relabeled as generic/unmeasured. |
| Why is this important? | Today some calibration-quality surfaces read like measured truth even when they are static constants, which creates the same trust/governance risk highlighted by BL-095. |
| How will we deliver it? | First freeze the truth/provenance contract, then align runtime + bridge publication, then connect QA/release evidence so fabricated scores cannot masquerade as real validation. |
| When is it done? | This item is done when user-visible verification and compensation surfaces distinguish measured, estimated, generic, and unavailable states without ambiguity. |
| Where is the source of truth? | This runbook, the 2026-03-17 review reports, BL-034/BL-057 historical surfaces, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Acceptance + slice tables | Separates truth-contract work from runtime/provenance implementation. | `## Acceptance IDs`, `## Implementation Slices` |
| Validation table | Keeps “measured vs estimated” evidence explicit. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-099 |
| Priority | P1 |
| Status | Done (2026-03-19: contract 3/3 PASS, execute 3/3 PASS; measured/estimated/generic/unavailable provenance live in HeadphoneVerificationContract.h, sanitizeProvenance live in bridge, scoreProvenance + compensationProvenance published through ProcessorSceneStateBridgeOps.h) |
| Track | E - R&D Expansion |
| Effort | High / M |
| Depends On | BL-034 (Done), BL-057 (Done) |
| Blocks | BL-089 |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Restore truthfulness and provenance clarity to headphone-calibration quality surfaces. BL-099 is complete only when verification scores cannot be mistaken for measured perceptual results unless real evidence exists, and when named headphone-compensation behavior is either traceable to documented measurement sources or clearly framed as generic/unmeasured.

## Source Inputs

- `Documentation/reviews/2026-03-17-comprehensive-code-dsp-review.md`
- `Documentation/reviews/2026-03-17-second-opinion-code-dsp-supplement.md`
- `Documentation/backlog/done/bl-034-headphone-calibration-verification.md`
- `Documentation/backlog/done/bl-057-device-preset-library.md`
- `Documentation/backlog/done/bl-058-companion-profile-acquisition.md`
- `Documentation/backlog/bl-060-phase-b-listening-test-harness.md`
- `Source/PluginProcessor.cpp`
- `Source/PluginProcessor.h`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/shared_contracts/HeadphoneVerificationContract.h`
- `Source/spatial_renderer/SpatialHeadphoneCompensation.h`

## Acceptance IDs

- `BL099-A1` Operator-visible headphone-verification scores are never presented as measured perceptual results when their source is synthetic/static.
- `BL099-A2` Verification publication includes explicit provenance status such as measured, estimated, generic, or unavailable, and bridge/UI consumers preserve that status without rewriting it into stronger claims.
- `BL099-A3` Named headphone-compensation coefficients cite a documented measurement/provenance source or collapse to clearly generic compensation behavior with no pseudo-specific branding claims.
- `BL099-A4` Trust/copy surfaces differentiate personalized profile selection, generic fallback behavior, and unmeasured compensation tweaks in plain language.
- `BL099-A5` QA and release evidence for headphone verification/compensation truthfulness prevents static lookup tables or undocumented coefficients from satisfying “validated” claims.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Freeze the provenance/truth contract for verification scores and compensation labels. | runbook + shared contracts + trust-language references | measured/estimated/generic/unavailable rules are explicit and approved |
| B | Align runtime + bridge publication with the contract. | `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`, `Source/processor_bridge/ProcessorUiBridgeOps.h`, related UI/companion surfaces as needed | synthetic scores no longer publish as measured truth and compensation labeling is accurate |
| C | Reconcile named compensation behavior with actual provenance. | `Source/spatial_renderer/SpatialHeadphoneCompensation.h`, related docs/resources, optional preset/provenance references | named profiles are evidence-backed or clearly generic |
| D | Add QA/release truthfulness evidence. | `TestEvidence/bl099_*`, targeted QA scripts, backlog/evidence docs | validation can distinguish real measurement evidence from placeholder constants |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL099-DOCS | Automated | `./scripts/validate-backlog-plain-language.sh` + `./scripts/validate-backlog-redundancy.py` + `./scripts/validate-docs-freshness.sh` | exit 0 |
| BL099-CONTRACT | Automated | `scripts/qa-bl099-headphone-truthfulness-mac.sh --contract-only` | score-status/provenance rules and compensation provenance expectations are explicitly asserted |
| BL099-EXECUTE | Automated | `scripts/qa-bl099-headphone-truthfulness-mac.sh --execute --runs 3` | synthetic-score publication is labeled honestly and compensation provenance output matches the contract |
| BL099-EVIDENCE | Focused validation | compare runtime output against BL-060/listening-harness evidence or explicit `unavailable`/`estimated` fallback state | user-visible truth claims match available evidence |

## Current Evidence

- Runtime publication now keeps verification stage/fallback telemetry while withholding synthetic score evidence.
- Compensation remains labeled `Generic baseline compensation` with `generic` provenance.
- Green evidence:
  - `TestEvidence/bl099_headphone_truthfulness_contract_20260319T043352Z/status.tsv`
  - `TestEvidence/bl099_headphone_truthfulness_20260319T043553Z/status.tsv`
  - `TestEvidence/locusq_production_p0_selftest_20260319T043606Z.json`

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | contract + execute truthfulness lane | status/provenance snapshots + replay notes |
| Candidate intake | T2 | 5 | repeated truthfulness/provenance replay | replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected truthfulness packet | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-099 is the headphone-verification/provenance follow-on from the 2026-03-17 review set. It complements BL-095: BL-095 corrects false FIR engine/latency claims, while BL-099 corrects false or weakly grounded quality/provenance claims that are published to operators and reviewers.
