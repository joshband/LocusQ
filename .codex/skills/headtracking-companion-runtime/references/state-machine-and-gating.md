Title: Companion State Machine And Gating Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# State Machine And Gating

## Runtime States
- `disabled_disconnected`: no valid headphone motion source; no pose streaming.
- `active_not_ready`: source exists but baseline/fit criteria not satisfied; keep gate closed.
- `active_ready`: stable source + baseline lock; eligible for sync.

## Gate Contract
- Send gate is closed by default on startup.
- Send gate opens only after `active_ready` and explicit sync/center action.
- Gate closes on disconnect/not-ready transitions.
