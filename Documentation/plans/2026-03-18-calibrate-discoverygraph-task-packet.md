Title: CALIBRATE DiscoveryGraph Wave A/B Task Packet
Document Type: Task Packet
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE DiscoveryGraph Wave A/B Task Packet

## Purpose

Convert the CALIBRATE autopilot roadmap into concrete implementation tasks for the first two execution waves:
- Wave A: Discovery graph foundation
- Wave B: topology and device registries

This packet is intentionally task-shaped.
It is meant to answer:
- what we build first
- what files are likely involved
- what can run in parallel
- what must be proven before the next wave begins

Validation status: `not tested`

## Scope

In scope:
- additive `DiscoveryGraph` contract
- output and input candidate publication
- ambiguity and confirmation states
- topology registry seed
- device registry seed
- initial QA lanes for discovery and registry truth

Out of scope:
- new measurement DSP
- deeper companion acquisition changes
- custom layout authoring UX
- multi-pass analysis engine
- full profile library UX

## Suggested Backlog Split

Recommended ownership split from this packet:

- `BL-102` DiscoveryGraph and automatic output/input identification
- `BL-103` Topology and device registries
- `BL-104` Headphone personalization workflow expansion
- `BL-105` Calibration analysis and multi-pass diagnostics

Reason:
- `BL-102` delivers the first honest automation substrate
- `BL-103` prevents further hardcoded growth
- `BL-104` can then build on registry and truth foundations
- `BL-105` becomes cleaner once the earlier contracts exist

## Complexity

Score: `5/5`

Rationale:
- touches native runtime, bridge payloads, and UI model/rendering
- must remain aligned with BL-099 and BL-101 truth rules
- introduces new foundational data models that later waves will depend on

## Implementation Strategy

Phased with parallel workstreams.

Recommended work mode:
1. freeze the data contracts
2. publish minimal runtime payloads
3. render them in a small UI surface
4. add dedicated QA
5. only then expand registry breadth

## Workstreams

### Workstream A: DiscoveryGraph Contract

Goal:
define one additive model for discovery knowledge

Primary deliverables:
- `DiscoveryGraph` doc section or dedicated contract doc
- bridge payload shape for output/input/topology/device candidates
- ambiguity and confirmation semantics

Likely write set:
- `Documentation/scene-state-contract.md`
- `Documentation/plans/2026-03-18-calibrate-autopilot-execution-packet.md`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`

### Workstream B: Runtime Publication

Goal:
publish real output/input candidate data into CALIBRATE

Primary deliverables:
- output candidate nodes
- input candidate nodes
- topology candidate nodes
- source/provenance/freshness overlays

Likely write set:
- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `Source/PluginProcessor.h`
- `Source/processor_bridge/ProcessorUiBridgeOps.h`

### Workstream C: UI Surfacing

Goal:
show discovery results, ambiguity, and confirmation needs without overwhelming the panel

Primary deliverables:
- `Detected Outputs`
- `Detected Inputs`
- `Topology Candidates`
- `Needs Confirmation`

Likely write set:
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`

### Workstream D: Registry Seed

Goal:
replace narrow hardcoded topology/device assumptions with registry-backed metadata

Primary deliverables:
- topology registry seed
- device registry seed
- migration rules for existing dropdowns and labels

Likely write set:
- `Source/processor_core/ProcessorParameterLayout.cpp`
- `Source/ui/src/index.ts`
- new registry docs if needed

### Workstream E: QA and Evidence

Goal:
prove discovery and registry truth without weakening BL-099 or BL-101

Primary deliverables:
- discovery contract lane
- discovery execute lane
- registry contract lane
- evidence bundle entries

Likely write set:
- `scripts/qa-bl102-discovery-graph-mac.sh`
- `scripts/qa-bl103-topology-registry-mac.sh`
- `Source/ui/src/index.ts`
- `status.json`

## Todo Checklist

### Wave A: Discovery Graph Foundation

- [ ] Define `DiscoveryGraph` node kinds:
  output, input, endpoint, topology_candidate, device_candidate, active_profile, ambiguity
- [ ] Define `DiscoveryGraph` edge kinds:
  derived_from, confirms, conflicts_with, overridden_by, expires_at
- [ ] Define required node metadata:
  `id`, `kind`, `label`, `source`, `provenance`, `ageMs`, `staleAfterMs`, `isStale`, `manualOverride`
