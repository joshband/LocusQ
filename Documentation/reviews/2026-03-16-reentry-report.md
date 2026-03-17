Title: LocusQ Re-Entry Report — 2026-03-16
Document Type: Review Report
Author: APC Codex
Created Date: 2026-03-16
Last Modified Date: 2026-03-16

# LocusQ Re-Entry Report — 2026-03-16

## Purpose

Provide a current-state snapshot and actionable re-entry path after a multi-week gap.
Serves as the starting-point document for resuming active work as of 2026-03-16.

## Related Documents

- Prior review: [`2026-03-07-comprehensive-code-review.md`](2026-03-07-comprehensive-code-review.md)
- Backlog authority: [`Documentation/backlog/index.md`](../backlog/index.md)
- Execution wave plan: [`Documentation/reports/2026-03-01-execution-wave-triage.md`](../reports/2026-03-01-execution-wave-triage.md)

---

## Status Legend

| Tag | Meaning |
|---|---|
| `[DONE]` | Closed and archived; evidence complete |
| `[ACTIVE]` | Currently in implementation or validation |
| `[NEXT]` | Highest-priority ready item for the next session |
| `[QUEUED]` | Ready once its direct predecessor closes |
| `[BLOCKED]` | Waiting on an open dependency |
| `[DEFERRED]` | Conditional or lower-priority; no immediate pressure |

---

## Project Snapshot

| Field | Value |
|---|---|
| Version | v1.0.0-ga |
| Active phase | `code` |
| UI framework | `webview` |
| Last commit | 2026-03-07 — "Commit March 7 session updates and validation artifacts" |
| Working tree | **Dirty** — uncommitted changes across `Source/`, `companion/`, `scripts/`, `Documentation/` |
| Test evidence | Multiple new `TestEvidence/bl05X_*` and `bl059_*` directories untracked |

---

## Priority Snapshot

| Priority | Open / Active Count | IDs |
|---|---|---|
| P0 | 2 active | BL-058, BL-059 |
| P1 | 5 active or queued | BL-053, BL-054, BL-055, BL-056, BL-080 |
| P2 | 4 open | BL-020, BL-021, BL-032 (done-candidate), BL-079 (in validation) |
| P3 | 1 open | BL-080 (authoring undo/redo, self-test failing) |

---

## Backlog Progress Snapshot

### P0 — Release Blockers

| ID | Title | Status | Runbook | Blocker / Notes |
|---|---|---|---|---|
| BL-058 | Companion profile acquisition UI + HRTF matching | [ACTIVE] | [bl-058](../backlog/bl-058-companion-profile-acquisition.md) | Core implementation landed (3-view matcher, CalibrationProfile write path, EarPhotoMatcher tests green). **Manual runtime evidence still pending** before owner promotion. |
| BL-059 | CalibrationProfile integration handoff | [ACTIVE] | [bl-059](../backlog/bl-059-calibration-profile-integration-handoff.md) | Fixture-driven contract+execute lane PASS. BL-053 and BL-055 dependency replays PASS. **Owner promotion packet pending.** |

### P1 — Critical Feature / Dependency-Blocking

| ID | Title | Status | Runbook | Blocker / Notes |
|---|---|---|---|---|
| BL-053 | Head tracking orientation injection | [ACTIVE] | [bl-053](../backlog/bl-053-head-tracking-orientation-injection.md) | Structural lane + T1 PASS; manual operator sync evidence captured. Owner promotion packet pending. Blocks BL-059. |
| BL-054 | PEQ cascade RT integration | [ACTIVE] | [bl-054](../backlog/bl-054-peq-cascade-rt-integration.md) | Atomic preset publish path landed; contract+execute lane + native build PASS. Blocks BL-056. |
| BL-055 | FIR convolution engine | [ACTIVE] | [bl-055](../backlog/bl-055-fir-convolution-engine.md) | C4/C6 remediation landed; execute replay PASS 2026-03-07. Blocks BL-056. |
| BL-056 | Calibration state migration + latency contract | [BLOCKED] | [bl-056](../backlog/bl-056-calibration-state-migration-latency.md) | Blocked on BL-054 + BL-055 owner promotion. Then unblocks BL-059. |
| BL-067 | AUv3 app-extension lifecycle and host validation | [BLOCKED] | [bl-067](../backlog/bl-067-auv3-app-extension-lifecycle-and-host-validation.md) | BL-073 gate satisfied (Done). No execute evidence `TODO` rows remain. Ready when bandwidth allows. |
| BL-068 | Temporal effects core (delay/echo/looper) | [BLOCKED] | [bl-068](../backlog/bl-068-temporal-effects-delay-echo-looper-frippertronics.md) | BL-073 gate satisfied. Blocked on BL-050 (Done) + BL-055 promotion. |
| BL-080 | Authoring undo/redo for timeline and preset operations | [ACTIVE] | [bl-080](../backlog/bl-080-authoring-undo-redo-for-timeline-and-preset-operations.md) | W3-A landed. **Production standalone self-test fails on CALIBRATE topology/legacy-alias timeout** before `UI-W3A-01`/`UI-W3A-02` execute. Needs targeted debug. |

