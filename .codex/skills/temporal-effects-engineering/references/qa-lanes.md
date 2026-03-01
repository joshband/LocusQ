Title: Temporal Effects QA Lanes

## Minimum Lanes
1. Deterministic replay lane
   - Fixed signal + fixed automation + fixed transport timeline -> identical output.
2. Runaway safety lane
   - Stress high feedback settings and verify bounded output behavior.
3. Click/zipper lane
   - Rapid parameter transitions must remain artifact-safe.
4. Long-duration soak lane
   - Validate drift, memory stability, and CPU bounds over extended playback.

## Evidence Outputs
- `status.tsv`
- `metrics_summary.tsv`
- `artifacts.md` (notable audio traces and observations)
