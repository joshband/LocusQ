Title: APC Skill: docs
Document Type: Skill Specification
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

---
name: skill_docs
description: "Documentation governance skill for metadata compliance, ADR hygiene, invariant traceability, and validation evidence logging."
---

Title: Documentation Governance Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

# SKILL: DOCUMENTATION GOVERNANCE

## Goal
Keep documentation lean, current, and enforceably consistent with specs, invariants, ADRs, and validation evidence.

## When To Use
- User asks to standardize docs, naming, structure, ADRs, or evidence logging.
- A phase changes behavior or implementation contracts.
- Validation results are produced and need durable snapshots/trends.

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

## Output Requirements
- Updated docs with metadata and cross-references.
- ADR updates for new/changed architectural decisions.
- Validation snapshot/trend entries reflecting the latest evidence.
- Brief report of what was updated and what remains intentionally deferred.
