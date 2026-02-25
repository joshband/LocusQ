---
Title: HX-02 Registration Lock and Memory-Order Audit
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-24
---

# HX-02: Registration Lock and Memory-Order Audit

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | In Validation (Slices A-B complete; Slice C pending) |
| Owner Track | Track F — Hardening |
| Depends On | BL-016 (Done) |
| Blocks | — |
| Annex Spec | (inline — references SceneGraph and SharedPtrAtomicContract) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Static analysis of atomic ordering |
| B | Med | M | Fix any violations found |
| C | Low | S | Regression validation |

## Objective

Audit and fix all registration lock and memory-order contract expectations in SceneGraph, SharedPtrAtomicContract, and any shared state paths. Ensure all atomic operations use correct memory ordering (acquire/release/seq_cst as appropriate) and that registration (emitter slot claiming) is free of race conditions.

## Scope & Non-Scope

**In scope:**
- SceneGraph.h atomic ordering audit (double-buffer swap, slot registration, snapshot publication)
- SharedPtrAtomicContract.h audit (atomic load/store patterns)
- PluginProcessor.cpp shared state paths (scene publication, parameter reads)
- Any other atomic usage across Source/

**Out of scope:**
- New features or API changes
- Performance optimization of atomic operations
- Lock-free algorithm redesign

## Architecture Context

- SceneGraph: lock-free singleton with double-buffered EmitterSlot array (`Source/SceneGraph.h`)
- SharedPtrAtomicContract: atomic shared_ptr wrappers from HX-01 (`Source/SharedPtrAtomicContract.h`)
- Scene snapshot publication: PluginProcessor.cpp publishes from processBlock to UI via atomic snapshot
- Invariants: Scene Graph (lock-free exchange), Audio Thread (RT safety)
- BL-016 transport contract provides the baseline for sequence-safe snapshot transport

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Static analysis of all atomic usage | `Source/SceneGraph.h`, `Source/SharedPtrAtomicContract.h`, `Source/PluginProcessor.cpp` | BL-016 done | Audit document with findings |
| B | Fix violations | Files from audit | Audit complete | All violations fixed |
| C | Regression validation | `tests/`, `TestEvidence/` | Fixes applied | Smoke + acceptance pass |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl HX-02 Slice A: Atomic ordering static audit
Load: $skill_impl, $skill_testing, $juce-webview-runtime

Objective: Audit every atomic operation in the LocusQ codebase for correct memory ordering.

Audit checklist per atomic operation:
1. Is the ordering correct? (acquire for loads, release for stores, seq_cst only when needed)
2. Is there a corresponding acquire for every release? (paired ordering)
3. Are there any relaxed operations that should be stronger?
4. Are compare_exchange operations using correct success/failure orderings?
5. Is the registration (slot claiming) path in SceneGraph free of ABA problems?
6. Does the double-buffer swap use correct acquire/release fencing?

Files to audit:
- Source/SceneGraph.h — all atomic members and operations
- Source/SharedPtrAtomicContract.h — atomic shared_ptr load/store
- Source/PluginProcessor.cpp — scene snapshot publication, parameter reads
- Source/SpatialRenderer.h — any atomic state
- Any other files with std::atomic usage

Output: Structured audit document listing each atomic operation, its current ordering,
whether it's correct, and recommended fix if not.

Evidence:
- TestEvidence/hx02_registration_audit_<timestamp>/atomic_audit.md
```

### Slice A — Standalone Fallback Prompt

```
You are auditing HX-02 for LocusQ, a JUCE spatial audio plugin.

PROJECT CONTEXT:
- SceneGraph (Source/SceneGraph.h): Lock-free singleton with double-buffered EmitterSlot array.
  Uses atomic operations for slot registration, buffer swapping, and snapshot publication.
- SharedPtrAtomicContract (Source/SharedPtrAtomicContract.h): Atomic shared_ptr wrappers
  created in HX-01 migration.
- PluginProcessor (Source/PluginProcessor.cpp): Publishes scene snapshots from processBlock
  (audio thread) consumed on message thread. Uses atomic parameter reads.
- RT safety invariant: processBlock must be lock-free, allocation-free.

TASK:
1. Search all Source/ files for std::atomic usage:
   grep -rn "std::atomic\|atomic_\|memory_order\|fetch_add\|compare_exchange\|load(\|store(" Source/
