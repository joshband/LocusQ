Title: Temporal Loop and Feedback Contracts

## Contract Inputs
- Delay/loop duration limits and rate assumptions.
- Tempo and transport coupling policy.
- Feedback topology and gain limits.

## Safety Contracts
- Hard ceiling for effective feedback gain.
- Explicit non-finite guards in read/write and post-feedback stages.
- Runaway mitigation policy (auto-clamp, damp, or controlled reset).

## Recall and Automation
- Session restore must reproduce loop position/state deterministically.
- Automation ramps must be click-safe and bounded.
- Transport-start edge cases must be documented and tested.
