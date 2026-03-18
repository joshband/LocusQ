Title: CALIBRATE Automation and Headphone Personalization Research Packet
Document Type: Research Report
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# CALIBRATE Automation and Headphone Personalization Research Packet

## Executive Summary

The current `CALIBRATE` implementation already has the beginnings of a strong system, but the contracts are split across too many layers:

- host/bus output discovery and routing bootstrap
- companion-side headphone/personality acquisition
- plugin-side profile loading and fallback
- CALIBRATE UI diagnostics and automation copy
- replayable QA for some state transitions, but not full truthfulness

The most important conclusion from both repo evidence and external research is that `CALIBRATE` should not treat automation as one thing. It should explicitly separate:

1. device discovery
2. channel/topology inference
3. calibration measurement
4. correction/profile application
5. verification and provenance

That split will make output discovery more expandable, headphone personalization much richer, and UI trust claims easier to prove.

## Direct Answers

### Have we validated that `HEADPHONE DEVICE STATUS`, `AUTOMATION SUMMARY`, `CALIBRATION STATUS`, and `HEADPHONE VERIFY` are true and accurate?

Not end-to-end.

- `HEADPHONE DEVICE STATUS`: no dedicated automated lane was found that proves the rendered card matches live companion/device reality in `CALIBRATE`. The card is currently a render of `status.hpDeviceStatus` from the bridge payload, not an independently verified truth surface. See [ProcessorUiBridgeOps.h](/Users/artbox/Documents/Repos/LocusQ/Source/processor_bridge/ProcessorUiBridgeOps.h#L674) and [index.ts](/Users/artbox/Documents/Repos/LocusQ/Source/ui/src/index.ts#L15647).
- `AUTOMATION SUMMARY`: no dedicated truth lane was found. Current coverage is composition/rendering oriented, not source-of-truth verification. See [index.ts](/Users/artbox/Documents/Repos/LocusQ/Source/ui/src/index.ts#L8096) and [index.html](/Users/artbox/Documents/Repos/LocusQ/Source/ui/public/index.html#L2295).
- `CALIBRATION STATUS`: automated coverage exists for routing/state transitions and some deterministic chips, but not for full semantic truthfulness of dock/profile/summary copy. See [index.ts](/Users/artbox/Documents/Repos/LocusQ/Source/ui/src/index.ts#L15129) and [Documentation/testing/bl-026-calibrate-uiux-v2-qa.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/testing/bl-026-calibrate-uiux-v2-qa.md#L10).
- `HEADPHONE VERIFY`: this has the strongest automation, but the evidence is mostly schema/replay/governance oriented, not proof of perceptual truth. Repo review evidence explicitly says the current verification scores are synthetic constants rather than measured listening outcomes. See [Documentation/testing/bl-034-headphone-verification-qa.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/testing/bl-034-headphone-verification-qa.md#L9), [PluginProcessor.cpp](/Users/artbox/Documents/Repos/LocusQ/Source/PluginProcessor.cpp#L1175), and [2026-03-17-second-opinion-code-dsp-supplement.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/reviews/2026-03-17-second-opinion-code-dsp-supplement.md#L87).

### Can LocusQ support any speaker or headphone configuration in a more automated way?

Yes, but only if the system moves from hardcoded UI assumptions toward a registry-driven model:

- topology registry
- device capability registry
- output discovery layer
- calibration recipe layer
- profile storage/provenance layer
- analysis/verification layer

The current UI and bridge are still partly limited by `4` routable calibration channels and speaker-first assumptions. See [ProcessorUiBridgeOps.h](/Users/artbox/Documents/Repos/LocusQ/Source/processor_bridge/ProcessorUiBridgeOps.h#L38) and [index.ts](/Users/artbox/Documents/Repos/LocusQ/Source/ui/src/index.ts#L15234).

## Current State Audit

### Strengths already present in-repo

- Canonical profile structure already exists in [calibration-profile-schema-v1.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/plans/calibration-profile-schema-v1.md#L1).
- Companion profile acquisition and HRTF matching already exist in [bl-058-companion-profile-acquisition.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-058-companion-profile-acquisition.md#L1).
- Plugin-side handoff/runtime application already exists in [2026-02-27-calibration-system-design.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/plans/2026-02-27-calibration-system-design.md#L29).
- Deterministic validation-card state contracts already exist for part of `CALIBRATE` in [bl-026-calibrate-uiux-v2-qa.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/testing/bl-026-calibrate-uiux-v2-qa.md#L10).
- Deterministic verification/governance contracts already exist for headphone telemetry in [bl-034-headphone-calibration-verification.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-034-headphone-calibration-verification.md#L61).

### Structural gaps

1. Automation is under-modeled.
   `Redetect Routing`, `Automation Summary`, and `Device Status` are still too close to “status copy” and not explicit enough about discovery source, confidence, age, and override state.

2. Speaker calibration and headphone personalization are still too tightly coupled.
   These are different jobs with different automation inputs and different truth/provenance rules.

3. The topology model is not yet open-ended enough.
   The UI can name many layouts, but calibration routing/editability still has narrower operational limits than the naming suggests.

4. Truthfulness is incomplete.
   BL-099 exists because operator-visible verification and compensation surfaces can still overstate what is actually measured. See [bl-099-headphone-verification-truthfulness-and-compensation-provenance.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/bl-099-headphone-verification-truthfulness-and-compensation-provenance.md#L1).

## External Research Takeaways

### Output discovery and routing

- JUCE already exposes the main building blocks needed for standalone discovery: device name, input/output channel names, default input/output channel masks, active channel masks, latencies, and open-device setup state. See:
  - `juce::AudioIODevice` docs: <https://docs.juce.com/master/classjuce_1_1AudioIODevice.html>
  - `juce::AudioDeviceManager::AudioDeviceSetup` docs: <https://docs.juce.com/master/structjuce_1_1AudioDeviceManager_1_1AudioDeviceSetup.html>
- RtAudio’s probe example reinforces the same pattern: enumerate devices, inspect their capabilities, and re-query on hot-plug because device IDs are not permanent. See <https://caml.music.mcgill.ca/~gary/rtaudio/probe.html>.

Implication for LocusQ:

- standalone can and should do richer output/input discovery than it does now
- plugin-host mode should stay more conservative and host-truth driven
- automation rows should say whether the source is `host layout`, `standalone device scan`, `companion`, or `manual override`

### Headphone personalization

- `libmysofa` provides a practical model for loading SOFA data with sample-rate-aware access and tunable neighbor search resolution. See <https://github.com/hoene/libmysofa>.
- `AutoEq` demonstrates the scale and utility of a measurement-backed headphone correction catalog plus generation of EQ targets/results. See <https://github.com/jaakkopasanen/AutoEq>.
- `Impulcifer` is especially relevant: it treats speaker virtualization, headphone compensation, room measurement, plots, and channel-balance correction as separate but connected stages. It also supports stereo, 7.1, and single-speaker workflows by building larger topologies from repeatable sweep recordings. See <https://github.com/jaakkopasanen/Impulcifer>.
- `CamillaDSP` is a strong runtime reference for channel mixers plus IIR/FIR correction pipelines, including per-channel routing and long-FIR tradeoffs. See <https://github.com/HEnquist/camilladsp>.

Implication for LocusQ:

- headphone personalization should expand from “device status + profile active” into a full subsystem:
  - device identity
  - measurement provenance
  - HRTF subject/source
  - EQ/compensation chain
  - asymmetry handling
  - verification evidence
  - fallback reason

### Analysis and multi-pass validation

- `pyroomacoustics` shows the value of separating simulation/evaluation tools from live runtime code. It provides RIR generation, beamforming, DOA, and comparative algorithm testing. See <https://pyroomacoustics.readthedocs.io/>.
- `Impulcifer` again is a practical reference for multi-pass plots, room-specific versus generic measurements, and correction/plot artifacts that help explain what went wrong.

Implication for LocusQ:

- analysis should not just say `PASS/FAIL`
- it should produce machine-readable artifacts and operator-facing issue categories:
  - routing mismatch
  - polarity inversion
  - delay inconsistency
  - low SNR
  - unstable repeated sweep
  - headphone compensation missing
  - HRTF/profile fallback
  - left/right asymmetry
  - front/back confusion risk

## Recommended Architecture Changes

## 1. Introduce a Discovery Graph

Add a dedicated internal structure for discovered devices and inferred calibration targets:

```text
DiscoveryGraph
  outputs[]
    device_id
    device_name
    channel_names[]
    active_channel_mask
    default_channel_mask
    transport_type
    endpoint_class = speaker|headphones|unknown
    confidence = measured|detected|inferred
    source = host|standalone_scan|companion|manual
  inputs[]
    device_id
    device_name
    channel_names[]
    mic_capability = measurement|unknown
    preprocessing_state
  topology_candidates[]
    topology_id
    source
    confidence
    required_channels
    writable_channels
    limitations[]
```

Use it to drive:

- `Auto-map Outputs`
- `Auto-select Input`
- topology suggestions
- warning copy
- automation summary rows

## 2. Replace hardcoded layout handling with a topology registry

Create a canonical registry for:

- physical speaker layouts:
  - mono
  - stereo
  - quad
  - 5.1
  - 7.1
  - 7.1.2
  - 7.4.2
  - ambisonic orders
  - custom named layouts
- headphone/virtual targets:
  - stereo downmix
  - steam binaural
  - virtual binaural
  - custom SOFA render
- per-topology metadata:
  - required channels
  - speaker labels
  - canonical positions
  - visual layout schema
  - calibration recipe type
  - supported discovery strategies
  - supported verification strategies

This is the main enabler for “easy to add any speaker/headphone configuration.”

## 3. Expand Headphone Personalization into a first-class flow

Add a dedicated `Headphone Personalization` track with stages:

1. Identify device
2. Resolve profile source
3. Acquire personalization input
4. Build correction chain
5. Run verification
6. Save profile and provenance

Recommended new data fields:

- `device_identity`
  - vendor
  - model
  - transport
  - serial/hash if available and privacy-safe
- `profile_source`
  - bundled_measured
  - companion_estimated
  - user_imported_sofa
  - autoeq_derived
  - generic
- `measurement_provenance`
  - measured_by_user
  - vendor_reference
  - third_party_dataset
  - inferred_match
  - none
- `ear_profile`
  - left_embedding_hash
  - right_embedding_hash
  - frontal_embedding_hash
  - subject_id
  - sofa_ref
  - confidence
- `compensation_chain`
  - peq filters
  - fir ref
  - crossfeed
  - asymmetry compensation
  - headtracking enabled/ready/centered
- `verification`
  - objective metrics
  - perceptual metrics
  - provenance of each metric
  - last_run_time
  - stale_after

## 4. Make profile storage richer and more composable

Split profile persistence into layered parts:

- `DeviceProfile`
  static headphone or speaker template
- `EnvironmentProfile`
  room/output mapping/input selection for this setup
- `PersonalizationProfile`
  ear/HRTF/compensation personalization
- `VerificationProfile`
  objective and listening evidence

Then allow a saved calibration profile to reference these components rather than duplicating everything into one flat blob.

Benefits:

- easier addition of new headphones/speakers
- shared device libraries
- environment-specific speaker mappings
- multiple personalizations on one device
- less stale copy in status surfaces

## 5. Add multi-pass and multi-vector calibration analysis

For speakers:

- repeated sweep consistency check
- polarity/phase stability check
- inter-channel delay variance
- low-frequency modal instability note
- SNR and clipping detection
- mic-input preprocessing detection if possible
- seat-position variance across multiple mic positions

For headphones:

- left/right compensation asymmetry
- profile fallback detection
- HRTF confidence
- externalization estimate provenance
- front/back confusion screening
- headtracking ready/pose-stale/centered state
- repeated verification delta across runs

For both:

- produce issue labels and suggested next actions
- persist machine-readable analysis alongside the profile

## 6. Upgrade visuals and information organization

Recommended layout:

1. `Target`
   Speaker Room | Headphones
2. `Auto Discover`
   detected outputs, detected inputs, source, confidence, age, overrides
3. `Map and Review`
   visual topology map + editable routing + unsupported rows clearly labeled
4. `Measure / Personalize`
   workflow specific to speakers or headphones
5. `Analyze`
   issue summary, multi-pass consistency, improvement suggestions
6. `Save Profile`
   explicit provenance and portability info

New visual elements:

- topology map rendered from registry positions rather than fixed rows
- source/confidence chips:
  - `HOST`
  - `SCAN`
  - `COMPANION`
  - `MANUAL`
  - `MEASURED`
  - `ESTIMATED`
  - `GENERIC`
  - `STALE`
- analysis radar or issue table for:
  - response
  - delay
  - polarity
  - symmetry
  - externalization
  - tracking readiness

## Truthfulness QA Expansion

Create a dedicated lane, proposed as `BL-100` or folded into BL-099 follow-on execution:

### New lane goals

- prove every CALIBRATE truth surface carries a provenance state
- prove UI copy never upgrades `estimated` or `generic` into `measured`
- prove automation summary rows identify their source
- prove stale companion/device data is labeled stale
- prove headphone verify metrics are marked unavailable when no real evidence exists

### Proposed automated checks

1. `CAL-AUTO-001`
   `HEADPHONE DEVICE STATUS` matches a known bridge payload fixture and preserves source/provenance labels.
2. `CAL-AUTO-002`
   `AUTOMATION SUMMARY` rows show source = host/scan/companion/manual for every auto-populated field.
3. `CAL-AUTO-003`
   `CALIBRATION STATUS` dock reflects routing/profile/run state without inventing readiness.
4. `CAL-AUTO-004`
   `HEADPHONE VERIFY` shows `UNAVAILABLE` or `ESTIMATED` when scores are synthetic.
5. `CAL-AUTO-005`
   stale payloads visibly degrade to `STALE` or `OUT OF DATE`.
6. `CAL-AUTO-006`
   requested vs active vs fallback surfaces remain consistent across all cards.
7. `CAL-AUTO-007`
   profile provenance is preserved after save/load/export/import.

### Recommended evidence artifacts

- `status.tsv`
- `truth_matrix.tsv`
- `payload_fixture_manifest.json`
- `ui_snapshot_manifest.json`
- `provenance_mismatch.tsv`
- `copy_claims_audit.md`

## Suggested Backlog Waves

### Wave A: Truth and discovery foundation

- add discovery graph
- add provenance enums
- rename `Redetect Routing` to `Auto-map Outputs`
- add `Auto-select Input`
- add source/confidence/age rows to automation summary
- complete BL-099 truthfulness alignment in CALIBRATE

### Wave B: Registry and visualization

- topology registry
- visual topology renderer
- custom layout import/schema
- extensible speaker/headphone device catalog

### Wave C: Headphone personalization expansion

- richer companion acquisition contract
- multi-profile personalization
- SOFA import and provenance
- left/right asymmetry support
- objective plus perceptual verification write-back

### Wave D: Analysis and improvement engine

- repeated-pass analysis
- multi-position speaker measurement
- issue diagnosis and recommendations
- calibration comparison and upgrade suggestions

## Concrete Next Steps

1. Treat BL-099 as mandatory gating for CALIBRATE trust surfaces, not just renderer diagnostics.
2. Add a new `CALIBRATE discovery/provenance contract` doc that becomes the source of truth for automation fields.
3. Introduce a topology/device registry before adding more layouts or headphones.
4. Build a dedicated truthfulness selftest scope for `HEADPHONE DEVICE STATUS`, `AUTOMATION SUMMARY`, `CALIBRATION STATUS`, and `HEADPHONE VERIFY`.
5. Expand headphone personalization from status-only into acquisition, correction, verification, and provenance.

## Sources

### Repo sources

- [2026-02-27-calibration-system-design.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/plans/2026-02-27-calibration-system-design.md)
- [calibration-profile-schema-v1.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/plans/calibration-profile-schema-v1.md)
- [bl-034-headphone-calibration-verification.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-034-headphone-calibration-verification.md)
- [bl-058-companion-profile-acquisition.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/done/bl-058-companion-profile-acquisition.md)
- [bl-099-headphone-verification-truthfulness-and-compensation-provenance.md](/Users/artbox/Documents/Repos/LocusQ/Documentation/backlog/bl-099-headphone-verification-truthfulness-and-compensation-provenance.md)
- [PluginProcessor.cpp](/Users/artbox/Documents/Repos/LocusQ/Source/PluginProcessor.cpp#L1087)
- [ProcessorUiBridgeOps.h](/Users/artbox/Documents/Repos/LocusQ/Source/processor_bridge/ProcessorUiBridgeOps.h)
- [index.ts](/Users/artbox/Documents/Repos/LocusQ/Source/ui/src/index.ts)

### External sources

- JUCE `AudioIODevice`: <https://docs.juce.com/master/classjuce_1_1AudioIODevice.html>
- JUCE `AudioDeviceManager::AudioDeviceSetup`: <https://docs.juce.com/master/structjuce_1_1AudioDeviceManager_1_1AudioDeviceSetup.html>
- RtAudio probe/device enumeration: <https://caml.music.mcgill.ca/~gary/rtaudio/probe.html>
- `libmysofa`: <https://github.com/hoene/libmysofa>
- `AutoEq`: <https://github.com/jaakkopasanen/AutoEq>
- `CamillaDSP`: <https://github.com/HEnquist/camilladsp>
- `Impulcifer`: <https://github.com/jaakkopasanen/Impulcifer>
- `pyroomacoustics`: <https://pyroomacoustics.readthedocs.io/>

## Validation Status

`not tested` for runtime behavior. This packet is based on repo-document/code audit plus web/GitHub research.
