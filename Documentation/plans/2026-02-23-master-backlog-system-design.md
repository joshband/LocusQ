Title: Master Backlog System Design
Document Type: Design
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# Master Backlog System Design

## Problem Statement

The LocusQ documentation has a master backlog, plan specs, and a single runbook file, but they lack:
1. **Standardized format** — BL-026 has a status ledger; BL-028 does not. No consistent template.
2. **Agent-executable prompts** — No document contains ready-to-paste AI agent mega-prompts.
3. **Shallow runbooks** — The single runbook file has 3-4 line entries per item; no validation commands, failure paths, or evidence contracts.
4. **End-to-end traceability** — No unified pipeline from idea intake through execution to closeout.

## Approved Approach: Layered Catalog (Approach C)

Three tiers, each with a clear role:

### Tier 1: Master Index (`Documentation/backlog/index.md`)
- Priority-ordered dashboard table with status, dependencies, owner track, and links
- Mermaid dependency graph for visual blocking relationships
- Track definitions (7 parallel agent tracks A-G)
- Intake process documentation
- Definition of Ready / Definition of Done
- Closed archive compact table
- **No prose dependency rules** — dependencies encoded in table columns and Mermaid graph

### Tier 2: Runbook Docs (`Documentation/backlog/bl-XXX-<slug>.md`)
- One per backlog item (open AND closed)
- Standardized template with all sections (see Template below)
- Contains agent mega-prompts (skill-aware + standalone fallback)
- Contains validation commands, failure paths, evidence bundle contracts
- Closed items get lightweight closeout runbooks

### Tier 3: Annex Specs (`Documentation/plans/*.md`)
- Deep architectural content stays in place
- Referenced from runbooks, never duplicated
- Keep date-stamped filenames (point-in-time snapshots)

## Directory Structure

```
Documentation/
  backlog/
    index.md                          # Master index (dashboard)
    _template-runbook.md              # Template for new runbook docs
    _template-intake.md               # Template for new idea intake
    bl-001-readme-standards.md        # Closeout runbook (done item)
    ...                               # All BL/HX items
    bl-031-tempo-token-scheduler.md   # Active runbook
    hx-02-registration-lock.md
    hx-05-payload-budget.md
    hx-06-rt-safety-audit.md
  plans/                              # Annex specs (unchanged)
```

## Runbook Template

Each runbook follows this standardized structure:

### Metadata Header
```
Title: BL-XXX <Title>
Document Type: Backlog Runbook
Author: APC Codex
Created Date: YYYY-MM-DD
Last Modified Date: YYYY-MM-DD
```

### Required Sections (Active Items)
1. **Status Ledger** — Priority, status, owner track, depends on, blocks, annex spec link
2. **Effort Estimate** — Per-slice complexity (Low/Med/High) and scope (S/M/L/XL)
3. **Objective** — What, why, what success looks like
4. **Scope & Non-Scope** — Explicit boundaries
5. **Architecture Context** — Relevant decisions, invariants, ADR links
6. **Implementation Slices** — Table with slice, description, files, entry gate, exit criteria
7. **Agent Mega-Prompt** — Skill-aware prompt + standalone fallback prompt per slice
8. **Validation Plan** — Lane ID, type, command, pass criteria
9. **Risks & Mitigations** — Risk, impact, likelihood, mitigation
10. **Failure & Rollback Paths** — What to do when validation fails
11. **Evidence Bundle Contract** — Artifact, path, required fields
12. **Closeout Checklist** — All gates for marking Done

### Required Sections (Closeout Items)
1. **Status Ledger** — Completed date, final status
2. **Objective** — Past tense
3. **What Was Built** — Key changes summary
4. **Key Files** — Primary files modified
5. **Evidence References** — Links to validation artifacts
6. **Completion Date** — When closed

## Intake Process

1. **Capture** — Create `_intake-YYYY-MM-DD-<slug>.md` using intake template
2. **Triage** — Assign BL/HX ID, determine dependencies, set priority
3. **Promote** — Convert to full runbook, add to master index
4. **Archive** — Remove intake doc after promotion

## Agent Mega-Prompt Design

### Skill-Aware Format
```
/impl BL-XXX Slice A: <instruction>
Load: $skill1, $skill2
Objective: ...
Constraints: ...
Validation: ...
Evidence: ...
```

### Standalone Fallback Format
```
You are implementing BL-XXX Slice A for LocusQ, a JUCE spatial audio plugin.

CONTEXT:
- [architecture context]
- [relevant invariants]
- [file targets with current state]

TASK:
- [step-by-step instructions]

CONSTRAINTS:
- [RT safety, threading rules]

VALIDATION:
- [exact commands]
- [expected output patterns]

EVIDENCE:
- [where to write results]
```

## Lifecycle & Governance

### Status Transitions
```
Intake → In Planning → In Progress → In Validation → Done
```

### Sync Contract (ADR-0005 Extended)
Any status change must update in the same changeset:
1. Runbook Status Ledger
2. Master index table
3. `status.json`
4. `TestEvidence/build-summary.md` and `validation-trend.md`
5. `README.md` and `CHANGELOG.md` (for Done transitions)

## Migration Plan
1. Create `Documentation/backlog/` directory
2. Write templates (`_template-runbook.md`, `_template-intake.md`)
3. Generate master index from current backlog data
4. Generate individual runbooks for all 17 open items
5. Generate lightweight closeout runbooks for 17 done items
6. Archive legacy `Documentation/runbooks/backlog-execution-runbooks.md`
7. Update `Documentation/README.md` tier references
8. Update `Documentation/standards.md` backlog contract

## Naming Conventions
- Runbooks: `bl-XXX-<short-slug>.md` (no dates — dates in metadata)
- Intake: `_intake-YYYY-MM-DD-<slug>.md`
- Templates: `_template-<type>.md` (sort first)
- Plan specs: keep existing `bl-XXX-<slug>-YYYY-MM-DD.md` convention (point-in-time)
