Title: LocusQ Documentation Standards
Document Type: Standard
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

# Documentation Standards

## Scope
Applies to human-authored markdown in:
- repository root (`AGENTS.md`, `AGENT_RULE.md`, `CLAUDE.md`, `SKILLS.md`)
- `.ideas/`
- `Design/`
- `Documentation/`
- `TestEvidence/`

Generated markdown under `qa_output/` is exempt.

Skill/runtime markdown under `.codex/skills/**`, `.claude/skills/**`, `.codex/workflows/**`, `.claude/workflows/**`, `.codex/rules/**`, and `.claude/rules/**` is exempt from this repository metadata-header contract and follows Codex/Claude runtime standards.

## Required Metadata Header
Every in-scope markdown file must include, in this order, at the top of file:
1. `Title`
2. `Document Type`
3. `Author`
4. `Created Date`
5. `Last Modified Date`

## Smart Brevity Writing Standard

Per `Documentation/adr/ADR-0021-smart-brevity-documentation-contract.md`, active documentation should optimize for fast scanning without losing truth.

Authoring rules:
1. Lead with the answer.
2. Keep sentences short and specific.
3. Keep paragraphs short.
4. Prefer bullets, tables, and labels over long narrative.
5. Use explicit file paths, dates, thresholds, and owners.
6. Point to canonical policy instead of repeating it.
7. Remove filler.
8. Keep long narrative only when compression would hide important nuance.

Formatting rules:
1. Tables are the default scannability tool.
2. Use Mermaid, screenshots, charts, SVGs, or HTML prototypes only when they clarify faster than prose.
3. Keep visual semantics portable; do not rely on inline color meaning.
4. Keep stable headings and section names.
5. Keep structured companions (`json`, `csv`, `tsv`, `yaml`) compact and current.

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

Testing and evidence rules:
1. `Documentation/testing/README.md` is the active testing-doc classification index.
2. Keep reusable guides and stable harness contracts active in `Documentation/testing/`.
3. Keep BL-specific QA docs active only while they add value beyond runbooks and `TestEvidence/`.
4. `TestEvidence/README.md` is the evidence classification index.
5. `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md` are the only always-canonical evidence docs.
6. Prefer packet directories over loose top-level files.
7. Archive one-off notes, heavy packets, and retired summaries under `Documentation/archive/<YYYY-MM-DD>-<slug>/`.

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
6. Machine-readable automation eligibility may live in `Documentation/backlog/automation-contracts.json` when item-by-item orchestration needs a stable contract.

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
   - active/open runbooks (`Documentation/backlog/bl-*.md`, `Documentation/backlog/hx-*.md`) must satisfy the current runbook schema and cadence policy;
   - done runbooks (`Documentation/backlog/done/*.md`) must preserve closeout evidence while conforming to readability schema;
   - backlog support ledgers (`Document Type: Backlog Support`) are exempt from runbook schema fields but must preserve canonical runbook linkage.
7. Per `ADR-0023`, backlog automation is draft-only by default.
8. Automation may run declared T1/T2/T3 commands and assemble draft `TestEvidence/` packets or sync summaries.
9. Automation may not finalize status changes, `Done` transitions, or archive moves without owner confirmation unless an item is explicitly marked `owner_gated_auto`.
10. Automation contracts should declare `automation_mode`, `automation_stage_cap`, `owner_required_for`, `heavy_wrapper`, `shared_files_risk`, and `closeout_ready`.

## Backlog Plain-Language and Visual Clarity Standard

Applies to backlog lifecycle documents:
- `Documentation/backlog/_template-intake.md`
- `Documentation/backlog/_template-runbook.md`
- `Documentation/backlog/_template-closeout.md`
- `Documentation/backlog/_template-promotion-decision.md`
- open runbooks (`Documentation/backlog/bl-*.md`, `Documentation/backlog/hx-*.md`)
- done runbooks (`Documentation/backlog/done/*.md`)

Required sections:
1. `## Plain-Language Summary`
2. `## 6W Snapshot (Who/What/Why/How/When/Where)`
3. `## Visual Aid Index`

Visual policy:
1. Use visuals only when they improve understanding.
2. Prefer compact tables first.
3. Add mermaid diagrams, screenshots, images, or charts only when they reduce ambiguity better than prose.
4. Link visual evidence to repo-local artifacts (`TestEvidence/...`) when applicable.

Validation:
1. `./scripts/validate-backlog-plain-language.sh`
2. `./scripts/validate-backlog-redundancy.py`
3. `./scripts/export-backlog-summaries.py --check`
4. `./scripts/validate-docs-freshness.sh`

