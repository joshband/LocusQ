Title: Archive Set 2026-09-04 Baseline Reset
Document Type: Archive Manifest
Author: Claude
Created Date: 2026-09-04
Last Modified Date: 2026-09-04

# 2026-09-04 Baseline Reset

## Purpose

Companion archive for the batches tracked in
`Documentation/plans/2026-09-04-baseline-reset-manifest.md`. Each batch that
archives (rather than deletes) content gets its own subfolder here, added as
that batch executes. This is not a one-shot dump — expect more subfolders to
land as later batches from the manifest complete.

## Moved Paths

### Design v1/v2 (Batch B)

- `Design/v1-style-guide.md`, `Design/v1-ui-spec.md`, `Design/v1-test.html`
- `Design/v2-style-guide.md`, `Design/v2-ui-spec.md`, `Design/v2-test.html`

Moved to `Documentation/archive/2026-09-04-baseline-reset/design-v1-v2/`,
unmodified, with git history preserved via `git mv`.

### TestEvidence timestamped-run pruning (Batch D)

Moved 22 of 27 candidate older/duplicate timestamped run directories from
`TestEvidence/` to
`Documentation/archive/2026-09-04-baseline-reset/test-evidence/<original-dirname>`,
keeping the single most-recent run per BL-item/lane prefix in place (44
entries, plus `build-summary.md`/`validation-trend.md`/`README.md`, were
already singletons or the correct "most recent" and were left untouched).

**5 candidates deliberately left in `TestEvidence/` despite being older
duplicates by timestamp**, because each is individually cited by exact path
as the sole evidence for a distinct test mode/phase (not a stale duplicate
of the kept run) in a live doc or `status.json` field:
- `bl054_peq_cascade_rt_integration_20260307T061821Z_73138` and `_73139` —
  the only contract-mode evidence for BL-054 (`Documentation/backlog/done/bl-054-peq-cascade-rt-integration.md:89-90`); the archived/kept `172533Z` runs are execute-mode.
- `bl059_calibration_integration_smoke_20260307T063151Z_93321` — a live
  `status.json` field value (`bl059_integration_smoke_contract_status_tsv`),
  the contract-mode counterpart to the kept execute-mode `_94423` run.
- `bl060_phase_b_listening_20260317T174025Z_90778` and `_20260317T174918Z_99523` —
  cited as the "T1" and "T2 3/3 PASS" evidence in
  `Documentation/reports/data/backlog-summary.{json,csv}` and the BL-060/BL-081
  backlog docs, distinct from the kept latest `_99650` run.

Re-classifying these five as "keep" (rather than updating their citing
docs/status.json to a new archive path) was the lower-risk choice for this
batch. Revisit only if `TestEvidence/` size becomes a problem again.

One older duplicate, `bl040_owner_sync_z11_20260317T044955Z_52916`, is
referenced once from inside an already-archived legacy doc
(`Documentation/archive/2026-03-18-doc-surface-consolidation/backlog/bl-040-ui-modularization-and-authority-status-legacy.md`)
— archived anyway since that citing doc is itself frozen historical content,
not a live authority surface.

`TestEvidence/archive/` (a pre-existing archive location with its own older
manifest files) was left alone — out of scope for this sweep.

## Replacement Rules

- Current canonical design package remains `Design/v3-*` and
  `Design/HANDOFF.md`, which already names v3 as the sole approved version.
- `Design/index.html` (the live browser preview) is unaffected — it was
  already serving the v3 package, not v1/v2.
- Current TestEvidence authority remains `TestEvidence/build-summary.md`,
  `TestEvidence/validation-trend.md`, and `TestEvidence/README.md`, plus the
  44 kept most-recent-per-lane run directories and the 5 re-classified
  mode/phase-specific runs listed above.

## Reason

`Design/HANDOFF.md` has named v3 as the approved package since 2026-02-19;
v1 and v2 were superseded proposals kept alongside it with no ongoing
purpose. Checked before moving: `.codex/skills/design/SKILL.md` and
`.codex/workflows/design.md` both mention "v1, v2, v3" only as a description
of the design skill's own iteration process for *future* design tasks (always
produce three rounds before handoff) — not as a path reference to these
specific files, so nothing there needed updating. A separate `v2` mention in
`.ideas/physics-simulation-impl-plan.md` turned out to reference different,
never-created files (`Design/physics-v1-ui-spec.md`,
`Design/physics-v1-style-guide.md`) — an existing dangling reference
unrelated to this archival, left alone.

## Re-Promotion Rule

Re-promote archived content only if a current canonical or active execution
document explicitly needs it again. Do not restore archived packets to
active folders just because they contain useful historical detail.
