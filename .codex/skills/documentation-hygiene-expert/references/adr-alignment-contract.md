Title: ADR Alignment Contract Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# ADR Alignment Contract

## Alignment Rules
1. Any changed canonical behavior doc must reference relevant ADR IDs when architecture decisions are involved.
2. If a doc claim conflicts with ADR intent, flag as `adr-conflict` and route to decision owner.
3. Do not silently update docs to contradict existing ADRs; update ADRs or add superseding ADRs first.

## Minimal Evidence For Alignment
- `doc_path`
- `adr_ids`
- `alignment_status` (`aligned`, `partial`, `conflict`)
- `resolution_action`
- `owner`

## Common Drift Patterns
- Docs describe runtime behavior that was changed in code without ADR update.
- Backlog/runbook claims promotion readiness without matching gate decisions.
- Feature docs keep historical options as active when they are deprecated or experimental.
