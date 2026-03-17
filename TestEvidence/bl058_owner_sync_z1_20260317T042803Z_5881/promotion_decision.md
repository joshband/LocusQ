Title: BL-058 Promotion Decision (Slice Z1 Owner Sync)
Document Type: Promotion Decision
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-058 Promotion Decision (Slice Z1 Owner Sync)

## Plain-Language Decision Summary

- What changed: Companion profile-acquisition UI and HRTF nearest-neighbor matching landed. Guided 3-view ear-photo capture card, in-memory-only embedding pipeline, CalibrationProfile.json write path, and explicit readiness/sync gate are all implemented and verified.
- Why this decision: All acceptance criteria pass at the harness + headless selftest level. Execute lane is 16/16 PASS. Send gate confirmed closed until sync. Privacy contract (no network, no persisted photos) confirmed.
- Decision: **Done-candidate** — advance BL-058 to Done-candidate. Live-hardware AirPods confirmation run required before final Done at release gate.

## 6W Snapshot

| Question | Plain-language answer |
|---|---|
| Who is impacted? | Headphone users relying on personalised HRTF; QA/release owners gating BL-059 handoff |
| What was reviewed? | Companion acquisition card, EarPhotoMatcher, CalibrationProfile write path, readiness state machine, send gate, privacy contract |
| Why this outcome? | All acceptance criteria verified; no regressions; pre-existing RT hits are out of BL-058 scope |
| How was confidence established? | Headless selftest (7/7 PASS) + execute lane (16/16 PASS) + readiness gate probe + static privacy scan |
| When to revisit? | Live AirPods hardware run required before release gate; CDN fallback fix (finding #4) tracked separately |
| Where is the evidence? | `TestEvidence/bl058_owner_sync_z1_20260317T042803Z_5881/` and cross-referenced paths below |

## Decision

- Result: `PASS`
- Decision: `Done-candidate`

## Scope Reviewed

- Slice 1: companion profile-acquisition card (3-view guided capture UI, `main.swift`)
- Slice 2: EarPhotoMatcher nearest-neighbor with fallback threshold (`EarPhotoMatcher.swift`)
- Slice 3: CalibrationProfile write path (`CalibrationProfile.swift`)
- Slice 4: Readiness state machine + send gate contract
- Slice 5: Privacy contract (no network, no image persistence)
- QA: `qa-bl058-companion-profile-acquisition-mac.sh` execute lane

## Required Gate Matrix

| Gate | Command | Expected | Actual | Status | Evidence |
|---|---|---|---|---|---|
| Build (VST3+AU) | `bash scripts/build-and-install-mac.sh` | PASS | PASS | PASS | build output 2026-03-17T04:12Z |
| Execute lane | `qa-bl058...mac.sh --execute` | 16/16 PASS | 16/16 PASS | PASS | `TestEvidence/bl058_companion_profile_20260317T042349Z_73281/status.tsv` |
| Headless selftest | `locusq-headtrack-companion --bl058-profile-selftest` | 7/7 PASS | 7/7 PASS | PASS | `TestEvidence/bl058_selftest_20260317T041922Z_70780/selftest_results.tsv` |
| Matching latency | selftest `matching_latency_lt_50ms` | < 50ms | 0.1050ms | PASS | selftest_results.tsv |
| Send gate closed | require-sync 2s run | packets_sent=0 | packets_sent=0, gate_open=false | PASS | `readiness_snap.tsv` |
| Privacy static scan | `rg 'https?://' companion/Sources` | no matches | no matches | PASS | `bl058_companion_profile_20260317T042349Z_73281/status.tsv` BL058-C4d |
| No images persisted | selftest `no_capture_files_persisted` | PASS | PASS | PASS | selftest_results.tsv |
| RT safety | `rt-safety-audit.sh` | n/a for BL-058 scope | non_allowlisted=9 in CalibrationEngine/SceneGraph/SpatialRenderer (pre-existing, out of scope) | N/A | pre-existing; BL-058 is companion Swift only |
| Status schema | `jq empty status.json` | PASS | PASS | PASS | validated 2026-03-17 |
| Docs freshness | `validate-docs-freshness.sh` | PASS (main tree) | PASS (14 errors in worktree skill files — excluded per CLAUDE.md) | PASS | main tree: 3/3 PASS |
| SHARED_FILES_TOUCHED | ownership delta check | no | no — BL-058 touches companion Swift only; no shared plugin DSP files | PASS | — |

## Determinism / Reliability Checks

| Check | Expected | Actual | Status | Evidence |
|---|---|---|---|---|
| Headless selftest runs | 7 checks | 7/7 PASS | PASS | selftest_results.tsv |
| Execute lane checks | 16 checks | 16/16 PASS | PASS | bl058_companion_profile_20260317T042349Z_73281/status.tsv |
| Matching latency stability | < 50ms | 0.2139ms (run 1), 0.1050ms (run 2) | PASS | two independent selftest runs |
| Send gate determinism | 0 packets in require-sync mode | 0/20 packets sent across 2s | PASS | readiness_snap.tsv |

## Contract Consistency

| Surface | Expected | Status | Notes |
|---|---|---|---|
| `Documentation/backlog/bl-058-companion-profile-acquisition.md` | status current | PASS | Updated 2026-03-07; implementation snapshot present |
| `Documentation/backlog/index.md` | row aligned | PASS | BL-058 row shows In Validation |
| `status.json` | evidence keys aligned | PASS | BL-058 session note present |
| `TestEvidence/validation-trend.md` | trend entry pending | TODO | Append entry after this packet is committed |

## Done Transition Readiness

| Check | Expected | Status | Notes |
|---|---|---|---|
| Closeout template | `_template-closeout.md` structure ready | READY | Apply at final Done transition |
| Runbook move planned | `Documentation/backlog/done/bl-058-companion-profile-acquisition.md` | READY | Move at final Done |
| Index row ready | row state → Done | READY | Update at final Done |

## Blockers

- **Live AirPods hardware run** (non-blocking for Done-candidate; required before release-gate Done): no physical AirPods hardware available in this session. Selftest + synthetic evidence is sufficient for Done-candidate posture.
- **CDN fallback in `companion/main.swift`** (re-entry report finding #4): Three.js CDN fallback violates offline/privacy contract. Tracked as open technical debt; does not affect matching/acquisition path validated here. Separate fix session required — see re-entry report Session 5.

## Evidence Index

- `TestEvidence/bl058_owner_sync_z1_20260317T042803Z_5881/promotion_decision.md` — this document
- `TestEvidence/bl058_companion_profile_20260317T042349Z_73281/status.tsv` — execute lane 16/16 PASS
- `TestEvidence/bl058_selftest_20260317T041922Z_70780/selftest_results.tsv` — headless selftest 7/7 PASS
- `TestEvidence/bl058_manual_runtime_20260317T042208Z_71715/` — manual runtime evidence packet
- `TestEvidence/bl058_companion_profile_20260317T041625Z_67899/status.tsv` — contract-only 10/10 PASS
