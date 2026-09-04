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

## Replacement Rules

- Current canonical design package remains `Design/v3-*` and
  `Design/HANDOFF.md`, which already names v3 as the sole approved version.
- `Design/index.html` (the live browser preview) is unaffected — it was
  already serving the v3 package, not v1/v2.

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
