Title: BL-097 Editor bridge cadence tiering and calibration reload isolation
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19

# BL-097 Editor bridge cadence tiering and calibration reload isolation

## Plain-Language Summary

BL-097 in plain terms: make the editor/runtime bridge less chatty and less fragile by separating slow structural scene publication from fast diagnostics, by stopping raw timer polling from directly triggering heavy calibration reload work, and by removing unconditional production file I/O from routine WebView asset serving. Current state: In Validation. The local slice is now implemented: scene and calibration bridge updates publish on separate cadences, companion-profile polling is debounced, Steam Audio reload is staged through a pending-apply step, and resource-request logging is no longer always-on in production.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin operators, WebView/runtime maintainers, and performance/QA owners protecting UI responsiveness and host stability. |
| What is changing? | Scene-state publication moves to cadence tiers or dirty regions, calibration file observation becomes a staged/debounced apply path instead of direct timer-driven reload work, and resource-request logging becomes explicit diagnostics rather than always-on production behavior. |
| Why is this important? | The current message-thread path does too much work too often, which raises UI hitch, payload churn, callback-lock contention risk, and avoidable file-I/O pressure during WebView load/reload. |
| How will we deliver it? | Define the publication tiers and apply pipeline first, then reduce full-snapshot churn, then isolate calibration parsing/apply work from raw editor polling. |
| When is it done? | This item is done when unchanged scenes stop paying full 30 Hz serialization cost and calibration-driven renderer reloads are no longer direct timer side-effects. |
| Where is the source of truth? | This runbook, the 2026-03-17 review report, HX-05 payload budget guidance, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Quick scan of scope, priority, and dependencies. | `## Status Ledger` |
| Slice table | Separates cadence work from calibration apply-pipeline work. | `## Implementation Slices` |
| Payload/cadence evidence | Proves the bridge really got lighter rather than just moved around. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-097 |
| Priority | P1 |
| Status | Done |
| Track | B - Scene/UI Runtime |
| Effort | High / M |
| Depends On | HX-05 (Done), BL-059 (Done), BL-074 (Done) |
| Blocks | — |
| Default Replay Tier | T1 |
| Heavy Lane Budget | Standard |

## Objective

Reduce editor-thread cost and improve runtime ownership clarity in the WebView bridge. BL-097 is complete only when structural scene updates, high-frequency diagnostics, calibration profile application, and WebView asset-request diagnostics each have bounded, intentional cadences and thread boundaries.

## Source Inputs

- `Documentation/reviews/2026-03-17-comprehensive-code-dsp-review.md`
- `Documentation/reviews/2026-03-17-second-opinion-code-dsp-supplement.md`
- `Documentation/backlog/done/hx-05-payload-budget.md`
- `Documentation/backlog/done/bl-059-calibration-profile-integration-handoff.md`
- `Source/PluginEditor.cpp`
- `Source/editor_shell/EditorShellHelpers.h`
- `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`
- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `Source/editor_webview/EditorWebViewRuntime.h`

## Acceptance IDs

- `BL097-A1` Full-scene and full-calibration snapshots are not reserialized and pushed every 33 ms when no relevant state changed.
- `BL097-A2` Structural scene data, high-frequency diagnostics, and reactive/pose telemetry use explicit cadence tiers or dirty-region publication.
- `BL097-A3` Companion calibration observation/parsing occurs through a staged/debounced path rather than direct heavy work on every editor timer tick.
- `BL097-A4` Renderer teardown/reload is no longer a blunt side-effect of raw file polling and callback-lock contention windows are reduced.
- `BL097-A5` Payload/cadence evidence remains within HX-05-style budgets while existing WebView/runtime self-tests stay green.
- `BL097-A6` WebView resource-request logging is debug/diagnostic gated or otherwise bounded, and production asset serving no longer performs unbounded synchronous file writes per request.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Define and instrument the bridge cadence contract. | `Source/PluginEditor.cpp`, `Source/editor_shell/EditorShellHelpers.h`, `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`, evidence tooling | structural vs high-frequency publication paths are measurable |
| B | Reduce scene/calibration publication churn. | `Source/PluginEditor.cpp`, `Source/editor_shell/EditorShellHelpers.h`, `Source/processor_bridge/ProcessorSceneStateBridgeOps.h`, `Source/ui/src/index.ts` as needed | unchanged state no longer triggers full 30 Hz pushes |
| C | Isolate calibration file observation, parse, and apply/reload stages. | `Source/PluginEditor.cpp`, `Source/processor_core/ProcessorCalibrationBridge.cpp`, related runtime hooks | profile-driven reload work is staged/debounced and easier to reason about |
| D | Bound WebView resource-request diagnostics so asset serving stays lightweight in production. | `Source/editor_webview/EditorWebViewRuntime.h`, related runtime diagnostics controls | asset-request logging is opt-in or size-bounded and no longer adds silent message-thread I/O pressure |

## Latest Validation Snapshot

- 2026-03-19 cadence slice: editor bridge work now runs at tiered cadences instead of full scene + calibration serialization on every 30 Hz tick.
- Calibration profile polling is debounced to a lower cadence, and heavy runtime reload work is applied through a dedicated pending-reload step instead of direct poll-side reload.
- Scene and calibration updates can publish independently through split bridge helpers.
- WebView resource-request logging is now debug or env gated (`LOCUSQ_WEBVIEW_RESOURCE_LOG`) instead of always-on production file I/O.
- Current evidence:
  - `TestEvidence/bl097_editor_bridge_cadence_20260319T045626Z/status.tsv`
  - `TestEvidence/bl097_editor_bridge_cadence_20260319T045626Z/summary.md`
  - `TestEvidence/locusq_production_p0_selftest_20260319T045626Z.json`
  - `TestEvidence/locusq_production_p0_selftest_20260319T045636Z.json`
  - `TestEvidence/locusq_production_p0_selftest_20260319T045642Z.json`

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL097-BUILD | Automated | representative plugin build + `locusq_webui_typecheck` | build/typecheck stay green |
| BL097-CONTRACT | Automated | `scripts/qa-bl097-editor-bridge-cadence-mac.sh --contract-only` | payload tiers, dirty-region semantics, calibration apply stages, and resource-log gating are explicitly asserted |
| BL097-EXECUTE | Automated | `scripts/qa-bl097-editor-bridge-cadence-mac.sh --execute --runs 3` | cadence/payload metrics improve, asset-request logging stays bounded/off in production mode, and no new runtime regressions appear |
| BL097-SELFTEST | Automated | representative standalone/WebView self-test lane | existing boot/runtime behavior remains green |
| BL097-PROFILE-APPLY | Focused validation | companion profile add/remove/change smoke | no editor hitch regressions or blunt reload storms |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | contract/execute bridge cadence lane + focused self-test | cadence metrics + replay notes |
| Candidate intake | T2 | 5 | owner-selected bridge stress replay | replay summary + blocker taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected cadence/reload isolation set | owner packet + deterministic evidence |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-097 is the bridge/runtime hardening follow-on from the 2026-03-17 review set. It complements HX-05 payload-budget guidance and BL-074 WebView reliability work, but it focuses on the current full-snapshot churn, calibration-apply coupling, and additive production log-I/O pressure that those earlier items did not close.
