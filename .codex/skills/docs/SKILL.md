---
name: skill_docs
description: "Documentation governance skill for metadata compliance, ADR hygiene, invariant traceability, and validation evidence logging."
---

Title: Documentation Governance Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-02

# SKILL: DOCUMENTATION GOVERNANCE

## Goal
Keep documentation lean, current, and enforceably consistent with specs, invariants, ADRs, and validation evidence.
Ensure backlog lifecycle documents are equally readable by non-technical humans and machine consumers (agents/scripts/LLMs).

## When To Use
- User asks to standardize docs, naming, structure, ADRs, or evidence logging.
- A phase changes behavior or implementation contracts.
- Validation results are produced and need durable snapshots/trends.
- Documentation cleanup intent is primarily governance/contract-focused rather than repo-wide de-bloat refactoring.
- Root documentation (`README.md`, `CHANGELOG.md`, routing contracts) must be synchronized with current project behavior.

## Required References
1. `Documentation/standards.md`
2. `Documentation/README.md`
3. `Documentation/invariants.md`
4. `Documentation/adr/`
5. `TestEvidence/build-summary.md`
6. `TestEvidence/validation-trend.md`

## Execution Checklist
1. Confirm each human-authored markdown file has:
   - `Title`
   - `Document Type`
   - `Author`
   - `Created Date`
   - `Last Modified Date`
2. Keep canonical docs in place; avoid duplicative docs with overlapping scope.
3. Ensure changed code paths are traceable to:
   - `.ideas/architecture.md`
   - `.ideas/parameter-spec.md`
   - `Documentation/invariants.md`
   - relevant ADRs under `Documentation/adr/`
4. Keep ADRs as separate files named `ADR-XXXX-kebab-case.md`.
5. Update validation snapshot and trend logs after each meaningful run.
6. Mark checklist/task status where applicable in plan/evidence docs.
7. When routing/skill posture changes, update root docs in one change set:
   - `README.md`, `CHANGELOG.md`, `AGENTS.md`, `CODEX.md`, `CLAUDE.md`, `SKILLS.md`, `AGENT_RULE.md`
8. If canonical `TestEvidence` packets match ignore patterns, stage them intentionally with force-add and document why.
9. Exempt skill/system markdown from this skill unless explicitly requested:
   - `.codex/skills/**`
   - `.claude/skills/**`
   - `.codex/workflows/**`
   - `.claude/workflows/**`
   - `.codex/rules/**`
   - `.claude/rules/**`
10. Enforce backlog readability contract for intake/runbook/closeout/promotion and archived done runbooks:
   - `## Plain-Language Summary`
   - `## 6W Snapshot (Who/What/Why/How/When/Where)`
   - `## Visual Aid Index` (visuals only when they materially improve clarity)
11. Run readability and freshness gates before closeout:
   - `./scripts/validate-backlog-plain-language.sh`
   - `./scripts/validate-docs-freshness.sh`

## Cross-Skill Routing
- For heavy documentation cleanup, deduplication, and freshness remediation, pair with `documentation-hygiene-expert`.
- Keep `skill_docs` focused on governance contracts, metadata discipline, traceability, and root-doc sync.
- Ownership boundary:
  - `documentation-hygiene-expert` leads backlog/root/API-doc/code-comment hygiene cleanup passes.
  - `skill_docs` leads ADR/invariant traceability, metadata standards, routing-contract parity, and validation-governance closeout.

## Output Requirements
- Updated docs with metadata and cross-references.
- ADR updates for new/changed architectural decisions.
- Validation snapshot/trend entries reflecting the latest evidence.
- Readability report covering plain-language + 6W + visual-aid coverage for touched backlog docs.
- Brief report of what was updated and what remains intentionally deferred.
