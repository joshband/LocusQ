Title: Production Self-Test and REAPER Headless Smoke Guide
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# Production Self-Test and REAPER Headless Smoke Guide

## Purpose
Explain, in plain language, what the two most-used validation lanes do:
- `scripts/standalone-ui-selftest-production-p0-mac.sh`
- `scripts/reaper-headless-render-smoke-mac.sh`

This guide covers what each lane is good for, what it cannot prove, and where to improve next.

## Quick Summary
Think of these as two different safety nets:

- Production self-test: "Is the standalone UI + runtime contract healthy right now?"
- REAPER headless smoke: "Can the plugin still be loaded and rendered by REAPER in an automated host flow?"

You usually want both.

## 1) Production Self-Test

### What It Is
An automated in-app check that launches the LocusQ standalone app in self-test mode and asks the WebView UI to run a deterministic checklist (P0 + selected P1 contract checks).

### How It Works
1. Script launches the standalone binary with self-test env flags (`LOCUSQ_UI_SELFTEST=1`).
2. UI runs the internal test suite (button states, contract chips, transport behavior, diagnostics, etc.).
3. UI writes a result JSON into `TestEvidence/`.
4. Script reads JSON (`ok/status`) and exits pass/fail.

Primary artifact shape:
- `TestEvidence/locusq_production_p0_selftest_<timestamp>.json`
- `TestEvidence/locusq_production_p0_selftest_<timestamp>.run.log`

### What It Is Good For
- Fast regression detection for UI contracts.
- Deterministic checks for known BL lanes (for example `UI-P1-014`, `UI-P1-025A..E`, etc.).
- Verifying requested/active diagnostics and state-sync behavior that manual checks often miss.
- Producing machine-readable evidence for backlog closeout.

### Gaps / What It Cannot Do
- Cannot judge perceptual audio quality ("does it sound right to a human?").
- Cannot validate real DAW interaction patterns (this is standalone, not host automation).
- Cannot fully validate external device behavior (Bluetooth stack variance, OS mixer quirks, user hardware idiosyncrasies).
- Cannot replace subjective binaural/head-tracking listening checks.

### Opportunities to Improve
- Expand assertion coverage for new UI lanes as features land.
- Add explicit pass/fail diagnostics for known fallback paths (for example profile-stage mismatch reasons).
- Add richer artifact summaries (auto-generated markdown from result JSON).

## 2) REAPER Headless Smoke

### What It Is
An automated host-load and render sanity lane for REAPER that runs without interactive GUI actions.

### How It Works
1. Script can auto-bootstrap a temporary REAPER project via ReaScript.
2. REAPER is launched in headless-style command flow (`-renderproject` path).
3. Render attempt is monitored with retry/timeout logic.
4. Script writes a status artifact under `TestEvidence/reaper_headless_render_<timestamp>/`.

Primary artifact shape:
- `TestEvidence/reaper_headless_render_<timestamp>/status.json`
- Project/render logs in same folder.

### What It Is Good For
- Catching "plugin cannot load/render in host" failures early.
- Validating a stable baseline host automation path.
- Confirming REAPER bootstrap + render flow still works after runtime/UI changes.

### Gaps / What It Cannot Do
- Not a full DAW behavior matrix (single host lane, not every host/version).
- Does not prove advanced editing/session workflows.
- Does not prove user-facing GUI ergonomics inside the host.
- Does not directly grade sound quality.

### Opportunities to Improve
- Add more host lanes (additional DAWs/versions).
- Add structured assertions on rendered audio fingerprints for targeted scenarios.
- Add optional "plugin parameter roundtrip" checks post-render.

## Why We Use Both Together
- Production self-test catches UI/runtime contract regressions quickly.
- REAPER smoke catches host integration breakage quickly.
- Together they reduce release risk more than either alone.

## What Still Needs Manual Validation
These checks are still human-in-the-loop:
- Perceptual spatial quality (headphones/speakers).
- Device-specific behavior (AirPods/Sony/AVR real-world setup).
- Long-session UX and workflow ergonomics.

## Practical Rule of Thumb
- Use production self-test as your first gate after UI/runtime changes.
- Use REAPER headless smoke as your host sanity gate before closeout.
- Use manual listening/host workflows to confirm real user experience.
