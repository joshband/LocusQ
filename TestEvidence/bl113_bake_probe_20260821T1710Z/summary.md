Title: BL-113 BakeRecorder Probe Evidence
Document Type: Test Evidence Summary
Author: Codex
Created Date: 2026-08-21
Last Modified Date: 2026-08-21

# BL-113 BakeRecorder Probe Evidence

The standalone probe passed all 13 checks on three consecutive executions. It covers transport lifecycle, paused and out-of-range behavior, six-track export schema, endpoint and time ordering, interpolation tolerance against every raw capture sample, idempotent export, reset/rebake behavior, and zero ordinary heap allocations inside worker-thread `tick()` calls.

The implementation decision is: `bake_kf_density` is a target spacing, while `bake_curve_fit_tolerance` is a hard raw-sample interpolation bound. Adaptive refinement may therefore emit more keyframes than the target density when motion requires it.

The direct GCC/JUCE-core lane passed. The repository CMake entry point reached JUCE's `juceaide` bootstrap but could not complete in this container because the image lacks `X11/Xlib.h`. The new probe target itself is independent of the private QA harness; canonical CMake/CTest replay remains an environment-only follow-up and no BL-113 owner promotion is claimed here.

Assumption: the bake window uses a stable BPM for PPQ-to-seconds conversion. Tempo-map-aware bake timing remains outside this step.
