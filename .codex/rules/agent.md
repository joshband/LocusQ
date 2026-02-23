Title: APC Agent Rule Contract
Document Type: Rule
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-23

# AGENT_RULE.md

## Purpose
Canonical APC agent rule contract.

This file is the single source for:
- `.codex/rules/agent.md`
- `.claude/rules/agent.md`

Do not edit those copies directly. After editing this file, sync both copies with:
`cp AGENT_RULE.md .codex/rules/agent.md && cp AGENT_RULE.md .claude/rules/agent.md`

## Priority Order
1. User request
2. Safety and correctness
3. `AGENTS.md` routing rules
4. This file
5. Active workflow and skill instructions
6. Existing repository conventions

## Routing Contract
- Slash commands and natural-language intent map through `AGENTS.md`.
- Enforce one phase at a time.
- Do not auto-advance to the next phase when a phase completes.

## Required Load Sequence
For phase execution, always load in this order:
1. This rule file (`rules/agent.md`)
2. Matching workflow in `../workflows/`
3. Referenced skill in `../skills/`

## State Contract
Before phase work:
- Read `status.json`.
- Validate prerequisites against the selected workflow and documented phase gates.

During phase work:
- Keep edits in phase scope.
- Update state directly in `status.json` with explicit rationale in notes/evidence surfaces.

After phase work:
- Validate required artifacts exist.
- Stop and report next expected command.

## Spec/Invariant/ADR Contract
- Treat `.ideas/architecture.md`, `.ideas/parameter-spec.md`, `.ideas/plan.md`, `Documentation/invariants.md`, and `Documentation/adr/*.md` as normative implementation inputs.
- For any code change, confirm behavior still satisfies documented invariants and ADR decisions.
- If a change requires deviating from a recorded invariant/ADR, create or update an ADR before closing the phase and reference it in status notes.
- Keep parameter and control wiring traceable in `Documentation/implementation-traceability.md`.

## Documentation Contract
- Human-authored Markdown docs in root, `.codex/`, `.claude/`, `.ideas/`, `Design/`, `Documentation/`, and `TestEvidence/` must include: `Title`, `Document Type`, `Author`, `Created Date`, and `Last Modified Date`.
- Generated reports under `qa_output/` are exempt and must not be manually edited for metadata.
- Keep docs concise: update canonical docs instead of creating parallel duplicates.
- Follow folder and naming conventions in `Documentation/standards.md`.
- Follow source-of-truth tiers in `Documentation/README.md` (`Tier 0..3`).
- Keep `Documentation/reports/` and `Documentation/exports/` empty/absent at closeout; archive outputs under `Documentation/archive/<YYYY-MM-DD>-<slug>/`.
- Record validation snapshots and trend entries in `TestEvidence/validation-trend.md`.
- Run `./scripts/validate-docs-freshness.sh`; populated generated doc output dirs must be treated as closeout blockers.

## Framework Gate
`ui_framework` in `status.json` is binding:
- `visage`: avoid WebView-only implementation outputs.
- `webview`: use WebView-compatible UI and integration paths.
- `pending`: block framework-specific implementation until resolved in planning.

## Build and Validation
- Prefer repository workflows/scripts over ad-hoc command sequences.
- Run the smallest meaningful validation first, then broaden as needed.
- Log significant validation commands/results in `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md`.
- If validation is skipped, state why.
- Report status as: `tested`, `partially tested`, or `not tested`.

## Troubleshooting Contract
- Check known issues first: `../troubleshooting/known-issues.yaml`.
- Reuse documented resolutions when a match exists.
- If failures repeat, capture issue details and document the fix.

## Response Quality
- Lead with result/recommendation.
- Keep reasoning concise and explicit.
- Reference changed files and validation status.
