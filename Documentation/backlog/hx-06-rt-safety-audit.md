---
Title: HX-06 Recurring RT-Safety Static Audit
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23
---

# HX-06: Recurring RT-Safety Static Audit

## Status Ledger

| Field | Value |
|---|---|
| Priority | P1 |
| Status | Open |
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

## Closeout Checklist

- [ ] RT audit script written and functional
- [ ] Allowlist populated with initial known-safe entries
- [ ] CI integration active (blocks violations on merge)
- [ ] Baseline audit run complete with clean report
- [ ] Evidence captured at designated paths
- [ ] status.json updated
- [ ] Documentation/backlog/index.md row updated
- [ ] TestEvidence surfaces updated
- [ ] ./scripts/validate-docs-freshness.sh passes
