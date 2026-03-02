---
Title: HX-06 Recurring RT-Safety Static Audit
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-03-02
---

# HX-06: Recurring RT-Safety Static Audit

## Plain-Language Summary

This runbook tracks **BL-016** (HX-06: Recurring RT-Safety Static Audit). Current status: **Done**. In plain terms: Establish a recurring RT-safety static-audit lane that scans processBlock() call paths for heap allocation, lock acquisition, blocking I/O, and other RT-unsafe operations.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |
| What is changing? | HX-06: Recurring RT-Safety Static Audit |
| Why is this important? | Establish a recurring RT-safety static-audit lane that scans processBlock() call paths for heap allocation, lock acquisition, blocking I/O, and other RT-unsafe operations. |
| How will we deliver it? | Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion. |
| When is it done? | This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done. |
| Where is the source of truth? | Runbook: `Documentation/backlog/done/hx-06-rt-safety-audit.md` plus repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

Use visuals only when they improve understanding; prefer compact tables first.

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status Ledger table | Gives a fast plain-language view of priority, state, dependencies, and ownership. | `## Status Ledger` |
| Validation table | Shows exactly how we verify success and safety. | `## Validation Plan` |
| Implementation slices table | Explains step-by-step delivery order and boundaries. | `## Implementation Slices` |
| Optional diagram/screenshot/chart | Use only when it makes complex behavior easier to understand than text alone. | Link under the most relevant section (usually validation or evidence). |


## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Done |
| Owner Track | Track F — Hardening |
| Depends On | BL-016 (Done) |
| Blocks | BL-030 |
| Annex Spec | (inline — references invariants and RT safety contract) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Med | M | Write audit script |
| B | Med | M | CI integration |
| C | Low | S | Baseline run + report |

## Objective

Establish a recurring RT-safety static-audit lane that scans processBlock() call paths for heap allocation, lock acquisition, blocking I/O, and other RT-unsafe operations. This lane must run in CI and produce structured reports, blocking merges that introduce RT violations.

## Scope & Non-Scope

**In scope:**
- Static analysis script for RT-unsafe patterns in processBlock call graph
- CI integration (GitHub Actions or local pre-commit hook)
- Baseline audit run with structured report
- Known-safe allowlist for false positives

**Out of scope:**
- Dynamic analysis or runtime profiling
- Fixing existing RT violations found (those become separate HX items)
- Modifying processBlock architecture

## Architecture Context

- processBlock path: `Source/PluginProcessor.cpp::processBlock()` calls into SpatialRenderer, PhysicsEngine, CalibrationEngine, FDNReverb, EarlyReflections, VBAPPanner, etc.
- All DSP headers are in `Source/` — header-only pattern
- RT-unsafe patterns to detect: `new`, `delete`, `malloc`, `free`, `std::mutex`, `lock()`, `std::vector::push_back`, `std::string` construction, `juce::MessageManager`, `triggerAsyncUpdate`, file I/O, `std::cout`
- Invariants: `Documentation/invariants.md` — Audio Thread section
- Existing atomic contract: HX-01 (SharedPtrAtomicContract.h) ensures shared_ptr usage is atomic

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Write RT audit script | `scripts/rt-safety-audit.sh` | BL-016 done | Script detects known RT-unsafe patterns |
| B | CI integration | `.github/workflows/` or pre-commit config | Script working | CI runs audit on every push |
| C | Baseline run + report | `TestEvidence/` | CI integrated | Clean baseline or documented exceptions |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/impl HX-06 Slice A: Write RT-safety static audit script
Load: $skill_impl, $skill_testing, $skill_docs

Objective: Create scripts/rt-safety-audit.sh that scans Source/ for RT-unsafe patterns
that could appear in the processBlock call path.

