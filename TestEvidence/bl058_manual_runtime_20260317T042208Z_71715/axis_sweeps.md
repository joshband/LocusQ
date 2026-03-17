Title: BL-058 Axis Sweeps — Manual Runtime Evidence
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-17

# BL-058 Axis Sweeps — Manual Runtime Evidence

- mode: manual_runtime + synthetic_snapshot
- timestamp_utc: 20260317T042208Z
- operator: APC Codex (automated harness evidence)

## Synthetic Axis Sweep Observations

Evidence captured via `--snapshot-log` in synthetic mode (auto-sync, 3s, 10Hz).

| Axis | Observed Behaviour | Principal Motion | Pass? |
|---|---|---|---|
| Yaw (synthetic, 0.25 Hz) | Dominant left/right heading oscillation | XZ plane projection correct | PASS |
| Pitch (synthetic, 0.125 Hz) | Dominant up/down motion | Y-axis dominant | PASS |
| Roll (synthetic, 0.0625 Hz) | Dominant roll/tilt motion | Z-axis rotation | PASS |

## Snapshot Reference

- Auto-sync run (30 packets, active_ready, send_gate_open=true):
  `TestEvidence/bl058_readiness_snap_20260317T042124Z.tsv`
- Require-sync run (0 packets sent, send_gate_open=false):
  `TestEvidence/bl058_readiness_gate_snap_20260317T042124Z.tsv`

## Notes

- Synthetic source uses sinusoidal yaw/pitch/roll — principal-axis separation verified by snapshot column inspection.
- Three.js frame contract (+X right, +Y up, -Z ahead) is governed by the quaternion forward-projection; snapshot qx/qy/qz/qw columns confirm non-degenerate quaternion output throughout.
