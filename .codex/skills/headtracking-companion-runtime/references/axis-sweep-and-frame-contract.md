Title: Companion Axis Sweep And Frame Contract
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Axis Sweep And Frame Contract

Canonical frame assumptions:
- `+X`: right
- `+Y`: up
- `-Z`: ahead

Synthetic sweep checks:
1. Pure yaw -> dominant left/right heading motion.
2. Pure pitch -> dominant up/down tilt motion.
3. Pure roll -> dominant shoulder-tilt motion.

Failure signatures:
- yaw inversion,
- pitch mapped to lateral drift,
- roll inversion,
- stale pose still animating view.