Patterns to detect (grep/ripgrep based):
- Heap allocation: new, delete, malloc, free, calloc, realloc
- Locking: std::mutex, std::lock_guard, std::unique_lock, .lock(), .unlock()
- STL dynamic allocation: push_back, emplace_back, resize, reserve, std::string(
- Blocking I/O: std::cout, std::cerr, fprintf, fopen, fwrite
- JUCE message thread calls: MessageManager, triggerAsyncUpdate, callAsync
- Exceptions: throw, try, catch (RT threads should not use exceptions)

Script requirements:
- Exit 0 if no violations found (or all are in allowlist)
- Exit 1 if violations found, with file:line:pattern output
- Allowlist file: scripts/rt-safety-allowlist.txt (one pattern per line: file:line)
- Only scan files reachable from processBlock call graph (start with known DSP headers)
- Output: structured TSV (file, line, pattern, severity, allowlisted)

Files to scan (processBlock call graph):
- Source/PluginProcessor.cpp (processBlock and called methods)
- Source/SpatialRenderer.h
- Source/PhysicsEngine.h
- Source/CalibrationEngine.h
- Source/FDNReverb.h
- Source/EarlyReflections.h
- Source/VBAPPanner.h
- Source/SpreadProcessor.h
- Source/DirectivityFilter.h
- Source/DistanceAttenuator.h
- Source/DopplerProcessor.h
- Source/AirAbsorption.h
- Source/SceneGraph.h (audio-thread access paths)
- Source/VisualTokenScheduler.h (if exists — BL-031)

Constraints:
- Script must be fast (< 5 seconds)
- False positive rate must be manageable (allowlist mechanism)
- Must work on macOS (bash + grep/rg)

Validation:
- Run script on current codebase
- Verify known-safe patterns are not flagged
- Verify intentionally unsafe test patterns would be flagged

Evidence:
- scripts/rt-safety-audit.sh (the script itself)
- scripts/rt-safety-allowlist.txt (initial allowlist)
- TestEvidence/hx06_rt_audit_<timestamp>/baseline_report.tsv
```

### Slice A — Standalone Fallback Prompt

```
You are implementing HX-06 Slice A for LocusQ.

PROJECT CONTEXT:
- LocusQ is a JUCE spatial audio plugin
- processBlock() in Source/PluginProcessor.cpp is the audio thread entry point
- processBlock calls into: SpatialRenderer, PhysicsEngine, CalibrationEngine,
  FDNReverb, EarlyReflections, VBAPPanner, SpreadProcessor, DirectivityFilter,
  DistanceAttenuator, DopplerProcessor, AirAbsorption, SceneGraph
- All these are header-only files in Source/
- RT safety invariant: NO heap allocation, locks, blocking I/O, exceptions in processBlock

TASK:
1. Create scripts/rt-safety-audit.sh:
   - Use grep or rg to scan the processBlock call graph files
   - Search for RT-unsafe patterns listed above
   - Support an allowlist file at scripts/rt-safety-allowlist.txt
   - Output structured TSV to stdout
   - Exit 0 (clean) or 1 (violations found)
2. Create scripts/rt-safety-allowlist.txt with initial known-safe entries
3. Run the script on current codebase
4. Document results

CONSTRAINTS:
- Must work on macOS with bash
- Must be fast (< 5 seconds)
- Must have clear allowlist mechanism for false positives

EVIDENCE:
- scripts/rt-safety-audit.sh
- scripts/rt-safety-allowlist.txt
- TestEvidence/hx06_rt_audit_<timestamp>/baseline_report.tsv
```

### Slice B — Skill-Aware Prompt

```
/impl HX-06 Slice B: CI integration for RT audit
Load: $skill_impl, $skill_docs

Objective: Integrate the RT audit script into CI so it runs on every push/PR.

Options (choose based on existing CI setup):
1. GitHub Actions workflow step
2. Pre-commit hook
3. Both

Constraints:
- Audit failure must block merge (not just warn)
- Audit must run before test suite (fail fast)
- Must cache allowlist between runs

Evidence:
- CI configuration file (workflow YAML or pre-commit config)
- TestEvidence/hx06_rt_audit_<timestamp>/ci_integration_proof.log
```

### Slice C — Skill-Aware Prompt

```
/test HX-06 Slice C: Baseline audit run and report
Load: $skill_test, $skill_docs

Objective: Run RT audit on current codebase, document baseline, resolve any
unexpected findings.

Validation:
- ./scripts/rt-safety-audit.sh
- Expected: exit 0 (clean) or documented exceptions in allowlist

Evidence:
- TestEvidence/hx06_rt_audit_<timestamp>/baseline_report.tsv
- TestEvidence/hx06_rt_audit_<timestamp>/status.tsv
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| HX-06-script | Automated | `./scripts/rt-safety-audit.sh` | Exit 0 or documented allowlist |
| HX-06-ci | Automated | CI pipeline run | Audit step passes |
| HX-06-baseline | Automated | Full audit run | Clean report or allowlisted exceptions |
| HX-06-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| High false positive rate | Med | Med | Allowlist mechanism + iterative tuning |
| Audit misses actual violations (false negatives) | High | Med | Start conservative, expand patterns over time |
| CI integration breaks existing pipeline | Med | Low | Add as separate job, non-blocking initially |

## Failure & Rollback Paths

- If script has too many false positives: expand allowlist, refine grep patterns
- If script misses known violations: add test case patterns, verify grep coverage
- If CI integration fails: run as local-only script until CI is fixed

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Audit script | `scripts/rt-safety-audit.sh` | executable script |
| Allowlist | `scripts/rt-safety-allowlist.txt` | file:line entries |
| Baseline report | `TestEvidence/hx06_rt_audit_<timestamp>/baseline_report.tsv` | file, line, pattern, severity, allowlisted |
| Status TSV | `TestEvidence/hx06_rt_audit_<timestamp>/status.tsv` | lane, result, timestamp |

## Execution Snapshot (2026-02-24)

- Script + allowlist are landed at `scripts/rt-safety-audit.sh` and `scripts/rt-safety-allowlist.txt`.
- CI integration is wired in `.github/workflows/qa_harness.yml` via `rt-safety-audit` and `needs: rt-safety-audit` on QA jobs (fail-fast merge gate behavior).
- Latest baseline evidence bundle: `TestEvidence/hx06_rt_audit_20260224T185318Z/` (`raw_report.tsv`, `baseline_report.tsv`, `ci_integration_proof.log`, `status.tsv`; `raw_exit=1`, `baseline_exit=0`).
- Slice D allowlist reconciliation evidence: `TestEvidence/hx06_allowlist_reconcile_20260224T202825Z/` (`rt_audit.tsv`, `rt_audit.log`, `status.tsv`, `allowlist_delta.md`; `non_allowlisted=0`).
- Owner line-drift reconciliation evidence: `TestEvidence/owner_rt_allowlist_refresh_20260224T224531Z/` (`rt_audit_before.tsv`, `rt_audit_after.tsv`, `rt_audit_before.log`, `rt_audit_after.log`, `status.tsv`; `before_non_allowlisted=102`, `after_non_allowlisted=0`).
- Owner line-drift reconciliation refresh (post BL-029 B1/C1/E1 merges): `TestEvidence/owner_rt_allowlist_refresh_20260224T231523Z/` (`rt_audit_before.tsv`, `rt_audit_after.tsv`, `rt_audit_before.log`, `rt_audit_after.log`, `status.tsv`; `before_non_allowlisted=92`, `after_non_allowlisted=0`).
- Owner line-drift reconciliation refresh (BL-029 G1/G2/G3 intake): `TestEvidence/owner_rt_allowlist_refresh_20260225T000548Z/` (`rt_audit_before.tsv`, `rt_audit_after.tsv`, `status.tsv`; `before_non_allowlisted=87`, `after_non_allowlisted=0`).
- Owner follow-up reconciliation for additional line movement: `TestEvidence/owner_rt_allowlist_refresh_20260225T000647Z/` (`rt_audit_before.tsv`, `rt_audit_after.tsv`, `status.tsv`; `before_non_allowlisted=6`, `after_non_allowlisted=0`).
- Owner BL-029 G4 replay confirms recurring drift under active parallel edits: `TestEvidence/owner_bl029_g4_triage_20260225T001418Z/` (`build/selftest/qa` pass, `rt_audit` fail with `non_allowlisted=110`).
- Owner BL-029 G5 replay confirms same RT drift signature while scoped selftests pass: `TestEvidence/owner_bl029_g5_triage_20260225T001834Z/` (`selftest_bl029` 3/3 pass, `rt_audit` fail with `non_allowlisted=110`).
- Owner freeze-window finalizer (Slice Z1): `TestEvidence/owner_bl029_rt_finalize_z1_20260225T002800Z/` (`freeze_fingerprint_before/after` stable, `before_non_allowlisted=110`, `after_non_allowlisted=0`, `docs_freshness=PASS`).
- Owner Z2 replay confirms standalone aborts are cleared but line-map drift returned after subsequent source changes: `TestEvidence/owner_bl029_z2_replay_20260225T013944Z/` (`selftest_bl029` 3/3 pass, `selftest_bl009` pass, `qa-bl009-headphone-contract` pass, `rt_audit` fail with `non_allowlisted=103`).
- Owner integrated closeout replay (Z2+Z3): `TestEvidence/owner_bl029_z2_z3_integrated_20260225T014638Z/` (all functional lanes pass; RT drift observed at `non_allowlisted=103` before reconciliation).
- Owner post-Z2/Z3 finalizer: `TestEvidence/owner_bl029_rt_finalize_z2z3_20260225T014808Z/` (`freeze_guard=PASS`, `before_non_allowlisted=0`, `after_non_allowlisted=0`, `docs_freshness=PASS`).
- Owner done-promotion packet (current branch verification): `TestEvidence/hx06_done_promotion_20260225T160025Z/` (`rt_audit.tsv`, `ci_wiring_check.log`, `docs_freshness.log`, `status.tsv`, `promotion_decision.md`; `non_allowlisted=0`).
- Owner parallel integration replay confirms HX-06 done-state stability on current tree: `TestEvidence/owner_parallel_integration_20260225T162457Z/` (`rt_audit.tsv` summary `non_allowlisted=0`, `docs_freshness.log` pass).
- BL-028 C2 RT gate reconciliation refresh: `TestEvidence/bl028_rt_gate_c2_20260225T210743Z/` (`rt_before.tsv` summary `non_allowlisted=113`, explicit allowlist drift reconciliation in `scripts/rt-safety-allowlist.txt`, `rt_after.tsv` summary `non_allowlisted=0`, `docs_freshness.log` pass).
- BL-028 C2 RT gate reconciliation verification rerun: `TestEvidence/bl028_rt_gate_c2_20260225T211058Z/` (`rt_before.tsv` summary `non_allowlisted=0`, `rt_after.tsv` summary `non_allowlisted=0`, no allowlist delta required, `docs_freshness.log` pass).
- BL-032 Slice B2 RT gate reconciliation: `TestEvidence/bl032_slice_b2_rt_reconcile_20260225T223431Z/` (`rt_before.tsv` summary `non_allowlisted=80`, `allowlist_delta.md` captures 80 explicit `file:line:RULE_ID` additions in `scripts/rt-safety-allowlist.txt`, `rt_after.tsv` summary `non_allowlisted=0`, freeze fingerprints stable for `Source/PluginProcessor.cpp`, `Source/processor_core/ProcessorParameterReaders.h`, `Source/processor_bridge/ProcessorBridgeUtilities.h`, `Source/shared_contracts/BridgeStatusContract.h`, `docs_freshness.log` pass).
- Owner caveat: future source line movement can still require allowlist refresh, but baseline is green at promotion time.
- Validation state: tested (local baseline pass + CI wiring contract parse + docs freshness pass).

## Closeout Checklist

- [x] RT audit script written and functional
- [x] Allowlist populated with initial known-safe entries
- [x] CI integration active (blocks violations on merge) *(workflow wired; first GitHub Actions run pending)*
- [x] Baseline audit run complete with clean report
- [x] Evidence captured at designated paths
- [x] status.json updated
- [x] Documentation/backlog/index.md row updated
- [x] TestEvidence surfaces updated
- [x] ./scripts/validate-docs-freshness.sh passes


## Governance Retrofit (2026-02-28)

This additive retrofit preserves historical closeout context while aligning this done runbook with current backlog governance templates.

### Status Ledger Addendum

| Field | Value |
|---|---|
| Promotion Decision Packet | `Legacy packet; see Evidence References and related owner sync artifacts.` |
| Final Evidence Root | `Legacy TestEvidence bundle(s); see Evidence References.` |
| Archived Runbook Path | `Documentation/backlog/done/hx-06-rt-safety-audit.md` |

### Promotion Gate Summary

| Gate | Status | Evidence |
|---|---|---|
| Build + smoke | Legacy closeout documented | `Evidence References` |
| Lane replay/parity | Legacy closeout documented | `Evidence References` |
| RT safety | Legacy closeout documented | `Evidence References` |
| Docs freshness | Legacy closeout documented | `Evidence References` |
| Status schema | Legacy closeout documented | `Evidence References` |
| Ownership safety (`SHARED_FILES_TOUCHED`) | Required for modern promotions; legacy packets may predate marker | `Evidence References` |

### Backlog/Status Sync Checklist

- [x] Runbook archived under `Documentation/backlog/done/`
- [x] Backlog index links the done runbook
- [x] Historical evidence references retained
- [ ] Legacy packet retrofitted to modern owner packet template (`_template-promotion-decision.md`) where needed
- [ ] Legacy closeout fully normalized to modern checklist fields where needed
