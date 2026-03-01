Title: Temporal Effects Prompt Examples

Use these prompts for delay/echo/looper/frippertronics slices.

## Plan Slice Prompt

```text
/plan LocusQ BL-068 Slice A
Load: $temporal-effects-engineering, $skill_plan

Goal:
- Define delay/echo/feedback/looper architecture and safety contract.
- Lock max delay/loop lengths, transport policy, and automation semantics.

Constraints:
- Realtime-safe buffer ownership and bounded memory.
- Deterministic output for deterministic timeline input.
```

## Implementation Slice Prompt

```text
/impl LocusQ BL-068 Slice B
Load: $temporal-effects-engineering, $skill_impl, $spatial-audio-engineering

Goal:
- Implement tempo-aware delay + looper core with runaway-safe feedback policy.
- Keep click-free transitions for automation and profile swaps.

Checks:
- No allocations/locks/blocking I/O in processBlock().
- Non-finite guards on all feedback and summing stages.
- CPU and latency contracts hold from 44.1kHz to 192kHz.
```

## Validation Prompt

```text
/test LocusQ BL-068 Candidate Gate
Load: $temporal-effects-engineering, $skill_testing, $perceptual-listening-harness

Run:
- Deterministic replay matrix (T1/T2 cadence)
- Runaway/finite-output stress lanes
- Click/zipper and transport recall checks
- Listening packet for long-feedback musical behavior
```
