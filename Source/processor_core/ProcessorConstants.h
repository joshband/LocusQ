#pragma once

//==============================================================================
// Processor-internal constants shared across compilation units.
// Extracted from PluginProcessor.cpp anonymous namespace (W0-A / BL-032).
//==============================================================================

namespace locusq::constants
{
    constexpr const char* kSnapshotSchemaProperty        = "locusq_snapshot_schema";
    constexpr const char* kSnapshotSchemaValueV2         = "locusq-state-v2";
    // V3 (BL-056): marks companion-calibration-profile handoff contract present.
    // V2 → V3 migration is transparent: no new mandatory state fields.
    // Old V2 states are loaded cleanly by the existing hasProperty guards.
    constexpr const char* kSnapshotSchemaValueV3         = "locusq-state-v3";
    constexpr const char* kSnapshotOutputLayoutProperty  = "locusq_output_layout";
    constexpr const char* kSnapshotOutputChannelsProperty = "locusq_output_channels";
    constexpr const char* kSceneSnapshotSchemaProperty   = "locusq-scene-snapshot-v1";
    constexpr int kMaxSnapshotOutputChannels   = 16;
    constexpr int kSceneSnapshotCadenceHz      = 30;
    constexpr int kSceneSnapshotStaleAfterMs   = 750;
} // namespace locusq::constants
