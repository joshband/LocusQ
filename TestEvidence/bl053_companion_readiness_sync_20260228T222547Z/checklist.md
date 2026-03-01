Title: BL-053 Companion Readiness + Sync Checklist
Document Type: Test Checklist
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-03-01

# BL-053 Companion Readiness + Sync Checklist

## Preconditions
- AirPods connected to macOS.
- Companion running from `/Applications/LocusQ Headtrack Companion.app`.
- LocusQ plugin open in DAW calibrate mode.

## Steps
1. With AirPods in case, confirm `Readiness` shows `disabled_disconnected` and `Send Gate` is `closed`.
2. Remove AirPods but keep out of ear, confirm `Readiness` shows `active_not_ready` and sync button remains disabled.
3. Insert AirPods in ear and hold still until `Readiness` becomes `active_ready` and `Baseline State` is `locked_*`.
4. Press `Center / Sync`; confirm `Send Gate` transitions to `open` and status becomes `ACTIVE`.
5. Motion sanity:
   - Turn head right -> model turns right.
   - Look down -> forward vector tilts down.
   - Top view (`T`) should preserve left/right yaw semantics.

## Expected Outcome
- No pose streaming while disconnected/not-ready.
- Streaming begins only when ready and synced.
- No recurring startup 45-degree offset after proper ready+sync flow.
