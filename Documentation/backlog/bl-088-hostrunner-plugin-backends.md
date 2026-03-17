Title: BL-088 HostRunner Plugin Backends (VST3/AU) — audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-088 HostRunner Plugin Backends (VST3/AU) — audio-dsp-qa-harness

## Plain-Language Summary

BL-088 in plain terms: Implement concrete VST3 and AU host backends for the `HostRunner` skeleton in `audio-dsp-qa-harness` so all four plugins can run format-native validation (parameter automation, plugin lifecycle, MIDI routing) inside the harness framework rather than relying on external host/pluginval wrappers. Current state: Open (deferred P2 — prototype on LocusQ once BL-082/BL-083/BL-084 are stable). For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin QA engineers who need to validate plugin format–native behavior (VST3 parameter automation, AU app-extension lifecycle, MIDI routing) in CI. |
| What is changing? | `HostRunner` skeletons in the harness become concrete implementations backed by JUCE `PluginInstance` API; plugins can write `HostRunnerScenario` tests that load the actual compiled plugin binary and validate its behavior as a host would see it. |
| Why is this important? | Current pluginval and DAW manual validation are opaque and non-deterministic. Harness-native host backends would give reproducible, CI-integrated format validation without external tool dependency. |
| How will we deliver it? | Prototype VST3 backend using JUCE `AudioPluginInstance` + `PluginDescription`; validate on LocusQ VST3 binary; then generalize for AU; document plugin adoption pattern. |
| When is it done? | When LocusQ can run at least one `HostRunnerScenario` (parameter automation round-trip + state save/restore) through the VST3 backend in CI with deterministic pass/fail. |
| Where is the source of truth? | Runbook `Documentation/backlog/bl-088-hostrunner-plugin-backends.md`, backlog authority `Documentation/backlog/index.md`, and evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |
| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-088 |
| Priority | P2 |
| Status | Open (deferred; start after BL-082, BL-083, BL-084 are Done) |
| Track | G - Tooling / Governance |
| Effort | Large / L |
| Depends On | BL-082 (runner app library), BL-083 (runtime-config contract), BL-084 (profiling contract) |
| Blocks | — |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Heavy wrapper containment required (binary launch per scenario run) |

## Objective

Implement two concrete `HostRunner` backends in `audio-dsp-qa-harness`:

### VST3 Backend (`runners/vst3_plugin_host.h`)
- Load a compiled VST3 binary by path using JUCE `KnownPluginList` + `AudioPluginInstance`
- Initialize with configurable `sample_rate`, `block_size`, `num_channels`
- Support: parameter get/set, state save/restore, MIDI note injection, audio buffer processing
- Report: parameter value round-trip correctness, state hash stability, audio output finite-ness

### AU Backend (`runners/au_plugin_host.h`) — macOS only
- Load AU component by type/subtype/manufacturer codes
- Same initialization and validation surface as VST3 backend
- Requires code-signed plugin or entitlement exemption for CI (document requirement)

### `HostRunnerScenario` scenario type
- New scenario JSON schema: `"type": "host_runner"`, fields: `plugin_path`, `format` (`vst3`|`au`), `parameter_automation_steps`, `state_roundtrip_count`, `expected_output_finite`
- Execution: `HostRunner` loads plugin, runs automation + state steps, captures results, evaluates invariants

## Acceptance IDs

- `audio-dsp-qa-harness` contains `runners/vst3_plugin_host.h` with documented public API
- LocusQ can run a `HostRunnerScenario` (VST3, parameter automation + state save/restore) in CI with deterministic PASS
- `HostRunnerScenario` JSON schema is documented in harness `scenarios/README.md`
- Audio output from HostRunner execution passes finite-output check (no NaN/Inf)
- AU backend implementation is gated behind `#if JUCE_MAC` or equivalent
- Existing `DspUnderTest`-based scenarios are unaffected (no regression)

## Risks

- **High**: JUCE `PluginInstance` API surface is broad; host sandbox + signing requirements on macOS may require entitlement changes
- **Medium**: VST3 host validation requires a built plugin binary; CI must produce the binary before the HostRunner test runs (ordering dependency)
- **Low**: MIDI routing semantics differ between VST3 and AU; may need per-format scenario variants

## Methodology Reference

- BL-088 origin analysis: `Documentation/archive/2026-02-25-research-legacy/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- LocusQ skeleton reference: `qa/main.cpp` (HostRunner scaffolding, lines 179–230, TODO markers)
- AUv3 lifecycle context: BL-067 runbook (`Documentation/backlog/done/bl-067-auv3-plugin-lifecycle.md`)

## Implementation Slices

### S1 — Design `HostRunnerScenario` schema and execution contract
Define JSON schema, document invariant types, specify initialization contract. No implementation yet.

### S2 — VST3 backend prototype on LocusQ
Implement `Vst3PluginHost` using JUCE API. Hardcode LocusQ binary path for initial prototype. Validate parameter round-trip + state hash on macOS.

### S3 — Generalize VST3 backend
Make binary path configurable via scenario JSON and `QA_PLUGIN_PATH` env var. Add CI step to `qa_harness.yml` that passes built binary path.

### S4 — AU backend (macOS)
Implement `AuPluginHost`. Gate behind `JUCE_MAC`. Document entitlement/signing requirements.

### S5 — Backport documentation
Document adoption pattern for echoform/memory-echoes/monument-reverb in harness `runners/README.md`.

## Validation Plan

QA harness script: `scripts/qa-bl088-hostrunner-backends-mac.sh` (to be authored in S3).
Evidence schema: `TestEvidence/bl088_*/status.tsv`.

Gate criteria:
- LocusQ VST3 `HostRunnerScenario` runs deterministically in CI: PASS
- Parameter automation round-trip error < 1e-6
- State hash stable across 3 consecutive save/restore cycles
- Audio output finite-output check: PASS

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | runbook primary lane command at dev-loop depth | validation matrix + replay summary |
| Candidate intake | T2 | 2 (heavy-wrapper cap: binary launch per run) | runbook candidate replay command set | contract/execute artifacts + taxonomy |
| Promotion | T3 | 3 (heavy-wrapper owner-approved) | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run sentinel drill when explicitly requested | parity/sentinel artifacts |

### Cost/Flake Policy

- This item uses heavy-wrapper containment: binary launch per scenario run is expensive.
- Cap T2 at 2 runs, T3 at 3 runs unless owner explicitly requests broader coverage.
- Diagnose failing run index before repeating sweeps.
- Document cadence overrides with rationale in `lane_notes.md` or `owner_decisions.md`.

## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Additional fields required at handoff:
- `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit introducing `runners/vst3_plugin_host.h`
- `PLUGIN_BINARY_PATH: <path>` — the LocusQ VST3 binary used for validation evidence

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This runbook lists only item-specific exceptions or additions.
