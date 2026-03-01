Title: Backlog Execution Wave Triage (Post Code Review)
Document Type: Backlog Support
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Backlog Execution Wave Triage (Post Code Review)

## Purpose

Lock owner assignments, wave order, and gate policy after the 2026-03-01 review so implementation can run without ambiguity.

## Owner Pods

| Pod | Scope | Primary BL Ownership |
|---|---|---|
| Hardening Pod | RT/runtime safety, thread-safe publication, bridge coherence | BL-050, BL-069, BL-070 |
| Calibration Pod | Companion/profile/calibration flow correctness | BL-058, BL-059 |
| QA Governance Pod | Contract-vs-execute truthfulness gates and promotion controls | BL-073 |
| Runtime Formats Pod | AUv3 lifecycle and host validation | BL-067 |
| Temporal DSP Pod | Delay/echo/looper/frippertronics core lanes | BL-068 |
| WebView Runtime Pod | UI runtime diagnostics and strict-gesture/degraded-mode gates | BL-074 |
| Listening Validation Pod | Phase B listening harness execution | BL-060 |
| HRTF Validation Pod | Conditional interpolation/crossfade lane | BL-061 (conditional) |

## Wave Sequence

### Wave 1 (Start Immediately)

- BL-050
- BL-058
- BL-059
- BL-073

Exit conditions:
- BL-073 execute-mode semantics are active and enforce `TODO` rows as failing in execute lanes.
- BL-050/BL-058/BL-059 produce implementation evidence and blocker taxonomy updates.

### Wave 2 (After Wave 1 Exit Conditions)

- BL-067
- BL-068
- BL-074

Entry gate:
- BL-073 policy and execute-mode failure semantics are in effect.

Promotion policy:
- BL-067 and BL-068 are `NO-GO` for promotion while any required execute evidence row remains `TODO`.

### Wave 3 (After Wave 2 Stabilizes)

- BL-060
- BL-061 (only if BL-060 gate passes)

Entry gate:
- BL-059 handoff and Wave 2 promotion-blocker governance remain stable.

## Current Kickoff Status (2026-03-01)

- Wave 1 started.
- BL-069 and BL-070 initial code remediation is landed in runtime sources and compile-validated.
- Backlog index/runbooks now encode the immediate promotion-blocker policy for BL-067/BL-068.
