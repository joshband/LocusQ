Title: CALIBRATE Discovery and Provenance Execution Packet
Document Type: Planning Packet
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Discovery and Provenance Execution Packet

## Purpose

Convert the 2026-03-18 CALIBRATE research packet into an execution-ready first wave for BL-101.

Primary goals:
- make automation source-of-truth explicit
- make CALIBRATE trust surfaces provable
- prepare the panel for wider topology/device support without overstating current capability

Primary inputs:
- `Documentation/reports/2026-03-18-calibrate-automation-and-headphone-personalization-research-packet.md`
- `Documentation/reports/2026-03-18-calibrate-review-and-redesign-spec.md`
- `Documentation/plans/2026-03-18-calibrate-redesign-execution-packet.md`
- `Documentation/backlog/bl-099-headphone-verification-truthfulness-and-compensation-provenance.md`

Validation status: `not tested`

## Scope Baseline

- `HEADPHONE DEVICE STATUS` currently reflects bridge payload values but lacks a dedicated end-to-end truth lane.
- `AUTOMATION SUMMARY` exists, but it does not yet fully disclose source, confidence, age, and override state for every field.
- `CALIBRATION STATUS` is partly contract-tested for rendering/state transitions, but not yet as a full semantic truth surface.
- `HEADPHONE VERIFY` has stronger automation than the rest of CALIBRATE, but current score truthfulness still depends on BL-099 follow-through.

## Execution Principles

1. Never present inferred or synthetic data as measured truth.
2. Separate source-of-truth from display copy.
3. Treat stale data as a first-class state.
4. Prefer additive bridge/schema changes over disruptive rewrites.
5. Build the QA lane alongside the truth contract, not after it.

## Wave Plan

### Wave 1A: Freeze The Contract

Objective:
Define one explicit semantics model for CALIBRATE discovery and provenance.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-P1` | Define source enums (`host`, `scan`, `companion`, `profile`, `manual`) | docs + bridge contracts | enums and copy rules are frozen |
| `CAL-P2` | Define provenance enums (`measured`, `detected`, `inferred`, `estimated`, `generic`, `unavailable`, `stale`) | docs + UI trust language | all CALIBRATE truth surfaces have allowed states |
| `CAL-P3` | Define stale/age/manual-override rules | docs + UI trust language | every auto-populated field can express freshness and override state |

### Wave 1B: Publish The Semantics

Objective:
Teach runtime and bridge payloads to carry the new truth fields.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-P4` | Add source/provenance fields to CALIBRATE payloads | `ProcessorUiBridgeOps.h`, calibration bridge | required fields are published deterministically |
| `CAL-P5` | Surface stale/age and override info in CALIBRATE | `index.ts`, `index.html` | automation summary and device/status cards expose freshness and override state |
| `CAL-P6` | Keep BL-099 headphone verification semantics aligned | `PluginProcessor.cpp`, bridge/UI touchpoints as needed | no contradictory truth-language across headphone verification surfaces |

### Wave 1C: Prove The Semantics

Objective:
Add automated CALIBRATE truthfulness gates.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-P7` | Add contract-only BL-101 lane | script + fixture docs | lane asserts allowed truth states and copy rules |
| `CAL-P8` | Add execute-mode BL-101 lane | script + fixture/runtime path | runtime output matches truth expectations |
| `CAL-P9` | Add BL-101 selftest scope for panel surfaces | `index.ts`, standalone selftest | UI truth surfaces pass deterministically in production shell |

### Wave 1D: Prepare Extensibility

Objective:
Leave behind a stable foundation for wider layout/device support.

| Slice ID | Description | Touch Zones | Exit Criteria |
|---|---|---|---|
| `CAL-P10` | Define discovery graph and topology/device registry direction | docs/spec only in first pass | future device/layout expansion has one contract anchor |
| `CAL-P11` | Define profile persistence expectations for provenance fields | schema/docs | save/load/export/import rules are explicit |

## Likely Write Set

- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `Source/PluginProcessor.cpp`
- `Documentation/testing/*.md`
- `scripts/qa-bl101-calibrate-truthfulness-mac.sh`

## Promotion Gates

| Gate | Requirement |
|---|---|
| `G1` | Every key CALIBRATE truth surface discloses source and provenance state |
| `G2` | stale and manual-override states are visible and deterministic |
| `G3` | BL-101 QA lanes exist and are machine-readable |
| `G4` | BL-099 semantics are not contradicted by CALIBRATE copy or payload mapping |
| `G5` | future topology/device expansion can point to one contract authority instead of ad-hoc status text |

## Validation Lanes

- `cd Source/ui && npm run typecheck`
- `cd Source/ui && npm run build`
- `cmake --build build_local --config Release --target LocusQ_Standalone -j 8`
- `LOCUSQ_UI_SELFTEST_SCOPE=bl101 ./scripts/standalone-ui-selftest-production-p0-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app`
- `scripts/qa-bl101-calibrate-truthfulness-mac.sh --contract-only`
- `scripts/qa-bl101-calibrate-truthfulness-mac.sh --execute --runs 3`

## Success Definition

This packet succeeds if BL-101 starts from a narrow but real foundation:
- CALIBRATE truth semantics are frozen
- runtime/UI can publish them
- automation can prove them
- and future automation/personalization work no longer has to guess what “auto-detected” or “verified” means
