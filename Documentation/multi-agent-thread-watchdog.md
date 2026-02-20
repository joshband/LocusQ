Title: Multi-Agent Thread Watchdog Guide
Document Type: Operational Guide
Author: Josh Band
Created Date: 2026-02-19
Last Modified Date: 2026-02-20

# Multi-Agent Thread Watchdog

## Status In This Repo

- This workflow is **optional** and **disabled by default**.
- Codex sessions in this repo should not auto-run bootstrap/watchdog commands unless explicitly requested.
- Scripts and templates remain in-repo for future exploration.

## Plain-English Summary

This capability helps you answer one question quickly:

Are my Codex agent slots actually doing useful work, or just sitting idle?

It does that by enforcing:
- a contract for each thread,
- regular heartbeat updates,
- proof-of-work artifacts,
- and strict closeout format when a thread finishes.

If a thread is stale, missing artifacts, timed out, or missing a proper DONE line, the watchdog fails with clear reasons.

## Thread Capacity Notes

- Current observed Codex agent thread limit in this environment: `6` total concurrent agent threads.
- Recommended operating policy with this limit:
  - `1` coordinator thread
  - `5` worker threads
- Important: this limit is environment/runtime policy and can change.
  - It may vary by Codex product surface, account tier, backend policy, or session state.
  - Do not assume the exact same number in every repo/session.
- Model-specific note:
  - Treat thread capacity as a platform/runtime limit, not a guaranteed property of one model family alone.

## Required Files

- `scripts/codex-session-bootstrap.sh` (session-start helper for baseline coordinator + worker contracts)
- `scripts/codex-init` (project bootstrap helper for thread contracts + heartbeats)
- `scripts/thread-watchdog` (executable Python script)
- `TestEvidence/thread-contracts.tsv` (contracts template)
- `TestEvidence/thread-heartbeats.tsv` (heartbeats template)

## What It Enforces

1. Explicit contract for each thread:
- required fields: `thread_id`, `task`, `expected_outputs`, `timeout_minutes`, `owner`
- optional field: `role` (`worker` or `coordinator`)

2. Heartbeats in one shared file:
- required format: `timestamp_utc	thread_id	status	last_artifact`

3. Proof-of-work artifact tracking:
- file paths are validated by mtime
- commit hashes are validated via `git`

4. Stalled classification:
- stale heartbeat AND stale/missing artifact => stalled

5. Slot policy:
- max 5 active workers
- max 1 active coordinator

6. Closeout discipline:
- status must be `DONE <result> <artifact/commit>`

## Quick Start

1. Start each new Codex session with:

```bash
./scripts/codex-session-bootstrap.sh
```

2. Append heartbeat rows to `TestEvidence/thread-heartbeats.tsv` every 5 minutes.
3. Run:

```bash
cd /path/to/your/project
./scripts/thread-watchdog
```

4. If it fails, fix the reported contract/heartbeat/artifact issue, then rerun.

## New/Fresh Project Setup (Document-Only)

Use this when someone only has this document and wants Codex to install the system from scratch.

1. In a new repo, ensure these directories exist:

```bash
mkdir -p scripts TestEvidence Documentation
```

2. Ask Codex to create four files:
- `scripts/thread-watchdog` (Python executable)
- `scripts/codex-init` (Python executable)
- `TestEvidence/thread-contracts.tsv` (header template)
- `TestEvidence/thread-heartbeats.tsv` (header template)

3. Mark scripts executable:

```bash
chmod +x scripts/thread-watchdog scripts/codex-init
```

4. Validate bootstrap:

```bash
./scripts/thread-watchdog --allow-empty
```

5. Register first thread and heartbeat:

```bash
./scripts/codex-init \
  --thread-id worker_1 \
  --task "Initial setup task" \
  --expected-outputs "TestEvidence/setup.log|<commit-hash>" \
  --timeout-minutes 60 \
  --owner "<owner>" \
  --role worker
```

6. Run strict gate:

```bash
./scripts/thread-watchdog
```

## Copy/Paste Prompt For Codex (Fresh Repo)

Use this prompt verbatim (edit owner and repo-specific paths):

```text
Set up a multi-agent thread watchdog in this repository.

Requirements:
1) Create executable scripts:
   - scripts/thread-watchdog
   - scripts/codex-init
2) Create TSV templates:
   - TestEvidence/thread-contracts.tsv
   - TestEvidence/thread-heartbeats.tsv
3) Enforce:
   - contract fields: thread_id, task, expected_outputs, timeout_minutes, owner, role(optional)
   - heartbeat fields: timestamp_utc, thread_id, status, last_artifact
   - stalled detection: stale heartbeat + stale/missing artifact
   - closeout format: DONE <result> <artifact/commit>
   - slot policy: <=5 active workers and <=1 active coordinator (configurable later)
4) Add Documentation/multi-agent-thread-watchdog.md with quickstart and usage examples.
5) Add an AGENTS.md section describing heartbeat cadence (every 5 minutes), DONE format, and watchdog checks before phase closeout.
6) Validate with:
   - ./scripts/thread-watchdog --allow-empty
   - ./scripts/codex-init --help

Return:
- created/modified file list
- exact commands run
- any assumptions
```

## Day-to-Day Activation (Codex)

To make this active in day-to-day Codex work, use:

- `scripts/codex-init` to register/update a thread contract and append a heartbeat.
- `scripts/thread-watchdog` as the gate before handoff/phase closeout.

Example bootstrap:

```bash
./scripts/codex-init \
  --thread-id worker_1 \
  --task "Phase 2.7 UI bridge triage" \
  --expected-outputs "TestEvidence/ui_bridge_debug.log|qa_output/suite_result.json" \
  --timeout-minutes 60 \
  --owner Josh \
  --role worker
```

Heartbeat-only update:

```bash
./scripts/codex-init \
  --heartbeat-only \
  --thread-id worker_1 \
  --status "WORKING relay wiring" \
  --last-artifact TestEvidence/ui_bridge_debug.log
```

## Recommended Status Patterns

- Working:
  - `WORKING <short-step>`
- Blocked:
  - `BLOCKED <reason>`
- Done:
  - `DONE PASS <artifact-or-commit>`
  - `DONE FAIL <artifact-or-commit>`

## Common Commands

Run strict checks:

```bash
./scripts/thread-watchdog
```

Allow empty templates while bootstrapping:

```bash
./scripts/thread-watchdog --allow-empty
```

Tune stale thresholds:

```bash
./scripts/thread-watchdog --stall-minutes 10 --artifact-max-age-minutes 10
```

## Reuse In Other Repos

Copy these four files:
- `scripts/codex-init`
- `scripts/thread-watchdog`
- `TestEvidence/thread-contracts.tsv`
- `TestEvidence/thread-heartbeats.tsv`

Then:
- keep the same TSV headers,
- keep `last_artifact` as either file paths or commit hashes,
- run the watchdog in CI or pre-merge checks to prevent ghost/stale thread usage,
- keep slot limits and stale thresholds configurable per repo.

## Output and Exit Codes

- `PASS` + exit `0`: healthy thread tracking.
- `FAIL` + exit `1`: at least one policy violation.
- exit `2`: invalid/missing input files or schema.
