Title: LocusQ Lessons Learned
Document Type: Lessons Learned Log
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-03-18

# Lessons Learned

| Date (UTC) | Context | Lesson | Action |
|---|---|---|---|
| 2026-02-18 | Phase 2.5 start | Implementation status can drift from executed code unless phase-state updates happen in the same change set. | Treat `status.json`, `build-summary`, and `validation-trend` updates as mandatory completion gates for `/impl` and `/test`. |
| 2026-02-19 | Phase 2.6 validation | QA adapter selection materially changes outcomes; running emitter RT-safety scenarios with forced `--spatial` can produce misleading allocation failures. | Standardize canonical smoke command without `--spatial`, and run dedicated spatial scenarios (`locusq_26_animation_internal_smoke.json`, Phase 2.5 acceptance suite) separately. |
| 2026-03-18 | Documentation hygiene audit | Machine-readable backlog exports can drift even when human-readable backlog docs still pass readability and redundancy gates. | Treat `Documentation/reports/data/backlog-summary.json` and `.csv` as freshness-critical generated artifacts and refresh them in the same change set as backlog edits. |
| 2026-03-18 | Documentation hygiene audit | Planning reports, review packets, testing narratives, and backlog runbooks can become parallel status systems if they are not retired quickly enough. | Keep status authority in `Documentation/backlog/index.md`, `status.json`, and `TestEvidence/*`; archive or pointer-reduce derivative packets after their decisions are absorbed. |
| 2026-03-18 | Documentation hygiene audit | Prototype folders stay ambiguous unless the kept surface is curated, not just labeled. | Keep only active prototype code plus a small retained reference set in `Documentation/Calibration POC/`; move exploratory duplicates to archive as soon as canonical docs absorb the useful parts. |
| 2026-03-18 | Documentation hygiene closeout | Archive-first compaction works better than deleting depth: keep the active path short, preserve the old body in archive, and update the manifest in the same change. | Use short active execution or closeout surfaces by default and keep long historical detail only under `Documentation/archive/<date>-<slug>/...` with a clear replacement note. |
| 2026-03-18 | Documentation hygiene closeout | Done runbooks do not need to preserve full execution diaries in the active surface to remain useful. | Keep active done runbooks to status, outcome, evidence pointers, milestone snapshot, and archive note; move replay diaries and long slice histories to archived legacy copies. |
