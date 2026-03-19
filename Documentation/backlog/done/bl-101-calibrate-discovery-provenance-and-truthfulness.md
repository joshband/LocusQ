Title: BL-101 CALIBRATE discovery, provenance, and truthfulness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-19 (Done — contract 4/4 PASS, execute 3/3 PASS; inferred/detected/generic/unavailable/manual_override/stale states live in ProcessorUiBridgeOps.h)

# BL-101 CALIBRATE discovery, provenance, and truthfulness

## Plain-Language Summary

BL-101 in plain terms: make `CALIBRATE` honest and expandable by separating device discovery, topology inference, profile provenance, freshness, and verification truth into explicit contracts instead of implied status text. Current state: Open. This item is the direct follow-on from the 2026-03-18 CALIBRATE research packet and exists because the current panel can show useful status, but it still cannot prove all of its automation and verification claims end-to-end.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin operators, headphone users, speaker-calibration users, QA/release owners, and maintainers expanding layout/device support. |
| What is changing? | `CALIBRATE` gains explicit discovery, provenance, and freshness semantics, a truer automation contract, and dedicated QA for truthfulness across outputs, headphones, inputs, and verification surfaces. |
| Why is this important? | Today `CALIBRATE` can over-compress discovery, routing, profile activation, and verification into one set of status surfaces, which makes it harder to trust and harder to scale to new headphone/speaker configurations. |
| How will we deliver it? | Freeze the discovery/provenance contract first, then align bridge/UI payloads, then add CALIBRATE truthfulness QA, then use the new contract as the base for future automation and wider topology support. |
| When is it done? | This item is done when CALIBRATE surfaces can distinguish host-derived, companion-derived, measured, estimated, generic, stale, and manual states without ambiguity and automated lanes prove those distinctions. |
| Where is the source of truth? | This runbook, the annex packet `Documentation/plans/2026-03-18-calibrate-discovery-provenance-execution-packet.md`, the 2026-03-18 research packet, and repo-local evidence under `TestEvidence/bl101_*/`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Slice table | Separates contract work from runtime/UI/QA work. | `## Implementation Slices` |
| Validation table | Makes the truthfulness gates concrete. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-101 |
| Priority | P1 |
| Status | Done (2026-03-19: contract 4/4 PASS, execute 3/3 PASS; inferred/detected/generic/unavailable/manual_override/stale provenance states live in ProcessorUiBridgeOps.h; makeBl101Descriptor wired across all CALIBRATE surfaces) |
| Track | E - R&D Expansion |
| Effort | High / L |
| Depends On | BL-026 (Done), BL-038 (Done), BL-059 (Done), BL-099 (Open, complementary) |
| Blocks | future CALIBRATE capability-expansion lanes |
| Annex Spec | `Documentation/plans/2026-03-18-calibrate-discovery-provenance-execution-packet.md` |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Make `CALIBRATE` trustworthy and expandable by freezing one explicit contract for output discovery, input identification, topology inference, profile provenance, freshness, and verification truthfulness. BL-101 is complete only when `HEADPHONE DEVICE STATUS`, `AUTOMATION SUMMARY`, `CALIBRATION STATUS`, and `HEADPHONE VERIFY` can state what is detected, inferred, measured, estimated, generic, stale, or manually overridden without rewriting weaker evidence into stronger claims.

## Scope & Non-Scope

**In scope:**
- CALIBRATE-wide provenance enums and copy rules
- output/input discovery source semantics
- topology inference source semantics
- profile provenance source semantics
- stale/manual/override semantics in CALIBRATE
- dedicated CALIBRATE truthfulness QA lanes
- persistence/export/load handling for provenance fields

**Out of scope:**
- new measurement DSP algorithms
- wider-than-current calibration routing backend expansion
- personalized HRTF generation beyond existing BL-058 matching path
- changing privacy/retention rules for companion capture
- replacing BL-099 scope around named compensation provenance

## Architecture Context

