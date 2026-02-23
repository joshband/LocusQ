Title: LocusQ Root README
Document Type: Project README
Author: APC Codex
Created Date: 2026-02-19
Last Modified Date: 2026-02-23

# LocusQ

LocusQ is a JUCE 8 spatial-audio plugin with a WebView UI, focused on deterministic runtime behavior, reproducible validation lanes, and documented phase contracts.

## Current Posture (UTC 2026-02-23)

- Version target: `v1.0.0-ga`
- Active phase: `code`
- UI framework gate: `webview`
- Canonical backlog/spec: `Documentation/backlog-post-v1-agentic-sprints.md`
- Canonical state surface: `status.json`
- Recent closeout: `BL-015` is `Done` (2026-02-23) with refreshed all-emitter baseline evidence (`TestEvidence/locusq_production_p0_selftest_20260223T034704Z.json`, `TestEvidence/locusq_smoke_suite_spatial_bl015_20260223T034751Z.log`). `BL-011`, `BL-016`, and `BL-010` remain `Done` with their validated evidence bundles.
- Hardening closeout: `HX-01` is `Done` (2026-02-23) after migrating SceneGraph shared_ptr publication to `Source/SharedPtrAtomicContract.h` with passing build/smoke evidence (`TestEvidence/hx01_sharedptr_atomic_build_20260223T034848Z.log`, `TestEvidence/hx01_sharedptr_atomic_qa_smoke_20260223T034918Z.log`).

## Quick Start (macOS)

1. Build and install plugin binaries:
   - `./scripts/build-and-install-mac.sh`
2. Optional standalone app install:
   - `LOCUSQ_INSTALL_STANDALONE=1 ./scripts/build-and-install-mac.sh`
3. Run primary UI gate:
   - `./scripts/ui-pr-gate-mac.sh`
4. Run production P0 self-test lane:
   - `./scripts/standalone-ui-selftest-production-p0-mac.sh`
5. Run docs freshness gate before closeout:
   - `./scripts/validate-docs-freshness.sh`

## Validation Ladder

Use the smallest meaningful lane first, then broaden:

1. Build/install
   - `./scripts/build-and-install-mac.sh`
2. UI gate
   - `./scripts/ui-pr-gate-mac.sh`
3. Production P0 UI self-test
   - `./scripts/standalone-ui-selftest-production-p0-mac.sh`
4. Host smoke (optional, deeper)
   - `./scripts/reaper-headless-render-smoke-mac.sh`
5. Docs contract
   - `./scripts/validate-docs-freshness.sh`

## Release and Docs Sync Contract

When acceptance or status claims change, update these in the same change set:

- `status.json`
- `README.md`
- `CHANGELOG.md`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`
- `Documentation/backlog-post-v1-agentic-sprints.md` (if backlog state/priority changed)

## Canonical References

- Backlog and execution spec:
  - `Documentation/backlog-post-v1-agentic-sprints.md`
- CLAP closeout contract (BL-011):
  - `Documentation/plans/bl-011-clap-contract-closeout-2026-02-23.md`
  - `Documentation/plans/LocusQClapContract.h`
- Architecture and planning intent:
  - `.ideas/plan.md`
  - `.ideas/architecture.md`
  - `.ideas/parameter-spec.md`
- Invariants, ADRs, and scene contracts:
  - `Documentation/invariants.md`
  - `Documentation/adr/`
  - `Documentation/scene-state-contract.md`
- Implementation traceability:
  - `Documentation/implementation-traceability.md`
- Validation evidence surfaces:
  - `TestEvidence/build-summary.md`
  - `TestEvidence/validation-trend.md`

## Root Docs Scope

- `README.md`: operator quickstart, current posture, and canonical entrypoints.
- `CHANGELOG.md`: release/history deltas (no live backlog source-of-truth).
- `AGENTS.md`: routing and execution contract.
- `CODEX.md` / `CLAUDE.md`: model-specific behavior deltas from `AGENTS.md`.
- `SKILLS.md`: skill index, triggers, and load order.
- `AGENT_RULE.md`: canonical parity source for `.codex/` and `.claude/` rule copies.

## UI State Screenshots (Build Local)

### CALIBRATE

![LocusQ CALIBRATE state](Documentation/images/readme/locusq-state-calibrate.png)

### EMITTER

![LocusQ EMITTER state](Documentation/images/readme/locusq-state-emitter.png)

### RENDERER

![LocusQ RENDERER state](Documentation/images/readme/locusq-state-renderer.png)

## Historical and Deep-Dive Records

Use these for detailed phase history and long-form evidence:

- `status.json`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/full-project-review-2026-02-20.md`
- `Documentation/archive/2026-02-23-historical-review-bundles/stage14-comprehensive-review-2026-02-20.md`