2. For each atomic operation found, document:
   - File and line number
   - Variable name and type
   - Operation (load/store/fetch_add/compare_exchange/etc.)
   - Current memory ordering
   - Whether ordering is correct (with reasoning)
   - Recommended fix if incorrect
3. Check for paired acquire/release patterns
4. Check for ABA problems in slot registration
5. Write audit report to TestEvidence/hx02_registration_audit_<timestamp>/atomic_audit.md

CONSTRAINTS:
- Do not modify any source code in this slice — audit only
- Be conservative: flag anything that could be wrong, even if uncertain

EVIDENCE:
- TestEvidence/hx02_registration_audit_<timestamp>/atomic_audit.md
```

### Slice B — Skill-Aware Prompt

```
/impl HX-02 Slice B: Fix atomic ordering violations
Load: $skill_impl, $juce-webview-runtime

Objective: Fix all violations identified in the Slice A audit.

Constraints:
- Minimize changes — fix ordering only, do not refactor algorithms
- Each fix must preserve the lock-free guarantee
- Test each fix individually before combining
- No performance regressions (atomic ordering upgrades are generally free on x86, but verify on ARM)

Validation:
- Build succeeds with no warnings
- Smoke suite passes
- No behavioral changes visible in UI

Evidence:
- TestEvidence/hx02_registration_audit_<timestamp>/fixes_applied.md
```

### Slice C — Skill-Aware Prompt

```
/test HX-02 Slice C: Regression validation after atomic fixes
Load: $skill_test, $skill_testing

Objective: Full regression validation after atomic ordering fixes.

Validation:
- ctest --test-dir build --output-on-failure
- Production self-test lane
- REAPER headless render smoke (if available)
- Multi-instance stability check (HX-03 regression guard)

Evidence:
- TestEvidence/hx02_registration_audit_<timestamp>/regression.log
- TestEvidence/hx02_registration_audit_<timestamp>/status.tsv
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| HX-02-audit | Manual | Code review | All atomics documented and assessed |
| HX-02-fixes | Automated | Build + smoke | Zero warnings, smoke passes |
| HX-02-regression | Automated | ctest + self-test + REAPER smoke | All lanes pass |
| HX-02-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Fixing ordering introduces subtle behavioral change | High | Med | Test each fix individually, compare output |
| ARM vs x86 ordering semantics differ | Med | Low | Test on ARM Mac (Apple Silicon) specifically |
| Missing paired acquire/release hard to detect | High | Med | Systematic audit with checklist |

## Failure & Rollback Paths

- If fix introduces regression: revert individual fix, isolate to specific atomic operation
- If multi-instance stability regresses: check SceneGraph registration path specifically
- If performance regresses on ARM: profile atomic operations, consider relaxed where provably safe

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Audit report | `TestEvidence/hx02_registration_audit_<timestamp>/atomic_audit.md` | file, line, operation, ordering, assessment |
| Fixes summary | `TestEvidence/hx02_registration_audit_<timestamp>/fixes_applied.md` | file, line, before, after, rationale |
| Regression log | `TestEvidence/hx02_registration_audit_<timestamp>/regression.log` | test output |
| Status TSV | `TestEvidence/hx02_registration_audit_<timestamp>/status.tsv` | lane, result, timestamp |

## Owner Validation Snapshot (2026-02-24)

| Slice | Status | Evidence |
|---|---|---|
| Slice A | Complete (audit produced actionable findings) | `TestEvidence/hx02_registration_audit_20260224_144643/atomic_audit.md`, `TestEvidence/hx02_registration_audit_20260224_144643/status.tsv` |
| Slice B | Complete (fixes implemented and validated) | `TestEvidence/hx02_registration_audit_20260224T195130Z/status.tsv`, `TestEvidence/hx02_registration_audit_20260224T195130Z/fixes_applied.md` |
| Slice C | Complete (regression lane pass) | `TestEvidence/hx02_slice_c_20260224T200311Z/status.tsv` |

## Closeout Checklist

- [x] All atomic operations audited and documented
- [x] All violations fixed
- [x] Regression suite passes (ctest, self-test, host smoke)
- [ ] No multi-instance stability regressions
- [x] Evidence captured at designated paths
- [ ] status.json updated
- [ ] Documentation/backlog/index.md row updated
- [ ] TestEvidence surfaces updated
- [ ] ./scripts/validate-docs-freshness.sh passes
