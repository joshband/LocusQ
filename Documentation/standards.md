Title: LocusQ Documentation Standards
Document Type: Standard
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-01

# Documentation Standards

## Scope
Applies to human-authored markdown in:
- repository root (`AGENTS.md`, `AGENT_RULE.md`, `CLAUDE.md`, `SKILLS.md`)
- `.ideas/`
- `Design/`
- `Documentation/`
- `TestEvidence/`

Generated markdown under `qa_output/` is exempt.

Skill-runtime markdown under `.codex/skills/**` and `.claude/skills/**` is also exempt from this repository metadata-header contract and follows Codex/Claude skill standards.

## Required Metadata Header
Every in-scope markdown file must include, in this order, at the top of file:
1. `Title`
2. `Document Type`
3. `Author`
4. `Created Date`
5. `Last Modified Date`

## Naming Conventions
- Use lowercase kebab-case for new docs, except canonical legacy files already in use.
- ADR names must follow: `ADR-XXXX-kebab-case.md` (zero-padded index).
- Keep versioned design docs in `Design/` as `vN-ui-spec.md` and `vN-style-guide.md`.

## Folder Placement
- Concepts/specs: `.ideas/`
- UI design artifacts: `Design/`
- Stable reference docs/ADRs/invariants/traceability: `Documentation/`
- Execution runbooks: `Documentation/runbooks/`
- Validation artifacts and run logs: `TestEvidence/`

## Source-Of-Truth Tiering
- Tier 0 canonical docs are listed in `Documentation/README.md` and are the only authority for status/closeout claims.
- Tier 1 docs are active execution specs and may drive implementation detail, but they must not supersede Tier 0 status surfaces.
- Tier 2 docs are historical/research references and are non-authoritative.
- Tier 3 docs are archived artifacts under `Documentation/archive/`.
- Cross-system architecture authority should be consolidated in `ARCHITECTURE.md`; duplicate architecture review docs should be archived once consolidated.

## Master Backlog Contract
1. `Documentation/backlog/index.md` is the single backlog authority for priority, ordering, and state.
2. Individual runbook docs in `Documentation/backlog/` carry execution detail, agent mega-prompts, validation plans, and evidence contracts (`bl-XXX-*.md` for open work, `done/*.md` for completed work).
3. Plan docs under `Documentation/plans/` carry deep architecture content but must not become competing backlog ledgers.
4. New backlog items enter via `Documentation/backlog/_template-intake.md` and are promoted to full runbooks using `Documentation/backlog/_template-runbook.md`.
5. The legacy files `Documentation/backlog-post-v1-agentic-sprints.md` and `Documentation/runbooks/backlog-execution-runbooks.md` are superseded and retained as Tier 2 reference only.

## Backlog Lifecycle Governance Standard

Applies to all remaining open backlog items and all future backlog items.

1. Intake must use `Documentation/backlog/_template-intake.md` and include replay/cost planning plus ownership boundaries.
2. Active runbooks must include replay tiering via `Documentation/backlog/_template-runbook.md` fields (`Default Replay Tier`, `Heavy Lane Budget`, and `Replay Cadence Plan`).
3. Owner promotion packets must use `Documentation/backlog/_template-promotion-decision.md`, including explicit:
   - replay cadence compliance,
   - ownership safety (`SHARED_FILES_TOUCHED: no|yes`),
   - evidence localization under `TestEvidence/`.
4. Done transitions must use `Documentation/backlog/_template-closeout.md` and move runbooks to `Documentation/backlog/done/bl-XXX-*.md` in the same change set as index/status/evidence sync.
5. Done/closeout evidence is not valid when canonical promotion artifacts only exist in `/tmp`; canonical copies must be under repository `TestEvidence/`.
6. Conformance scope:
   - active/open runbooks (`Documentation/backlog/bl-*.md`) must satisfy the current runbook schema and cadence policy;
   - legacy done runbooks (`Documentation/backlog/done/*.md`) are grandfathered and need not be bulk-retrofitted unless touched;
   - backlog support ledgers (`Document Type: Backlog Support`) are exempt from runbook schema fields but must preserve canonical runbook linkage.