### P1 — Done-Candidates (Owner Promotion Pending)

| ID | Title | Status | Runbook | Notes |
|---|---|---|---|---|
| BL-032 | Source modularization of PluginProcessor/PluginEditor | [ACTIVE] | [bl-032](../backlog/bl-032-source-modularization.md) | F1 done-promotion PASS, but `Source/PluginProcessor.cpp` 3653 lines > 3600 guardrail. RT audit PASS. Needs one targeted trim pass. |
| BL-039 | Parameter relay spec generation | [ACTIVE] | [bl-039](../backlog/bl-039-parameter-relay-spec-generation.md) | Z10 owner D2 accepted; 100/100 parity green. Owner promotion packet pending. |
| BL-040 | UI modularization and authority status UX | [ACTIVE] | [bl-040](../backlog/bl-040-ui-modularization-and-authority-status.md) | Z10 owner D2 accepted; 100-run diagnostics green. Owner promotion packet pending. |

### P2 — In Progress / Open

| ID | Title | Status | Runbook | Notes |
|---|---|---|---|---|
| BL-020 | Confidence/masking overlay mapping | [ACTIVE] | [bl-020](../backlog/bl-020-confidence-masking.md) | C4 mode parity + exit semantics green. Owner promotion review pending. |
| BL-021 | Room-story overlays | [ACTIVE] | [bl-021](../backlog/bl-021-room-story-overlays.md) | C2 soak PASS; N13 owner recheck PASS. |
| BL-079 | APVTS parameter grouping and host hierarchy | [ACTIVE] | [bl-079](../backlog/bl-079-apvts-parameter-grouping-and-host-hierarchy.md) | W1-D implementation landed; build + parity checks PASS. Promotion follow-up pending clean checkout. |
| BL-078 | Runtime finite-output enforcement and diagnostics | [QUEUED] | [bl-078](../backlog/bl-078-runtime-finite-output-enforcement-and-diagnostics.md) | Follow-on from BL-036 closeout. BL-036 Done; no immediate blockers. |

### P1/P2 — Conditional / Downstream

| ID | Title | Status | Notes |
|---|---|---|---|
| BL-060 | Phase B listening test harness + evaluation | [BLOCKED] | Blocked on BL-059 + BL-071 (Done) + BL-072 (Done). |
| BL-061 | HRTF interpolation + crossfade | [DEFERRED] | Conditional on BL-060 gate pass. |

---

## Uncommitted Working Tree — Action Required

The following files are modified but uncommitted since the March 7 session. Reviewing and committing these is **step zero** before resuming any BL lane work.

### Source (C++)
- `Source/HeadPoseInterpolator.h`
- `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`
- `Source/SpatialRenderer.h`
- `Source/headphone_dsp/HeadphoneCalibrationChain.h`, `HeadphonePeqHook.h`
- `Source/processor_core/ProcessorCalibrationBridge.cpp`
- `Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp`, `SpatialSteamAudioBackend.cpp`

