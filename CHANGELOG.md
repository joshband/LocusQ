Title: LocusQ Changelog
Document Type: Changelog
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-03-01

# Changelog

All notable changes to LocusQ are documented here.

## [Unreleased]

Operational snapshot:
- Live backlog authority: `Documentation/backlog/index.md`
- Canonical runtime/state authority: `status.json`

### Added

- Specialist execution skills for active lanes:
  - `steam-audio-capi`, `clap-plugin-lifecycle`, `spatial-audio-engineering`
  - `headtracking-companion-runtime`, `hrtf-rendering-validation-lab`, `perceptual-listening-harness`
  - `documentation-hygiene-expert` for repo-scale documentation cleanup and freshness ownership.
- Backlog execution expansion for post-v1 delivery orchestration:
  - `Documentation/backlog-post-v1-agentic-sprints.md`
- Git artifact hygiene automation surfaces:
  - `scripts/git-artifact-hygiene-audit.sh`
  - `scripts/git-artifact-hygiene-guard.sh`
  - `scripts/git-artifact-cleanup-index.sh`
  - `scripts/install-git-hygiene-hooks.sh`
  - `.github/workflows/git-artifact-hygiene.yml`
  - `.githooks/pre-commit`

### Changed

- Documentation skill ownership split is explicit and normalized:
  - `documentation-hygiene-expert` owns cleanup, dedupe, simplification, freshness ownership, and stale comment/API-doc hygiene.
  - `skill_docs` owns governance metadata, ADR/invariant traceability, standards/tier enforcement, and routing-contract parity.
- Root routing/governance contracts were synchronized:
  - `AGENTS.md`, `CODEX.md`, `CLAUDE.md`, `SKILLS.md`, `AGENT_RULE.md`, `Documentation/skill-selection-matrix.md`.
- Documentation cleanup/compaction pass completed:
  - `Documentation/README.md`, `Documentation/standards.md`, `README.md` deduped and simplified.
  - Evidence surfaces compacted with history preserved in archives:
    - `Documentation/archive/2026-03-01-build-summary-compaction/build-summary-legacy-2026-03-01.md`
    - `Documentation/archive/2026-03-01-validation-trend-compaction/validation-trend-legacy-2026-03-01.md`
- Skill-runtime markdown exemption alignment (Codex + Claude):
  - Standard documentation governance passes now exempt `.codex/*` and `.claude/*` skill/workflow/rule markdown unless explicitly requested.
  - `scripts/validate-docs-freshness.sh` now prunes runtime skill/workflow/rule markdown paths from metadata-freshness checks.
- Skill routing and references now map git artifact hygiene intent to `documentation-hygiene-expert` for both Codex and Claude.

### Fixed

- BL-043 FDN sample-rate integrity (P0):
  - Delay times are now invariant in milliseconds across `44.1k/48k/96k/192k`.
  - QA parity sweep added: `scripts/qa-bl043-fdn-samplerate-sweep-mac.sh`.
  - Canonical done runbook: `Documentation/backlog/done/bl-043-fdn-sample-rate-integrity.md`.

### Recent Done Promotions

- BL-023, BL-052, BL-042, BL-044, BL-046, BL-047, BL-048, BL-049 moved to `Done` with synchronized backlog/archive/evidence updates.
- BL-030 release-governance RL-09 closeout captured; RL-05 authoritative closure recorded in owner sync evidence packets.
- BL-013 and BL-017 done promotions completed with promotion-decision packets.

## [v1.0.0-ga] - 2026-02-20

- Initial GA release baseline for LocusQ established with spatial renderer, host/runtime integration, QA harness lanes, and packaging/release foundations.
- See archive for full detailed implementation and validation narrative.

## Legacy Changelog Archive

The full pre-compaction changelog history was archived on 2026-03-01 at:
- `Documentation/archive/2026-03-01-changelog-compaction/changelog-legacy-2026-03-01.md`

Use the archive for deep historical chronology; keep this file concise and current.
