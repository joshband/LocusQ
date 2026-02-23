Title: BL-011 CLAP Playbook
Document Type: Reference
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# BL-011 Objective

Add CLAP format support with deterministic adapter behavior, CI/test lane coverage, and release-ready documentation.

# Execution Order

1. Baseline and branch safety.
   - Confirm current dirty tree scope (`git status --short`).
   - Locate current format configuration in `CMakeLists.txt`.
   - Keep changes scoped to CLAP enablement and validation.
2. Build integration.
   - Wire `clap-juce-extensions` in build system.
   - Keep feature flag / optional dependency semantics explicit.
   - Verify target discovery:
     - `cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release`
     - `cmake --build build_local --config Release --target help | rg -i "clap|locusq"`
3. Adapter integration.
   - Add CLAP adapter boundary where host events are converted to internal events.
   - Keep core renderer/engine independent from CLAP host headers.
   - Align with `Documentation/plans/LocusQClapContract.h`:
     - immutable capability negotiation per session
     - deterministic runtime mode selection
     - bounded voice/mod event capacities
     - lock-free telemetry bridge
4. QA and host validation.
   - Validate `.clap` artifact generation.
   - Run CLAP-aware checks (`clap-validator`, `clap-info`) when available.
   - Run existing regression lanes (`pluginval`, harness scenarios) to detect non-CLAP regressions.
5. Closeout docs and backlog.
   - Update BL-011 status only after validation evidence exists.
   - Update `TestEvidence/build-summary.md`, `TestEvidence/validation-trend.md`, and routing docs if skill mapping changed.

# File-Level Change Map (Expected)

- Build/config:
  - `CMakeLists.txt`
  - optional `cmake/` helpers if introduced
- Processor/adapter:
  - `Source/PluginProcessor.cpp`
  - `Source/PluginProcessor.h`
  - `Source/SpatialRenderer.h` only if format-agnostic contract integration is required
- Documentation/evidence:
  - `Documentation/backlog-post-v1-agentic-sprints.md`
  - `TestEvidence/build-summary.md`
  - `TestEvidence/validation-trend.md`

# Red Flags

- CLAP-specific types leaking into core DSP classes.
- Runtime behavior branching on host name/version.
- Any allocation or lock added inside `processBlock()`.
- Missing fallback behavior when CLAP extensions are partially unavailable.
