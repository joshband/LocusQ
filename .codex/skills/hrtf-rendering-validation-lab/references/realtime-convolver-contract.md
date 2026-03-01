Title: Realtime Convolver Contract
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Realtime Convolver Contract

Mandatory constraints:
- no heap allocation in `processBlock()`
- no locks in `processBlock()`
- no blocking I/O in `processBlock()`

Latency requirements:
- direct FIR path latency = 0
- partitioned path latency explicitly reported and verified