### Companion (Swift)
- `companion/Sources/LocusQHeadTrackerCore/CalibrationProfile.swift`
- `companion/Sources/LocusQHeadTrackerCore/EarPhotoMatcher.swift`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`

### Scripts / QA
- `scripts/sync-companion-app-mac.sh`
- `scripts/qa-bl011-clap-closeout-mac.sh`, `qa-bl053-*`, `qa-bl055-*`, `qa-bl058-*`, `qa-bl059-*`, `qa-bl069-*`

### Untracked (need explicit add or .gitignore decision)
- `Documentation/reviews/2026-03-07-comprehensive-code-review.md` ← this review doc
- `TestEvidence/bl053_*`, `bl054_*`, `bl059_*` directories
- `companion/Tests/LocusQHeadTrackerTests/EarPhotoMatcherTests.swift`
- `scripts/qa-bl054-peq-cascade-rt-integration-mac.sh`

---

## Open Technical Debt (Carried From 2026-03-07 Review)

Full detail: [`2026-03-07-comprehensive-code-review.md`](2026-03-07-comprehensive-code-review.md)

### High Severity

| # | Finding | Status | Source Location |
|---|---|---|---|
| 1 | Head-pose clock mismatch — companion uses epoch ms, interpolator uses `getMillisecondCounterHiRes()` | Partially addressed by `059920ba` — **verify unification is complete** | `Source/HeadPoseInterpolator.h:67-96`, `PluginProcessor.cpp:1639-1643` |
| 2 | Stale pose never cleared on disconnect — main render path has no freshness gate | Open | `PluginProcessor.cpp:1639-1662` vs. `2212-2238` |
| 3 | CalibrationProfile FIR/SOFA not wired into DSP — FIR mode enables but loads identity taps; `sofa_ref` schema key mismatches | Open | `ProcessorCalibrationBridge.cpp:1245-1258`, `SpatialSteamAudioBackend.cpp:273-300` |
| 4 | Companion CDN fallback for Three.js violates BL-058 privacy/offline contract | Open | `companion/main.swift:1204-1229`, `sync-companion-app-mac.sh:64-67` |
| 5 | BL-011 CLAP closeout script can false-green against stale artifacts | Open | `scripts/qa-bl011-clap-closeout-mac.sh:162-181` |

### Medium Severity

| # | Finding | Status |
|---|---|---|
| 6 | Linux CI does not build plugin formats despite architecture claim | Open |
| 7 | AUv3 missing from release governance workflow | Open |
| 8 | UI PR gate validates Stage 12, not production UI | Open |
| 9 | Bridge promise leak on timeout | Open |
| 10 | 30 Hz malformed JSON reparse on companion calibration file failures | Open |

---

## Recommended Re-Entry Path

Work items are sequenced by dependency order, P0-first, and grouped into sessions of roughly bounded scope.

### Session 0 — Working Tree Hygiene (Do First, ~30 min)

- [ ] `git diff` all modified source files; confirm each change is intentional
- [ ] Stage and commit the March 7 session remnants: test evidence directories, EarPhotoMatcherTests.swift, QA scripts, companion/source changes
- [ ] Add `2026-03-07-comprehensive-code-review.md` and this file to a docs commit
- [ ] Confirm no orphaned partial work in `Source/` that contradicts a green runbook lane

### Session 1 — Verify Clock Fix and Close BL-058 (P0)

1. [NEXT] Verify `059920ba` fully unifies the head-pose time base:
   - Confirm companion sends monotonic-equivalent timestamps
   - Add a telemetry/assertion guard for impossible deltas
2. [NEXT] Capture manual runtime evidence for BL-058:
   - Run companion profile acquisition flow end-to-end
   - Capture `TestEvidence/bl058_*` promotion evidence bundle
   - Generate owner promotion packet per `_template-promotion-decision.md`

### Session 2 — Owner-Promote BL-053, BL-055, BL-059 (P0/P1)

3. [QUEUED] Promote BL-053: promotion packet + `TestEvidence/bl053_owner_*`
4. [QUEUED] Promote BL-055: promotion packet + `TestEvidence/bl055_owner_*`
5. [QUEUED] Promote BL-059: owner promotion packet (lane evidence already captured 2026-03-07)
6. Sync `Documentation/backlog/index.md`, `status.json`, `TestEvidence/validation-trend.md`

### Session 3 — Unblock and Close BL-056 (P1)

7. [QUEUED after BL-054/BL-055 promotion] Implement BL-056 calibration state migration + latency contract
   - Runbook: `Documentation/backlog/bl-056-calibration-state-migration-latency.md`
   - Annex: `Documentation/plans/2026-02-27-calibration-system-design.md`

### Session 4 — BL-059 Downstream and Stale-Pose Gating

8. [QUEUED after BL-059 Done] Start BL-060 Phase B listening test harness
9. Address stale-pose never-cleared finding (Finding #2 above) — add freshness gate to main render path to match behavior already in calibration monitoring path

### Session 5 — Companion Packaging Fix (P0 Quality Risk)

10. Remove CDN fallback in companion `main.swift`; bundle pinned Three.js artifact from npm toolchain
11. Update `sync-companion-app-mac.sh` to copy from built `Source/ui/` output, not the deleted `three.min.js` path
12. Add startup assertion: no network calls on companion init

### Session 6 — Wire FIR/SOFA Into DSP (Before Calibration Ship Claims)

13. Wire profile FIR taps into `HeadphoneFirHook::loadImpulseResponse()`
14. Fix `sofa_ref` schema lookup to match the actual CalibrationProfile JSON structure
15. Close the Steam Audio SOFA swap TODO in `SpatialSteamAudioBackend.cpp`

### Session 7 — Promote Done-Candidates

16. BL-032: trim `PluginProcessor.cpp` to satisfy `< 3600` LOC guardrail; re-run F1 promotion
17. BL-039: owner promotion packet
18. BL-040: owner promotion packet
19. BL-079: verify clean-checkout build; owner promotion packet

### Deferred (No Immediate Pressure)

- BL-020, BL-021: R&D expansion lanes; can resume after P0/P1 chain closes
- BL-067, BL-068: AUv3 + temporal effects; BL-073 gate satisfied, resume when bandwidth allows
- BL-060/BL-061: downstream of BL-059 Done
- BL-078: runtime finite-output enforcement; no hard blocker but no immediate gate

---

## Validation Status

- `not tested` — This document is a static review and re-entry plan only.
- Refer to `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md` for last live evidence.
- Run `./scripts/validate-docs-freshness.sh` before resuming BL lane work to confirm governance state.
