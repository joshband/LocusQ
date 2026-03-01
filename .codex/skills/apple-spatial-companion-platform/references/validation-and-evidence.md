Title: Apple Companion Validation And Evidence
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Validation And Evidence

## Required checks
- API availability/authorization behavior is explicit and failure-safe.
- Capture pipeline fallback behavior is deterministic and logged.
- Privacy guardrails are verified (no unexpected persistence/network use).
- BL-058 readiness/sync gating is still respected after capture-path changes.

## Suggested evidence packet
- `status.tsv`
- `results.tsv`
- `privacy_contract_check.md`
- `capture_quality_checks.tsv`
- `fallback_matrix.tsv`

## Backlog alignment
- Canonical status authority: `Documentation/backlog/index.md`
- Runbook details:
  - `Documentation/backlog/bl-057-device-preset-library.md`
  - `Documentation/backlog/bl-058-companion-profile-acquisition.md`