- [ ] Define candidate ranking fields:
  `rank`, `confidence`, `reasonCodes`
- [ ] Define confirmation state fields:
  `needsConfirmation`, `confirmationPrompt`, `blockedBy`
- [ ] Publish output candidates from host/runtime state
- [ ] Publish input candidates for standalone mic suggestion
- [ ] Publish topology candidates from channel width plus route evidence
- [ ] Render a compact CALIBRATE discovery summary
- [ ] Add a `Needs Confirmation` list for ambiguous states
- [ ] Add BL-102 contract-only QA lane
- [ ] Add BL-102 execute lane

### Wave B: Registries

- [ ] Define topology registry record shape
- [ ] Seed stereo, quad, downmix stereo, steam binaural, virtual binaural
- [ ] Define optional wide-layout placeholder registry entries with explicit capability labels
- [ ] Define device registry record shape
- [ ] Seed generic, AirPods Pro 2, AirPods Pro 3, Sony WH-1000XM5, custom SOFA
- [ ] Add registry-backed label resolution in UI
- [ ] Add registry-backed capability notes
- [ ] Add BL-103 contract-only QA lane
- [ ] Add BL-103 execute lane

## Suggested Parallel Task Board

### Lane 1: Contract

Tasks:
- define `DiscoveryGraph` schema
- update scene/bridge contract docs
- align field names with BL-101 semantics

Can run in parallel with:
- initial UI stub work

### Lane 2: Runtime

Tasks:
- collect output candidates
- collect input candidates
- derive topology candidates
- publish ambiguity metadata

Depends on:
- contract names freezing first

### Lane 3: UI

Tasks:
- add discovery cards/rows
- add ambiguity/confirmation affordances
- keep speaker/headphone split intact

Depends on:
- minimal payload availability

### Lane 4: Registry

Tasks:
- define topology/device record tables
- migrate existing labels to registry reads
- add capability notes

Can start after:
- rough field names are known

### Lane 5: QA

Tasks:
- add BL-102 and BL-103 scripts
- add selftest scopes
- write contract assertions and execute assertions

Depends on:
- minimal runtime/UI surfaces landing

## Dependencies

Hard dependencies:
- BL-099 truth contract must stay authoritative for headphone verification and compensation language
- BL-101 source/provenance/freshness semantics must remain the one shared truth model

Soft dependencies:
- richer standalone input enumeration may require optional companion/native extension later
- custom layout support should wait until registry seed is stable

## Validation

Foundation checks:
- `cd Source/ui && npm run typecheck`
- `cd Source/ui && npm run build`
- `cmake --build build_local --config Release --target LocusQ_Standalone -j 8`

Truth checks:
- `./scripts/qa-bl101-calibrate-truthfulness-mac.sh --contract-only`
- `./scripts/qa-bl101-calibrate-truthfulness-mac.sh --execute --runs 3`
- `./scripts/qa-bl099-headphone-truthfulness-mac.sh --contract-only`
- `./scripts/qa-bl099-headphone-truthfulness-mac.sh --execute --runs 3`

New checks to add:
- `./scripts/qa-bl102-discovery-graph-mac.sh --contract-only`
- `./scripts/qa-bl102-discovery-graph-mac.sh --execute --runs 3`
- `./scripts/qa-bl103-topology-registry-mac.sh --contract-only`
- `./scripts/qa-bl103-topology-registry-mac.sh --execute --runs 3`

## Exit Criteria

Wave A is done when:
- CALIBRATE can show output/input/topology candidates
- ambiguous states are visible
- users can tell what still needs confirmation
- BL-102 lane proves the new truth surfaces

Wave B is done when:
- topology and device labels come from registries
- current supported modes are registry-backed
- unsupported or partial modes are described honestly
- BL-103 lane proves registry truth and capability copy

## Recommended Next Step

Start with one narrow implementation slice:

`AUTO-P1` + `AUTO-P2`

That means:
- freeze the `DiscoveryGraph` field schema
- publish output candidate nodes and topology candidates
- render one compact discovery summary in CALIBRATE

This is the smallest slice that visibly improves automation while preserving truthfulness.

## Sequencing Advice

Recommended sequence:
1. `BL-102` under the existing BL-101 umbrella
2. `BL-103`
3. `BL-104`
4. `BL-105`

Do not expand more layouts or richer headphone recipes before `BL-102` and `BL-103` are at least minimally landed, or the codebase will keep accumulating one-off logic in the panel and bridge.
