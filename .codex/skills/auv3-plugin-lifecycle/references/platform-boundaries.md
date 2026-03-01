Title: AUv3 Platform Boundaries

Use this reference to keep AUv3 integration scoped and safe.

## Core Boundary Rules
- Treat AUv3 as an extension runtime with stricter lifecycle and sandbox constraints than AUv2/VST3.
- Keep DSP and renderer logic format-agnostic.
- Keep app-level services (UI orchestration, downloads, long-lived background tasks) out of extension audio path.
- Use explicit state contracts for extension launch/teardown/reload cases.

## Audio Thread Rules
- No allocation, locks, or blocking I/O in realtime callbacks.
- No dependency on message-thread availability for DSP correctness.
- Non-finite protection and deterministic fallback behavior are mandatory.

## Capability Contract
- Features that rely on unavailable extension capability must degrade predictably.
- Requested mode and active mode must both be observable in diagnostics/evidence.
