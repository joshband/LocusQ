# LocusQ Signal Path (Phase 1 Extraction)

## Scope
This document traces the runtime audio path from `LocusQAudioProcessor::processBlock()` to host output and identifies high-confidence reduction candidates.

## Canonical Runtime Path

### Pre-flight (all modes)
`AudioProcessor::processBlock`
→ clear non-used output channels
→ `visualTokenScheduler.processBlock(...)`
→ bypass gate (`bypass` APVTS param)
→ mode transition prep/crossfade guards
→ mode switch (`Calibrate` / `Emitter` / `Renderer`)

### Calibrate mode
Input
→ `calibrationEngine.processBlock(buffer, micChannel)` (active calibration states only)
→ `applyCalibrationMonitoringPath(...)` (optional binaural monitor fold-down)
→ output

### Emitter mode
Input
→ publish source buffer into `sceneGraph` slot
→ `ChoreographyWorker::pushAudioBlock(...)` (analysis side path)
→ `publishEmitterState(...)` (state publication side path)
→ output passthrough (unchanged audio)

### Renderer mode
Input (ignored after mode entry)
→ `prepareRendererRealtimeStateForBlock()`
→ `buffer.clear()`
→ `spatialRenderer.process(buffer, sceneGraph)`
→ finite/denormal/clamp guardrail pass
→ speaker RMS telemetry smoothing
→ confidence-masking diagnostics publish
→ output

### Post-mode finalization (all modes)
mode transition crossfade (if needed)
→ confidence diagnostics publish
→ `sceneGraph.advanceSampleCounter(...)`
→ calibration latency host-report (`setLatencySamples` on change)
→ output

## High-Confidence Reduction Candidates (No behavior change in this phase)

### Candidate for removal or collapse
- Multiple diagnostics/publication writes embedded in `processBlock` can move into dedicated non-owning helper functions without changing DSP order.
- Repeated APVTS reads inside mode-specific logic should be consolidated into explicit per-block snapshots.
- Confidence-masking calculations currently co-reside with renderer audio path and can be isolated behind a pure helper for traceability.

### Candidate for gate
- Experimental / advanced diagnostics paths (confidence masking overlays) are candidates for compile-time gating once inventory is completed.

## File impact summary (Phase 1 only)
- Deleted files: none
- Modified files: none
- Added files: `Documentation/SIGNAL_PATH.md`

## Validation status
- Status: partially tested
- Method: static trace extraction from `Source/PluginProcessor.cpp` and related call sites (no behavior changes made in this phase).
