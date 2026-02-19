Title: LocusQ Lessons Learned
Document Type: Lessons Learned Log
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-19

# Lessons Learned

| Date (UTC) | Context | Lesson | Action |
|---|---|---|---|
| 2026-02-18 | Phase 2.5 start | Implementation status can drift from executed code unless phase-state updates happen in the same change set. | Treat `status.json`, `build-summary`, and `validation-trend` updates as mandatory completion gates for `/impl` and `/test`. |
| 2026-02-19 | Phase 2.6 validation | QA adapter selection materially changes outcomes; running emitter RT-safety scenarios with forced `--spatial` can produce misleading allocation failures. | Standardize canonical smoke command without `--spatial`, and run dedicated spatial scenarios (`locusq_26_animation_internal_smoke.json`, Phase 2.5 acceptance suite) separately. |
