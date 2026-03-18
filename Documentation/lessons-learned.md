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
| 2026-03-18 | Documentation hygiene audit | `Documentation/Calibration POC/` is too ambiguous as a mixed active/historical/prototype surface and carries the highest artifact-hygiene risk. | Reclassify it explicitly as active research prototype material or archive/externalize most of it; do not leave it mixed with active product documentation by default. |