Key helpers:
1. `./scripts/new-backlog-item.py`
2. `Documentation/backlog/runbook-authoring-guide.md`
3. `Documentation/backlog/backlog-summary-schema.md`
4. `scripts/backlog-auto-123.py`
5. `Documentation/backlog/automation-draft-flow.md`
6. `scripts/backlog-closeout-draft.py`

## Portable Status-Rich Roadmap And Review Standard

Applies to any markdown document that serves as a live prioritization, roadmap, review, or execution-tracking surface, including:
- architecture/code reviews,
- backlog runbooks,
- closeout docs,
- execution summaries with active remaining work.

Status formatting rules:
1. Use portable markdown only. Do not rely on inline HTML/CSS color for state because renderer support is inconsistent.
2. Prefer these status tags:
   - `[DONE]`
   - `[ACTIVE]`
   - `[NEXT]`
   - `[QUEUED]`
   - `[DEFERRED]`
   - `[BLOCKED]`
3. Use `~~strikethrough~~` on completed item names when the named task/slice is fully complete and no longer active.
4. When a document is used for prioritization or progress tracking, include a scannable status surface such as:
   - `## Status Legend` or `## Review Status Legend`
   - `## Priority Snapshot`, `## Progress Snapshot`, or `## Completion Snapshot`
5. Snapshot tables should include, at minimum:
   - item/work package,
   - status,
   - priority,
   - estimate,
   - actual/time,
   - tokens (or `n/a`),
   - updated/completed date,
   - location/files or scope,
   - remaining work or evidence pointer.
6. Never invent effort telemetry. If exact wall-clock time or token usage was not logged, use `not logged` or `n/a`.
7. Prefer tables first. Use Mermaid only when it reduces ambiguity better than prose plus tables.

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
- `README.md`
- `CHANGELOG.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

When the same closeout also changes backlog status/priority, update `Documentation/backlog/index.md` in that change set.

## Validation Logging
- Snapshot: update `TestEvidence/build-summary.md` after meaningful build/test runs.
- Trend: append an entry to `TestEvidence/validation-trend.md` for each meaningful run.

## API Documentation
- Doxygen is the preferred API doc generator for C++ source comments.
- Use Doxygen-style comments (`/** ... */`) for public classes and nontrivial methods.
- Recommended command: `doxygen Documentation/Doxyfile`

## Minimalism Rule
Prefer updating canonical docs over creating new files. New docs require a clear owner and purpose.
When a doc must stay detailed, keep the active surface short and move deep narrative or historical context into evidence packets, appendices, or archive bundles.
Apply the same rule to testing docs: prefer short guide or contract surfaces in `Documentation/testing/` and keep bulky run output in `TestEvidence/` or archive.

## Artifact Tracking Rule
Apply artifact tracking and retention policy from `Documentation/adr/ADR-0010-repository-artifact-tracking-and-retention-policy.md`:
1. classify by artifact class first;
2. keep generated/heavy artifacts local-only by default;
3. track only canonical decision-grade evidence.
4. keep only the current generated version in active folders;
5. archive older generated summaries, visuals, and timestamped support packets quickly;
6. keep active markdown short and pair it with one compact structured companion when needed;
7. do not treat debug exhaust or repeated payload-failure snippets as active documentation.

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
3. Historical note: `Documentation/runbooks/backlog-execution-runbooks.md` was a procedural companion and is now superseded by `Documentation/backlog/index.md` plus individual runbooks.

## Closeout Sync Snapshot (2026-02-28)

1. BL-023 done-transition moved runbook authority from `Documentation/backlog/bl-023-resize-dpi-hardening.md` to `Documentation/backlog/done/bl-023-resize-dpi-hardening.md`.
2. Backlog catalog authority was synchronized in `Documentation/backlog/index.md` in the same change set.
3. Canonical promotion evidence remains repo-local under `TestEvidence/bl023_slice_a2_t3_promotion_20260228T201500Z/`.

## Closeout Sync Snapshot (2026-03-03)

1. BL-057 done-transition moved runbook authority from `Documentation/backlog/bl-057-device-preset-library.md` to `Documentation/backlog/done/bl-057-device-preset-library.md`.
2. Backlog catalog authority was synchronized in `Documentation/backlog/index.md` in the same change set.
3. Canonical promotion evidence is repo-local under:
   - `TestEvidence/bl057_candidate_t2_closeout/`
   - `TestEvidence/bl057_promotion_t3_closeout/`.

## Architecture Consolidation Snapshot (2026-03-01)

1. Cross-system architecture source-of-truth is consolidated in `ARCHITECTURE.md`.
2. Prior standalone architecture reviews were archived under `Documentation/archive/2026-03-01-architecture-review-consolidation/`.
3. Any future architecture review with durable value must be merged into `ARCHITECTURE.md` and archived in the same change set.
