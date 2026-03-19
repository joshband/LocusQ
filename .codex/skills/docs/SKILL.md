---
name: skill_docs
description: "Documentation governance skill for metadata compliance, ADR hygiene, invariant traceability, and validation evidence logging."
---

Title: Documentation Governance Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

# SKILL: DOCUMENTATION GOVERNANCE

## Goal
Keep documentation lean, current, and enforceably consistent with specs, invariants, ADRs, and validation evidence.
Ensure backlog lifecycle documents are equally readable by non-technical humans and machine consumers (agents/scripts/LLMs).
Apply Smart Brevity by default: short sentences, short paragraphs, clear labels, compact tables, and visuals only when they materially improve understanding.

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
7. `Documentation/backlog/runbook-authoring-guide.md`
8. `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`

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
11. Enforce portable status-rich formatting for roadmap/review/backlog docs whenever priority or progress state changes:
   - use portable markdown only; do not rely on HTML/CSS color
   - include `## Status Legend` or `## Review Status Legend`
   - include `## Priority Snapshot`, `## Progress Snapshot`, or `## Completion Snapshot`
   - use status tags `[DONE]`, `[ACTIVE]`, `[NEXT]`, `[QUEUED]`, `[DEFERRED]`, `[BLOCKED]`
   - use `~~strikethrough~~` on completed item names when appropriate
   - include estimate, actual/time, tokens, updated/completed date, scope/where, and remaining/evidence columns
   - use `not logged` or `n/a` instead of inventing time/token telemetry
12. Run readability and freshness gates before closeout:
   - `./scripts/validate-backlog-plain-language.sh`
   - `./scripts/validate-backlog-redundancy.py`
   - `./scripts/export-backlog-summaries.py --check`
   - `./scripts/validate-docs-freshness.sh`
13. Refresh machine-readable backlog summary artifacts when runbooks/index change:
   - `./scripts/export-backlog-summaries.py`
14. Enforce Smart Brevity on touched docs:
   - lead with the answer or decision,
   - keep most paragraphs to `1-3` sentences,
   - split long narrative into bullets, tables, or labeled sections,
   - use concrete dates, paths, owners, and evidence pointers,
   - avoid filler and repeated policy prose when a canonical pointer will do,
   - use visuals only when they reduce ambiguity faster than prose.
15. When backlog automation is in scope, enforce the `ADR-0023` boundary:
   - keep automation draft-only by default,
   - allow packet drafting and proposed status diffs,
   - require owner confirmation before promotion, archive, or authoritative status changes.

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
- Smart Brevity compliance for touched docs, including any intentional exceptions.
- Backlog automation governance notes when applicable, including owner-confirmation boundaries.
- Readability report covering plain-language + 6W + visual-aid coverage for touched backlog docs.
- Status-rich snapshot coverage for touched roadmap/review/backlog docs, including any `not logged` / `n/a` effort fields used honestly.
- Brief report of what was updated and what remains intentionally deferred.
