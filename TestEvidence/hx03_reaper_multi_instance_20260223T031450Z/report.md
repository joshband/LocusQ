Title: HX-03 REAPER Multi-Instance Stability Report
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# HX-03 REAPER Multi-Instance Stability (20260223T031450Z)

- overall: `pass`
- instance_count_per_phase: `3`
- phase_count: `2` (clean_cache, warm_cache)
- start_stagger_sec: `1`
- skip_install: `0`
- require_locusq: `1`
- bootstrap_timeout_sec: `45`
- render_timeout_sec: `90`
- check_crash_reports: `1`
- pass_count: `26`
- warn_count: `0`
- fail_count: `0`

## Artifacts

- `status.tsv`
- `clean_cache/instance_*.log`
- `clean_cache/instance_*_status.json`
- `clean_cache/new_crash_reports.txt`
- `warm_cache/instance_*.log`
- `warm_cache/instance_*_status.json`
- `warm_cache/new_crash_reports.txt`
- `clean_cache_build_install.log` (unless install skipped)
