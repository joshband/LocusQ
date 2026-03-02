---
name: documentation-hygiene-expert
description: SDLC-aware documentation cleanup and governance specialist for de-bloating repos, consolidating source-of-truth docs, enforcing freshness ownership, aligning implementation docs with ADRs, and automating git artifact hygiene (tracked ignored files, stale archives, and history bloat audits).
---

Title: Documentation Hygiene Expert Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-02

# Documentation Hygiene Expert

Use this skill when docs have become stale, duplicated, scattered, or hard to trust over time.

## Goal
Produce a lean, current, ADR-aligned documentation set that is easy to navigate and safe to operate during active SDLC work.
Every backlog/runbook lifecycle document must also be understandable to non-technical readers while remaining machine-parseable for scripts and coding agents.

## When To Use
- Documentation is bloated or duplicated across root folders, `Documentation/`, and feature/runbook surfaces.
- Ownership/freshness is unclear and stale docs are causing wrong implementation or testing assumptions.
- A project needs a fast cleanup pass before implementation, release, or onboarding.
- ADR alignment is weak (docs drift from `Documentation/adr/*.md`, invariants, or architecture notes).
- Backlog authority and lifecycle docs (`Documentation/backlog/index.md`, `Documentation/backlog/done/*`) need reconciliation.
- Critical intent docs such as `ARCHITECTURE.md`, API references, or high-signal code comments are stale or contradictory.
- Git trees are polluted by tracked ignored artifacts, stale evidence archives, or oversized blobs and require scripted cleanup/guardrails.

## Ownership Boundaries
`documentation-hygiene-expert` is the primary owner for:
- Repo-scale documentation inventory, de-bloat, deduplication, and canonical consolidation.
- Backlog/runbook/architecture clarity passes (including `Documentation/backlog/index.md`, `Documentation/backlog/done/*`, `ARCHITECTURE.md`).
- Root content freshness for `README.md` and `CHANGELOG.md` (behavior claims, scope, and operator-facing clarity).
- API documentation hygiene (remove stale endpoint/contract claims, tighten examples, improve scannability/accessibility).
- Code comment hygiene audits (remove stale or misleading comments and collapse duplicated narrative).
- Git artifact hygiene automation for local/CI guardrails, tracked-ignored cleanup manifests, and history-bloat audit reporting.

`skill_docs` is the primary owner for:
- Governance metadata discipline, ADR/invariant traceability, and tier/standards contract enforcement.
- Root routing-contract synchronization (`AGENTS.md`, `CODEX.md`, `CLAUDE.md`, `SKILLS.md`, `AGENT_RULE.md`).
- Validation governance closeout (`TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, freshness-gate evidence).

Use both skills for high-impact cleanup:
1. `documentation-hygiene-expert` executes inventory/consolidation and content hygiene.
2. `skill_docs` normalizes governance/metadata/traceability and closeout gates.

## Explicit Exemptions (Skill/System Markdown)
- Exempt from this skill by default:
  - `.codex/skills/**`
  - `.claude/skills/**`
  - `.codex/workflows/**`
  - `.claude/workflows/**`
  - `.codex/rules/**`
  - `.claude/rules/**`
- These files follow Codex/Claude skill-runtime standards and should only be edited when the user explicitly requests skill/runtime contract changes.

## Required References
1. `references/doc-inventory-and-triage.md`
2. `references/sdlc-freshness-cadence.md`
3. `references/adr-alignment-contract.md`
4. `references/markdown-accessibility-and-focus.md`
5. `references/root-docs-maintenance-loop.md`
6. `references/git-artifact-hygiene-automation.md`

## Workflow
1. Run artifact hygiene baseline (when repo clutter is in scope).
   - Audit current tree and reachable history:
     - `./scripts/git-artifact-hygiene-audit.sh --ref HEAD`
   - Generate tracked cleanup manifest when needed:
     - `./scripts/git-artifact-cleanup-index.sh --manifest TestEvidence/git_artifact_cleanup_candidates.tsv`
   - Enforce pre-commit/CI guardrails:
     - `./scripts/git-artifact-hygiene-guard.sh`
2. Build inventory and classify authority.
   - Identify canonical docs, duplicate docs, generated scratch docs, and stale candidate docs.
   - Assign each file one role: `canonical`, `supporting`, `generated`, `archive`, or `delete-candidate`.
3. De-bloat without losing signal.
   - Merge overlapping docs into a single canonical file.
   - Replace duplicate long-form content with short pointers to the canonical source.
   - Mark unresolved docs as `deferred` with explicit follow-up owner/date.
4. Re-anchor docs to SDLC surfaces.
   - Ensure plan/design/impl/test/ship docs map to actual workflow artifacts and current project state.
   - Ensure docs that claim behavior are backed by current code paths or current evidence.
   - Reconcile backlog authority surfaces (`Documentation/backlog/index.md` + `Documentation/backlog/done/*`) with status claims.
   - Reconcile architecture intent surfaces (`ARCHITECTURE.md`, ADRs, invariants) with operational docs.
5. Enforce ADR and invariant alignment.
   - For each changed canonical doc, verify references to applicable ADRs and invariants.
   - Flag any behavior claim that conflicts with ADR decisions for follow-up.
6. Raise readability and accessibility.
   - Improve structure, headings, and scannability.
   - Use precise language, avoid ambiguous claims, and remove filler.
   - Keep markdown consistent with repository metadata and formatting conventions.
   - Normalize API docs/examples and code-comment narrative where stale prose creates implementation risk.
   - Enforce backlog readability contract:
     - `## Plain-Language Summary`
     - `## 6W Snapshot (Who/What/Why/How/When/Where)`
     - `## Visual Aid Index`
   - Add visuals only when they increase clarity (tables first; mermaid/images/charts/screenshots when needed).
7. Publish governance outcome.
   - Record what moved, what merged, what was archived, and what was intentionally deferred.
   - Confirm freshness cadence (owner + trigger + review window) for critical docs.
8. Run root-doc maintenance loop.
   - Confirm root docs are current and non-conflicting:
     - `README.md`
     - `CHANGELOG.md`
     - `AGENTS.md`
     - `CODEX.md`
     - `CLAUDE.md`
     - `SKILLS.md`
     - `AGENT_RULE.md`
   - Escalate to `skill_docs` if governance metadata/sync contract updates are required.
9. Validate readability tooling gates.
   - `./scripts/validate-backlog-plain-language.sh`
   - `./scripts/validate-docs-freshness.sh`

## Cross-Skill Routing
- Pair with `skill_docs` for metadata/traceability/standards enforcement and root-doc sync.
- Pair with `skill_plan` when architecture/phase intent must be rebaselined before cleanup.
- Pair with `skill_testing` when outdated QA docs need replay-tier or evidence-contract normalization.
- Pair with `skill_troubleshooting` when docs conflict with runtime behavior and root-cause investigation is needed.

## Deliverables
- Documentation hygiene summary:
  - canonical docs retained/created,
  - merged/archived/delete-candidate docs,
  - unresolved follow-ups with owners.
- ADR alignment summary for changed canonical docs.
- Freshness contract summary (owner, cadence, trigger condition).
- Backlog readability summary:
  - plain-language coverage (open/done runbooks),
  - 6W coverage,
  - visual-aid coverage and rationale.
- Git artifact hygiene summary:
  - tracked-ignored findings,
  - archive/history-bloat findings,
  - staged/local/CI guardrail status,
  - cleanup plan (`index-only` vs `history rewrite`).
- Validation status: `tested`, `partially tested`, or `not tested`.
