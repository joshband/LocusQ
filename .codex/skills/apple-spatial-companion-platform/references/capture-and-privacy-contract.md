Title: Capture And Privacy Contract
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Capture And Privacy Contract

Capture workflow expectations:
1. Guided sequence: left ear, right ear, frontal.
2. Deterministic quality gates (lighting, occlusion, framing, blur).
3. Deterministic fallback if capture or embedding fails.

Privacy expectations:
- No network dependency in capture/matching path.
- No raw image persistence unless explicitly approved by runbook policy.
- Store only minimal profile outputs required for downstream matching (`subject_id`, `sofa_ref`, confidence/fallback metadata).
- Make data-retention behavior visible in operator docs and runbook acceptance criteria.
