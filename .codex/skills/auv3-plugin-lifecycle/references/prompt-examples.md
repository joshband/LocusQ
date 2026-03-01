Title: AUv3 Prompt Examples

Use these turnkey prompts when routing AUv3 lifecycle work.

## Plan Slice Prompt

```text
/plan LocusQ BL-067 Slice A
Load: $auv3-plugin-lifecycle, $skill_plan, $skill_docs

Goal:
- Define AUv3 app-extension architecture and parity contract with AU/VST3/CLAP.
- Produce acceptance IDs, dependency map, and validation lanes.

Constraints:
- Keep DSP format-agnostic and realtime-safe.
- No host-name branching as behavior authority.

Output:
- Updated runbook slice plan + validation/evidence table.
```

## Implementation Slice Prompt

```text
/impl LocusQ BL-067 Slice B
Load: $auv3-plugin-lifecycle, $skill_impl, $clap-plugin-lifecycle

Goal:
- Wire AUv3 target/build settings and extension-safe runtime boundaries.
- Preserve existing AU/VST3/CLAP behavior.

Checks:
- No allocations/locks/blocking I/O in processBlock().
- State restore and automation deterministic across reload.
- Extension-unavailable fallback remains deterministic.

Evidence:
- TestEvidence/bl067_slice_b_<timestamp>/{status.tsv,host_matrix.tsv,parity_regression.tsv}
```

## Validation Prompt

```text
/test LocusQ BL-067 Candidate Gate
Load: $auv3-plugin-lifecycle, $skill_test, $skill_testing

Run:
- AUv3 host smoke + lifecycle transition matrix
- Non-AUv3 regression suite (AU/VST3/CLAP parity)

Decision:
- PASS only if AUv3 lanes are green and cross-format parity has no new regressions.
```
