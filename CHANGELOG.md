Title: LocusQ Changelog
Document Type: Changelog
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-03-20

# Changelog

All notable changes to LocusQ are documented here.

## [Unreleased]

Operational snapshot:
- Live backlog authority: `Documentation/backlog/index.md`
- Canonical runtime/state authority: `status.json`

### Added

- BL-088: HostRunner live VST3 integration test — `#ifdef LOCUSQ_HOST_RUNNER_LIVE` test case and CMake option added to `audio-dsp-qa-harness`; QA script contract 6/6 + execute 2/2 + live 13/13 PASS against real `LocusQ.vst3`
- BL-020: Confidence/masking overlay contract — deterministic combinedConfidence formula, fallback codes, and degradation policy confirmed; QA lane 3/3 PASS
- BL-021: Room-story overlay contract — five modes, degraded/stale-hold states, additive composite policy confirmed; QA lane 3/3 PASS

### Added

- BL-113 CL-P2: FormationSystem — 7 formation geometry types (Line/Arc/Circle/Grid/Spiral/SphereSurface/Custom), morph animation (loop/pingpong at configurable Hz rate), spread delta (avgPairwiseDist/maxDist clamped [0..1]) published to `ChoreographyOffset.spreadDelta`; 16 new APVTS params in `emitter_choreography` group; PhysicsWorker tick now passes active emitter count to `compute()`; build clean, RT non_allowlisted=0.
- BL-113 CL-P1: ChoreographyWorker infrastructure — `AudioRingBuffer` (SPSC lock-free ring, 4096×2 pre-allocated), `ChoreographyWorker` (ADR-0020 Layer 3 colocated in PhysicsWorker tick), `ChoreographyOffset` per-emitter additive struct; `composedRestPos` wiring in spring/containment forces; `choro_enable` APVTS param registered, wired, and traced; audio ring push in Emitter mode; build clean, RT audit non_allowlisted=0.
- BL-112: Choreography Lab authority and parameter contract accepted — `choro_*`/`bake_*` namespace frozen; traceability gate (parameter-spec + implementation-traceability updates required before implementation claims advance); production-candidate (BL-113/BL-114) vs lab-only (BL-115) promotion boundary explicit; BL-113..BL-116 unblocked.
- BL-111: Three-mode UI consistency and overflow audit complete — defect matrix captured for CALIBRATE/EMITTER/RENDERER at launch size; five fix slices landed (CALIBRATE acknowledgment coupling, shared help triggers, RENDERER authority condensation, disclosure rhythm, renderer polish); canonical selftest and standalone smoke stayed green throughout.

### Changed

- BL-106 is now fully closed out:
  - runbook moved into `Documentation/backlog/done/`.
  - canonical wrapper packet recorded at `TestEvidence/bl106_validation_20260319T223047Z/`.
  - backlog authority, status, and evidence summaries are synchronized.
- BL-102 is now fully closed out:
  - runbook moved into `Documentation/backlog/done/`.
  - backlog authority, status, and evidence summaries are synchronized.
  - owner sync packet recorded at `TestEvidence/bl102_owner_sync_z1_20260319T215518Z/`.
- BL-032: Source modularization complete — PluginProcessor/PluginEditor decomposed into `processor_core`, `processor_bridge`, `shared_contracts`, `editor_shell`, `editor_webview` modules; LOC guardrails and RT audit green (PluginProcessor.cpp 3248 ≤ 3600 LOC, RT audit non_allowlisted=0)

### Added

- Architecture review W1-B hardening: triple-buffered RT keyframe timeline snapshots, `juce::Thread` physics cadence, and fixed-size sequence-safe headphone diagnostics publication.
- Architecture review W2-B WebView UX hardening: branded boot shell/loading skeleton, deduped warning/error toasts, and a compact calibration status dock for degraded/native-bridge states.
- Architecture review W2-C format-lane expansion: dedicated CLAP and AUv3 CI jobs plus clean-bundle npm dev-dependency install coverage for fresh WebView builds.
- Architecture review W2-D calibration portability: native JSON profile export/import with async file choosers, compatibility checks, and WebView library controls.
- Architecture review W3-C accessibility hardening: keyboard navigation, ARIA semantics, focus-visible states, and viewport keyboard nudging for the production WebView UI.

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

- BL-080 and BL-089 through BL-094 are now fully closed out:
  - runbook/archive paths, backlog authority, and evidence summaries are synchronized.
  - BL-080 moved into `Documentation/backlog/done/`.
  - BL-089 through BL-094 now read as formal Done in the master backlog index.
- BL-076 advanced through the W0-B header/body split: `Source/SpatialRenderer.cpp` now owns the out-of-line renderer implementation, `Source/SpatialRenderer.h` is reduced to a bounded declaration surface, and refreshed contract/execute guardrail evidence is recorded for 2026-03-06.
- BL-036 was archived to `Documentation/backlog/done/` after an explicit scope split moved its remaining runtime implementation work into new follow-on BL-078.
- BL-041 was archived to `Documentation/backlog/done/` after its BL-036 dependency cleared; contract/execute parity evidence and closeout sync remain green.
- BL-037 remains archived to `Documentation/backlog/done/` from the same mainline promotion sequence.
- Backlog promotion sync on `main` archived BL-035, BL-038, and BL-051 to `Documentation/backlog/done/`.
- BL-032 was rechecked for the same promotion pass and retained at done-candidate because `BL032-G-001` is still failing (`Source/PluginProcessor.cpp` `3653 > 3600`) while RT audit remains green.
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

- BL-102 moved to `Done` with owner sync packet `TestEvidence/bl102_owner_sync_z1_20260319T215518Z/` and closeout sync on 2026-03-20.
- BL-080, BL-089, BL-090, BL-091, BL-092, BL-093, and BL-094 moved to `Done` with closeout sync on 2026-03-18.
- BL-036, BL-037, and BL-041 moved to `Done` with runbook archive/index/status/evidence sync.
- BL-035, BL-038, and BL-051 moved to `Done` with runbook archive/index/status/evidence sync.
- BL-050, BL-069, and BL-070 moved to `Done` with runbook archive/index/status/evidence sync.
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