- `Documentation/plans/2026-02-27-calibration-system-design.md` already separates companion, plugin, and profile responsibilities.
- `Documentation/plans/calibration-profile-schema-v1.md` already establishes a useful base profile schema but does not fully model discovery/provenance across all CALIBRATE surfaces.
- BL-026 established deterministic CALIBRATE UI contracts, but mostly for card-state/rendering behavior.
- BL-099 owns headphone verification/compensation truthfulness specifically; BL-101 complements it by owning the broader CALIBRATE discovery/provenance surface.

Primary supporting inputs:
- `Documentation/reports/2026-03-18-calibrate-automation-and-headphone-personalization-research-packet.md`
- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `Documentation/plans/2026-03-18-calibrate-redesign-execution-packet.md`

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Freeze discovery/provenance/truth contract for CALIBRATE. | runbook + annex plan + UI trust-language references + shared bridge field definitions as needed | source/provenance states and copy rules are explicit and approved |
| B | Align runtime + bridge publication with the contract. | `Source/processor_bridge/ProcessorUiBridgeOps.h`, `Source/processor_core/ProcessorCalibrationBridge.cpp`, `Source/ui/src/index.ts`, companion surfaces if needed | CALIBRATE can publish source, confidence, age/stale, and manual override state without ambiguity |
| C | Add dedicated CALIBRATE truthfulness QA. | targeted scripts, selftests, test fixtures, `Documentation/testing/*.md`, `TestEvidence/bl101_*` | automated lanes can prove weak evidence is not mislabeled as stronger truth |
| D | Prepare extensibility baseline for future topology/device growth. | topology/device registry design docs, profile schema follow-ons, targeted UI docs | future wider-layout and broader device support can build on one stable discovery/provenance model |

## Acceptance IDs

- `BL101-A1` Every auto-populated CALIBRATE field can identify its source as host, standalone scan, companion, persisted profile, or manual override.
- `BL101-A2` CALIBRATE truth surfaces can identify provenance state as measured, detected, inferred, estimated, generic, or unavailable, with freshness/state overlays such as stale and manual override expressed separately and deterministically.
- `BL101-A3` UI copy never upgrades a weaker evidence state into a stronger claim.
- `BL101-A4` Save/load/export/import paths preserve provenance fields or explicitly mark when provenance is unavailable.
- `BL101-A5` Automated QA exists for `HEADPHONE DEVICE STATUS`, `AUTOMATION SUMMARY`, `CALIBRATION STATUS`, and `HEADPHONE VERIFY` truth semantics.
- `BL101-A6` BL-101 remains complementary to BL-099 and does not regress its headphone-verification truthfulness goals.

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL101-DOCS | Automated | `./scripts/validate-backlog-plain-language.sh` + `./scripts/validate-backlog-redundancy.py` + `./scripts/validate-docs-freshness.sh` | exit 0 |
| BL101-CONTRACT | Automated | `scripts/qa-bl101-calibrate-truthfulness-mac.sh --contract-only` | required provenance/source states and copy rules are asserted |
| BL101-EXECUTE | Automated | `scripts/qa-bl101-calibrate-truthfulness-mac.sh --execute --runs 3` | runtime CALIBRATE surfaces preserve truthful source/provenance semantics |
| BL101-SELFTEST | Automated | `LOCUSQ_UI_SELFTEST_SCOPE=bl101 ./scripts/standalone-ui-selftest-production-p0-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app` | BL-101 UI truth surfaces pass with no false-green rows |
| BL101-EVIDENCE | Focused validation | compare runtime output and saved profile artifacts against fixture truth and explicit provenance expectations | user-visible claims match available evidence |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | contract + execute truthfulness lane | status/provenance snapshots + replay notes |
| Candidate intake | T2 | 5 | repeated truthfulness/provenance replay | replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected CALIBRATE truthfulness packet | owner packet + deterministic evidence |

## Governance Alignment (2026-03-18)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-101 is the CALIBRATE discovery/provenance follow-on from the 2026-03-18 research and redesign review set. It complements:

- BL-026: deterministic CALIBRATE UI contracts
- BL-059: calibration profile handoff
- BL-099: headphone verification and compensation provenance

BL-101 should become the main authority for CALIBRATE-wide truthfulness outside BL-099’s narrower headphone-verification scope.