## Backlog Validation Cadence Standard

Default cadence tiers are defined in `Documentation/backlog/index.md` and are normative unless owner-approved stricter overrides are documented.

1. Use the minimum tier needed for the current stage (`T0/T1` dev, `T2` intake, `T3` promotion, `T4` sentinel only).
2. Avoid blind full reruns after a single failure; diagnose the failing run index first.
3. Heavy wrappers (>=20 binary launches per wrapper run) must use cost-contained reruns unless owner explicitly requests wider sweeps.

## Cross-Reference Requirements
When code behavior changes, updated docs must reference:
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `Documentation/invariants.md`
- relevant `Documentation/adr/*.md`

## Status And Task Hygiene
- Keep task checkboxes current in `.ideas/plan.md`.
- Keep phase status aligned in `status.json`.
- Avoid duplicative “summary” docs when a canonical file already exists.

## Phase Closeout Freshness Gate
Per `Documentation/adr/ADR-0005-phase-closeout-docs-freshness-gate.md`, any phase closeout that changes acceptance/status claims must update this canonical bundle in the same change set:
- `status.json`
- `Documentation/backlog-post-v1-agentic-sprints.md`
- `README.md`
- `CHANGELOG.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

## Validation Logging
- Snapshot: update `TestEvidence/build-summary.md` after meaningful build/test runs.
- Trend: append an entry to `TestEvidence/validation-trend.md` for each meaningful run.

## API Documentation
- Doxygen is the preferred API doc generator for C++ source comments.
- Use Doxygen-style comments (`/** ... */`) for public classes and nontrivial methods.
- Recommended command: `doxygen Documentation/Doxyfile`

## Minimalism Rule
Prefer updating canonical docs over creating new files. New docs require a clear owner and purpose.

## Artifact Tracking Rule
Apply artifact tracking and retention policy from `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`:
1. classify by artifact class first;
2. keep generated/heavy artifacts local-only by default;
3. track only canonical decision-grade evidence.

## Archival Rule
When documentation bloat or ambiguity appears:
1. Classify docs into Tier 0-3 (per `Documentation/README.md`).
2. Move generated snapshots and one-off operational bundles into `Documentation/archive/<YYYY-MM-DD>-<slug>/`.
3. Keep top-level generated scratch directory `Documentation/exports/` empty or absent; archive its outputs instead.
4. Keep `Documentation/reports/` for active report artifacts that are intentionally referenceable from current docs.
5. Keep historical docs in-place only if active docs/status surfaces still reference them; otherwise archive them.
6. Update `Documentation/README.md` in the same change to reflect any tier changes.
7. Run `./scripts/validate-docs-freshness.sh` after archival edits.
8. Keep only active research under `Documentation/research/`, index it in `Documentation/research/README.md`, and move superseded research to `Documentation/archive/<YYYY-MM-DD>-<slug>/`.

## Tier Promotion Snapshot (2026-02-24)
1. Tier 1 execution specs now include `Documentation/plans/bl-029-dsp-visualization-and-tooling-spec-2026-02-24.md`.
2. Tier 1 execution specs now include `Documentation/plans/bl-031-tempo-locked-visual-token-scheduler-spec-2026-02-24.md`.
2. Tier 1 execution specs include `Documentation/runbooks/backlog-execution-runbooks.md` as the procedural companion to the master backlog.

## Closeout Sync Snapshot (2026-02-28)

1. BL-023 done-transition moved runbook authority from `Documentation/backlog/bl-023-resize-dpi-hardening.md` to `Documentation/backlog/done/bl-023-resize-dpi-hardening.md`.
2. Backlog catalog authority was synchronized in `Documentation/backlog/index.md` in the same change set.
3. Canonical promotion evidence remains repo-local under `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/`.

## Architecture Consolidation Snapshot (2026-03-01)

1. Cross-system architecture source-of-truth is consolidated in `ARCHITECTURE.md`.
2. Prior standalone architecture reviews were archived under `Documentation/archive/2026-03-01-architecture-review-consolidation/`.
3. Any future architecture review with durable value must be merged into `ARCHITECTURE.md` and archived in the same change set.
