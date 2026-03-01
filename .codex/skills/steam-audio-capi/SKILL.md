---
name: steam-audio-capi
description: Integrate and validate Steam Audio C API runtime paths (dynamic loading, context/HRTF/effects lifecycle, fallback behavior, and BL-009 style headphone rendering checks).
---

Title: Steam Audio C API Integration Skill
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-21
Last Modified Date: 2026-03-01

# Steam Audio C API

Use this skill when implementing or debugging Steam Audio runtime integration in LocusQ, especially BL-009 class work (headphone binaural path with deterministic fallback).

## Scope
- CMake gating for optional Steam Audio builds.
- Runtime dynamic loading (`libphonon`/`phonon.dll`) with safe fallback.
- `prepare`/`reset`/teardown ownership for context + HRTF + effect objects.
- Realtime-safe block processing (no allocation/lock on audio thread).
- UI/scene telemetry and selftest gates for requested vs active mode.

## Workflow
1. Verify SDK baseline from `third_party/steam-audio/sdk/steamaudio/include/phonon.h` and `phonon_version.h`.
2. Gate compilation via `LOCUSQ_ENABLE_STEAM_AUDIO`; do not force hard link dependency for fallback builds.
3. Resolve runtime library path from env first, then compile-time default path, then system loader fallback.
4. Initialize Steam objects only outside the audio thread (typically `prepareToPlay` path):
   - context
   - HRTF
   - effect (prefer virtual surround for quad -> stereo headphone rendering)
5. In audio processing, reuse preallocated buffers and fallback to existing stereo downmix on any unavailable/failure path.
6. Keep telemetry explicit (`requested` mode vs `active` mode + availability) for UI assertions.
7. Validate with production selftest plus optional BL-009 assertion gate.
8. For BL-053 class orientation work, verify requested vs active path coherence.
   - Orientation pointer is present at call boundary.
   - Orientation pointer is consumed in active monitoring render path.
   - Fallback/activation telemetry clearly indicates active mode and fallback reason.

## Realtime Rules
- Never allocate/free on audio thread.
- Never block on dynamic loading or file IO in audio thread.
- Never assume runtime library availability even when compiled with Steam option on.
- On failure, fall back immediately and keep output deterministic.

## Orientation Consumption Checks (BL-053+)
- Verify orientation parameter presence in both caller and cal-monitor render entry points.
- Verify no dead-path bypass (for example ignore-unused orientation) in active `virtual_binaural` monitoring render.
- Verify stale/disconnect behavior falls back to deterministic identity orientation without glitches.

## Validation Commands
- Build with Steam path enabled:
  - `cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release -DLOCUSQ_ENABLE_STEAM_AUDIO=ON`
  - `cmake --build build_local --config Release --target LocusQ_Standalone -j 8`
- Run production selftest:
  - `scripts/standalone-ui-selftest-production-p0-mac.sh`
- Run BL-009 opt-in assertion:
  - `LOCUSQ_UI_SELFTEST_BL009=1 scripts/standalone-ui-selftest-production-p0-mac.sh`

## References
- `references/sources.md`
