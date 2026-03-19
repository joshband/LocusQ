Title: BL-100 Desktop operator runner and evidence contract — audio-dsp-qa-harness
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-19

# BL-100 Desktop operator runner and evidence contract — audio-dsp-qa-harness

## Plain-Language Summary

BL-100 in plain terms: add a harness-owned macOS desktop robot layer that can launch apps, drive mouse and keyboard actions, capture screenshots or video, and emit machine-readable evidence so plugin repos stop hand-rolling fragile UI automation scripts. Done 2026-03-19: harness desktop operator module committed at `6cea7b95`, 50/50 ctest PASS, contract 22/22 + execute lane 3/3. Indexed into canonical backlog. For technical detail, see `## Objective` and `## Validation Plan`.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | QA maintainers for LocusQ, LocusQ Headtrack Companion, echoform, monument-reverb, memory-echoes, and future harness adopters that need desktop-level automation. |
| What is changing? | `audio-dsp-qa-harness` gains a reusable desktop operator layer for native app launch, input simulation, screenshot or video capture, crash harvesting, and structured evidence export. |
| Why is this important? | LocusQ already has useful selftests and host smoke scripts, but they remain repo-local, unevenly observable, and not easily reusable across plugins or companion apps. |
| How will we deliver it? | Build a macOS-first desktop operator module in the harness with a small action-plan DSL, deterministic retries and waits, structured artifact export, and fixture-backed tests. |
| When is it done? | When the harness can run at least one deterministic end-to-end desktop flow, emit status plus capture artifacts, classify failures clearly, and expose an adoption note for thin downstream adapters. |
| Where is the source of truth? | This runbook draft, future indexed backlog authority in `Documentation/backlog/index.md`, and implementation plus evidence in `audio-dsp-qa-harness`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast scan of ownership, priority, and dependencies. | `## Status Ledger` |
| Slice breakdown | Shows safe sequencing from operator core to downstream adoption. | `## Implementation Slices` |
| Validation table | Defines what “robot-ready” proof looks like. | `## Validation Plan` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-100 |
| Priority | P1 |
| Status | Done |
| Track | G - Tooling / Governance |
| Effort | Medium / M |
| Depends On | BL-082, BL-083, BL-084, BL-085 |
| Blocks | downstream standalone/companion/desktop-host robotic QA adoption lanes |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Create a harness-owned desktop operator layer for `audio-dsp-qa-harness` that supports deterministic desktop automation on macOS without baking plugin-specific knowledge into the harness core.

The first version should own:

1. **Desktop control primitives**
   - launch, activate, and quit apps
   - wait for process or window readiness
   - click, double-click, drag, type text, press key, press hotkey
   - optional menu navigation where stable

2. **Evidence capture**
   - screenshots
   - optional screen recording
   - action log
   - timing metrics
   - crash report path capture

3. **Machine-readable artifact contract**
   - `status.json`
   - `actions.tsv`
   - `captures_manifest.json`
   - `metrics.tsv`
   - `crash_report_paths.txt`

4. **Failure taxonomy**
   - `launch_failed`
   - `app_not_ready`
   - `interaction_failed`
   - `timeout`
   - `app_crashed`
   - `assertion_failed`
   - `evidence_missing`

5. **Adapter boundary**
   - keep app-specific selectors, workflows, and assertions downstream
   - keep the harness core focused on reusable operator actions, retries, waits, and evidence export

## Non-Goals

- Cross-platform parity in the first slice
- Vision-model autonomy or image-understanding agents in the harness core
- LocusQ adoption in the same upstream implementation lane
- BL-088 host backend work
- Replacing existing LocusQ selftests before the harness operator is proven

## Acceptance IDs

- `audio-dsp-qa-harness` contains a reusable desktop operator module
- A small action-plan DSL or equivalent structured plan format exists and is documented
- macOS backend can drive at least one deterministic fixture flow end-to-end
- screenshot capture is emitted and recorded in a manifest
- failure classification is machine-readable and stable
- operator evidence is suitable for CI triage even when video capture is disabled
- downstream adoption note explains how to keep `qa/main.cpp` or app-side glue thin

## Methodology Reference

- LocusQ standalone selftest reference: `scripts/standalone-ui-selftest-production-p0-mac.sh`
- LocusQ REAPER smoke reference: `scripts/reaper-headless-render-smoke-mac.sh`
- LocusQ companion install/sync reference: `scripts/sync-companion-app-mac.sh`
- Upstream foundation items: `Documentation/backlog/bl-082-qa-runner-app-library.md`, `Documentation/backlog/bl-083-runtime-config-contract.md`, `Documentation/backlog/bl-084-profiling-contract-hardening.md`, `Documentation/backlog/bl-085-cmake-integration-module.md`

## Implementation Slices

### S1 — Desktop operator core

Author a macOS-first operator module in `audio-dsp-qa-harness` with:
- app lifecycle control
- input simulation primitives
- waits, retries, and timeout helpers
- artifact directory management

### S2 — Evidence contract

Define and implement the canonical artifact schema:
- `status.json`
- `actions.tsv`
- `captures_manifest.json`
- `metrics.tsv`
- `crash_report_paths.txt`

Ensure failures classify cleanly and do not depend on humans parsing raw logs.

### S3 — Fixture-backed validation

Add deterministic tests that prove:
- action plans parse and execute
- timeout and retry paths behave predictably
- screenshots register in the capture manifest
- optional video capture registers honestly when enabled

### S4 — Adoption note

Write a short migration note for downstream repos describing:
- standalone app selftest hardening
- companion launch/sync verification
- host/manual blocker capture flows

## Validation Plan

Primary configure and test path:

```bash
cmake -S . -B build_bl100 -DBUILD_QA_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build_bl100 --parallel
ctest --test-dir build_bl100 --output-on-failure -R 'desktop_operator|qa_runner_app|test_suite'
ctest --test-dir build_bl100 --output-on-failure
git diff --check
```

Expected evidence contract:
- deterministic unit-test coverage for plan parsing, retries, and failure classes
- at least one integration-style operator smoke artifact bundle
- screenshot artifact present in the capture manifest
- honest status when optional video capture is unavailable or disabled

Gate criterion:
- upstream build and relevant tests PASS
- operator smoke emits the required machine-readable artifacts
- no plugin-specific logic is embedded into the harness core

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 3 | focused unit/integration reruns for the operator core | test output + validation matrix |
| Candidate intake | T2 | 5 | operator smoke replay at candidate depth | artifact bundles + taxonomy |
| Promotion | T3 | 10 or owner-approved equivalent | owner-selected promotion replay command set | owner packet + deterministic replay evidence |
| Sentinel | T4 | 20+ (explicit only) | long-run flake drill on fixture-backed operator flows | parity, flake, and crash artifacts |

### Cost/Flake Policy

- Diagnose a failing action index or wait condition before repeating full runs.
- Treat video capture as optional evidence in early slices so lack of recording support does not create false-red results.
- Document all operator environment assumptions, including Accessibility or AppleScript permissions, in lane notes.

## Handoff Return Contract

Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.

Additional field required at handoff:
- `UPSTREAM_HARNESS_COMMIT: <sha>` — the `audio-dsp-qa-harness` commit introducing the desktop operator module

## Governance Alignment (2026-03-17)

Canonical lifecycle and evidence rules are defined in:
- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)
- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)

This draft adds item-specific acceptance, artifact, and failure-taxonomy expectations for a future indexed backlog item.
