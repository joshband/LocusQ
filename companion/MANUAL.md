---
Title: LocusQ Head-Tracking Companion Manual
Document Type: User Guide
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26
---

# LocusQ Head-Tracking Companion Manual

This manual covers the fastest way to run the companion in real AirPods sessions and verify that telemetry is fresh and trusted.

## 1. Build Once

```bash
cd companion
swift build -c release
```

## 2. Start Live Monitoring

Low-impact default (recommended first):

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --ui --hz 60 --sched-profile eco --monitor-hz 30
```

Higher responsiveness mode:

```bash
cd companion
.build/release/locusq-headtrack-companion --mode live --ui --hz 60 --sched-profile performance --monitor-hz 60
```

## 3. Configure LocusQ

In LocusQ:
- Enable head tracking path (Steam Binaural + headphone profile as needed).
- Keep at least one instance open; in multi-instance sessions, open all relevant tracks.

## 4. Verify Telemetry Health

In the companion monitor, confirm:
- `Plugin Ingest` = `active`
- `Plugin Sources / Consumers` > 0
- `Plugin Ack Age` stays low/stable
- `Output Device` is your AirPods endpoint
- `Effective Rate` is near target send rate
- `Interval / Jitter` is stable (not drifting upward continuously)

In LocusQ diagnostics card, confirm:
- head tracking shows live updates
- consumer count is non-zero while instances are loaded

## 5. Scheduling Guidance

- `eco`: lowest system impact; use for large sessions.
- `balanced`: default tradeoff.
- `performance`: highest telemetry responsiveness; use for troubleshooting.

Notes:
- Hard CPU-core pinning is not guaranteed from plugin/companion context on macOS.
- The implemented strategy is process isolation + QoS hints + RT-safe plugin ingest path.

## 6. Common Issues

`Plugin Ingest = waiting`:
- Companion may not be running, or plugin bridge is not active.
- Check port pairing (`19765` stream, `19766` ack).

`Plugin Ingest = stale` or high `Ack Age`:
- Raise profile (`--sched-profile performance`).
- Reduce monitor load (`--monitor-hz 30`).
- Check host load and number of heavy UI windows.

Wrong output device:
- Switch macOS output to AirPods and re-check device row.

## 7. Stop

- Close monitor window, or press `Ctrl+C` in terminal.
- Companion performs graceful shutdown and prints final summary lines.
