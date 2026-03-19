Title: CALIBRATE Automation and Headphone Personalization Research Packet
Document Type: Research Report
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Automation and Headphone Personalization Research Packet

## Status

Support report.
Validation status: `not tested`.

Legacy deep packet:
- `Documentation/archive/2026-03-18-doc-surface-consolidation/reports/2026-03-18-calibrate-automation-and-headphone-personalization-research-packet-legacy.md`

## Bottom Line

`CALIBRATE` should stop treating automation as one opaque feature.

The system needs five separate truth surfaces:
1. device discovery
2. channel and topology inference
3. calibration measurement
4. correction or profile application
5. verification and provenance

That split makes the UI more honest.
It also makes wider layouts and richer headphone flows easier to add later.

## Direct Answers

### Are the current CALIBRATE trust surfaces fully validated?

No.

Current state:
- `HEADPHONE DEVICE STATUS`: renders bridge payload state, but no dedicated truth lane proves it matches live device reality
- `AUTOMATION SUMMARY`: no dedicated source-of-truth verification lane was found
- `CALIBRATION STATUS`: some state-transition coverage exists, but not full semantic truthfulness
- `HEADPHONE VERIFY`: strongest automation exists here, but current scores still lean on synthetic or schema-oriented evidence rather than measured listening truth

Decision:
- treat BL-099 style truthfulness work as CALIBRATE gating, not optional polish

### Can LocusQ become more automated across speaker and headphone setups?

Yes.

But it needs a registry-driven model:
- topology registry
- device capability registry
- output discovery layer
- calibration recipe layer
- profile provenance layer
- analysis and verification layer

## Current State

### Strengths

- profile schema exists
- companion acquisition exists
- plugin handoff exists
- part of CALIBRATE already has deterministic UI state contracts
- headphone verification governance already exists

### Gaps

1. Automation is under-modeled.
   Source, confidence, age, and override state are not explicit enough.
2. Speaker calibration and headphone personalization are still too coupled.
3. Layout naming is broader than live writable calibration support.
4. Operator-facing truthfulness still overstates some verification surfaces.

## Architecture Moves

### 1. Add a discovery graph

Purpose:
- model outputs, inputs, topology candidates, source, and confidence explicitly

Use it for:
- `Auto-map Outputs`
- `Auto-select Input`
- topology suggestions
- automation summary rows
- warning copy

### 2. Add a topology registry

Purpose:
- replace hardcoded layout assumptions with canonical per-topology metadata

Registry should define:
- required channels
- writable channels
- speaker labels and positions
- recipe type
- supported discovery strategies
- supported verification strategies

### 3. Promote headphone personalization to a first-class flow

Flow:
1. identify device
2. resolve profile source
3. acquire personalization input
4. build correction chain
5. run verification
6. save profile and provenance

Required truth fields:
- device identity
- profile source
- measurement provenance
- ear or subject reference
- compensation chain
- verification metrics and freshness

### 4. Split profile storage into layers

Keep separate:
- `DeviceProfile`
- `EnvironmentProfile`
- `PersonalizationProfile`
- `VerificationProfile`

Reason:
- less duplication
- clearer provenance
- easier multi-device and multi-user growth

### 5. Add multi-pass analysis

Speaker checks:
- repeated sweep consistency
- polarity and phase stability
- inter-channel delay variance
- SNR and clipping issues

Headphone checks:
- left/right asymmetry
- profile fallback
- HRTF confidence
- externalization estimate provenance
- tracking readiness and staleness

Shared output:
- machine-readable issue labels
- suggested next actions

## Truthfulness QA Expansion

Recommended lane:
- either a new `BL-100` or a BL-099 follow-on

Required checks:
- every CALIBRATE truth surface carries provenance
- `estimated` and `generic` are never presented as `measured`
- automation summary rows always identify their source
- stale device or companion data is labeled stale
- headphone verification surfaces show `UNAVAILABLE` or `ESTIMATED` when real evidence is missing
- requested, active, and fallback states remain consistent

Required evidence:
- `status.tsv`
- `truth_matrix.tsv`
- `payload_fixture_manifest.json`
- `ui_snapshot_manifest.json`
- `provenance_mismatch.tsv`

## Suggested Backlog Waves

### Wave A: Truth and discovery foundation

- add discovery graph
- add provenance enums
- rename `Redetect Routing` to `Auto-map Outputs`
- add `Auto-select Input`
- add source, confidence, and age rows
- complete BL-099 truthfulness alignment in CALIBRATE

### Wave B: Registry and visualization

- topology registry
- topology renderer
- custom layout schema
- extensible speaker and headphone catalog

### Wave C: Headphone personalization expansion

- richer companion acquisition contract
- multi-profile personalization
- SOFA import and provenance
- asymmetry support
- objective plus perceptual verification write-back

### Wave D: Analysis and improvement engine

- repeated-pass analysis
- multi-position speaker measurement
- issue diagnosis and recommendations
- calibration comparison and upgrade suggestions

## Concrete Next Steps

1. Make BL-099-style truthfulness gating mandatory for CALIBRATE.
2. Add one short discovery and provenance contract doc.
3. Introduce a topology and device registry before adding more layouts.
4. Build a dedicated truthfulness selftest scope for the four CALIBRATE trust surfaces.
5. Expand headphone personalization from status display into acquisition, correction, verification, and provenance.

## Sources

Repo sources:
- `Documentation/plans/2026-02-27-calibration-system-design.md`
- `Documentation/plans/calibration-profile-schema-v1.md`
- `Documentation/backlog/done/bl-034-headphone-calibration-verification.md`
- `Documentation/backlog/done/bl-058-companion-profile-acquisition.md`
- `Documentation/backlog/bl-099-headphone-verification-truthfulness-and-compensation-provenance.md`
- `Source/PluginProcessor.cpp`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/ui/src/index.ts`

External references:
- JUCE `AudioIODevice`
- JUCE `AudioDeviceManager::AudioDeviceSetup`
- RtAudio probe example
- `libmysofa`
- `AutoEq`
- `CamillaDSP`
- `Impulcifer`
- `pyroomacoustics`

## Archive Note

Use this file for the current research conclusions.
Use the archive copy for the long-form repo audit, external research notes, and original architecture detail.
