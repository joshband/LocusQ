# LocusQ TODO (Plain Language)

Last updated: 2026-02-26

This file is a simple view of what is still left to do.
It is based on:
- `Documentation/backlog/index.md`
- `status.json`
- open runbooks for BL-030 and BL-032

## 1) Top Priority: Get Release Governance to Green (BL-030)

Status: In Validation (not done yet)

What is still blocking release:

1. RL-03 selftest stability is still flaky.
- Latest K1 run: BL-029 scope was `9/10`, BL-009 scope was `10/10`.
- One `app_exited_before_result` failure still happened.

2. RL-04 REAPER headless smoke is still red.
- Runtime/ABRT stability is not reliable enough yet.

3. RL-05 manual device evidence is still incomplete.
- Required manual notes for DEV-01..DEV-06 still need to be completed and validated.
- If external mic hardware is unavailable for DEV-06, a formal waiver must be used.

4. RL-06 pluginval reliability is still red.
- Harness exists, but pass-rate is not stable yet.

What is already green in BL-030:
- RL-09 (release notes linkage) is PASS.

## 2) In-Progress Hardening Work (BL-032)

Status: In Implementation

What is left:

1. Fix RT audit allowlist drift after line-map movement.
- Current blocker: `non_allowlisted=92` in latest D1 packet.
- Guardrail size target is now fixed; this RT drift is the remaining blocker.

## 3) Open Backlog Items Not Started Yet

These are still marked Todo in the master backlog:

1. BL-020: Confidence and masking overlays (R&D track)
- Add user-facing confidence/masking visual behavior.

2. BL-021: Room-story overlays (R&D track)
- Add room storytelling overlays to improve scene understanding.

3. BL-023: Resize and DPI hardening (UX track)
- Improve UI behavior on different window sizes and display scales.

## 4) Future-State / Roadmap (Non-Blocking Right Now)

These are known future tasks that are intentionally deferred and do not block current closeout:

1. AI features are post-v1 by policy (ADR-0004).
- No AI orchestration in the current critical path.

2. HX-05 follow-on runtime work.
- Runtime payload budget enforcement implementation was deferred.
- Runtime stress/perf lane for that enforcement was also deferred.

3. BL-013 follow-on parity work.
- CLAP backend parity remains a documented future slice risk area.
- AU backend probes are future work.

4. BL-029 Slice H is aspirational post-v1.
- Offline ML calibration assistant is explicitly non-blocking and deferred.

## 5) Suggested Execution Order

If the team wants one clear order:

1. Finish BL-030 blockers in this order: RL-03, RL-04, RL-06, RL-05 evidence closure.
2. Close BL-032 RT drift blocker.
3. Start Todo backlog items (BL-023 first for UX stability, then BL-020/BL-021).
4. Schedule future-state items (AI/post-v1, HX-05 follow-ons, BL-013/BL-029 deferred items).

## 6) Done Means

For each remaining item, mark it done only when:

1. Validation passes with evidence artifacts.
2. `Documentation/backlog/index.md` and `status.json` are updated.
3. `./scripts/validate-docs-freshness.sh` passes.
