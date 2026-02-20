Title: Reactive Visual QA and Troubleshooting
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# QA and Troubleshooting Checklist

## Core Checks
- Signal absent: visuals should settle to stable baseline.
- Signal spike: visuals should respond without clipping artifacts.
- Steady tone/noise: no drift or uncontrolled growth.
- Fast transients: no runaway flicker.

## Common Failure Signatures
- Jitter at rest: missing deadband or smoothing.
- Laggy response: over-smoothed or stale cache.
- Burst hitching: per-frame allocation or expensive recompute.
