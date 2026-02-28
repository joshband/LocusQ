Title: BL-052 Steam Audio Virtual Surround + Quad Layout
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-28
Last Modified Date: 2026-02-28

# BL-052 Steam Audio Virtual Surround + Quad Layout

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-052 |
| Priority | P1 |
| Status | Open |
| Track | E - R&D Expansion |
| Effort | Med / M |
| Depends On | BL-038 |
| Blocks | BL-053, BL-054 |

## Objective

Implement `QuadSpeakerLayout` enum and `SteamAudioVirtualSurround` class to render a quad speaker bed to stereo binaural via Steam Audio. Wire monitoring mode switch (speakers / steam_binaural / virtual_binaural) in PluginProcessor.

## Acceptance IDs

- quad→binaural renders without RT allocation in processBlock
- monitoring mode switch is deterministic
- speakers path is unchanged

## Validation Plan

QA harness script: `scripts/qa-bl052-steam-audio-virtual-surround-mac.sh` (to be authored).
Evidence schema: `TestEvidence/bl052_*/status.tsv`.
