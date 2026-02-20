Title: LocusQ Stage 14 Comprehensive Review
Document Type: Review Report
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# LocusQ Stage 14 Comprehensive Review

## Scope
Comprehensive architecture, code, design, and QA review for Stage 14 closeout with explicit focus on:

1. `.ideas` vs implementation drift,
2. laptop speakers/mic/headphones usability gates,
3. release decision readiness.

## Inputs Reviewed
- `.ideas/creative-brief.md`
- `.ideas/architecture.md`
- `.ideas/parameter-spec.md`
- `.ideas/plan.md`
- `Source/PluginProcessor.cpp`
- `Source/PluginEditor.cpp`
- `Source/PluginEditor.h`
- `Source/ui/public/incremental/index_stage12.html`
- `Source/ui/public/incremental/js/stage12_ui.js`
- `Documentation/stage14-review-release-checklist.md`
- `Documentation/implementation-traceability.md`
- `Documentation/research/qa-harness-upstream-backport-opportunities-2026-02-20.md`
- `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`
- `status.json`

## Findings (Severity Ordered)

### High

1. `rend_phys_interact` is still a no-op/deferred control and remains unbound in Stage 12 UI bridge.
   - Evidence: parameter is declared (`Source/PluginProcessor.cpp`) but has no active runtime consumer beyond declaration/read-path absence in processor/editor bridge coverage.
   - Risk: user-visible contract ambiguity unless explicitly documented as deferred.

### Medium

1. `emit_dir_azimuth`, `emit_dir_elevation`, `phys_vel_x`, `phys_vel_y`, `phys_vel_z` are DSP/runtime-backed but not exposed in Stage 12 relay/attachment/UI controls.
   - Evidence: APVTS/DSP read-path exists in processor; corresponding WebView relay+attachment coverage does not exist in `PluginEditor` and Stage 12 UI control map.
   - Risk: parameter-spec parity drift and reduced editability for advanced motion/directivity workflows.

2. Manual DAW acceptance (including portable-device profile checks) is still open.
   - Evidence: `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md` shows Stage 13 handoff pending with `DEV-01..DEV-06` rows unexecuted.
   - Risk: release without in-host click-path and device-profile signoff.

3. Stale REAPER load risk existed when host stayed running or cache rows were stale.
   - Evidence: installed binaries matched current build hashes but REAPER process remained active during verification.
   - Mitigation now landed: `scripts/build-and-install-mac.sh` automates host/cache hygiene (REAPER auto-quit option, AU registrar refresh, REAPER cache pruning, binary hash verification).

### Low

1. README drift in PR gate docs was present (smoke gate described as default despite Stage 12 self-test being default).
   - Mitigation now landed in `README.md`.

## Architecture Verification Summary

- Output layout contract is aligned to source for mono/stereo/quad host negotiation (`isBusesLayoutSupported` + scene-state telemetry mapping).
- Calibration routing contract is explicit and deterministic:
  - mic input selection via `cal_mic_channel`,
  - speaker routing via `cal_spk1_out..cal_spk4_out`,
  - auto-detection path preserves manual overrides unless forced.
- This supports the Stage 14 portable profile goal (laptop stereo + headphones + calibration mic routing), but still requires manual DAW evidence rows to close.

## Design Verification Summary

- Stage 12 remains the primary incremental route with fallback routes preserved by policy.
- Control-rail/viewport behavior is documented and automated self-test covered.
- In-host manual UX signoff is still required for final parity confidence.

## QA Verification Summary

- Automated status remains green for Stage 13 non-manual closeout lanes (self-test/UI gate/non-UI suites/pluginval/standalone smoke), with manual DAW rerun intentionally deferred.
- Contract-pack adoption and runtime-config alignment are now backported in LocusQ and wired into CI critical lane.

## Drift Matrix (.ideas vs Implementation)

| Area | Current State | Disposition |
|---|---|---|
| Portable device profile gate | Spec requires laptop speakers/mic/headphones checks | Pending manual execution (`DEV-01..DEV-06`) |
| `rend_phys_interact` | Parameter present, runtime/UI behavior deferred | Decide implement vs explicit defer contract |
| `emit_dir_*` + `phys_vel_*` Stage 12 exposure | Runtime present, UI bridge absent | Bind in Stage 14 or explicitly defer |
| REAPER stale load handling | Previously manual and error-prone | Automated in install script (new) |

## Comparison Notes (echoform, memory-echoes, monument-reverb)

- Harness migration patterns are broadly aligned after the contract-pack/runtime-config backport.
- `monument-reverb` operational scripts highlighted the value of explicit host cache refresh; LocusQ now includes that behavior in canonical install automation.
- Next upstream target remains harness-side runner/contract unification across all migrated repos.

## Release Recommendation

Current recommendation: `hold` for GA, optional `draft-pre-release` state lock.

- Reason to hold GA: manual DAW + portable-device checklist is still open.
- Reason `draft-pre-release` is acceptable now: automated lanes are green, drift list is explicit and bounded, and install/cache automation is improved.

## Opinionated Next Steps

1. Execute one fresh manual DAW session using current binaries:
   - `./scripts/build-and-install-mac.sh`
   - complete `UI-01..UI-12`, `DEV-01..DEV-06` in `TestEvidence/phase-2-7a-manual-host-ui-acceptance.md`.
2. Decide `rend_phys_interact` path now:
   - implement runtime behavior in v1, or
   - record explicit defer/no-op contract and keep hidden/unbound in UI.
3. Resolve Stage 12 exposure gap for `emit_dir_*` and `phys_vel_*`:
   - add relay/attachment/UI controls, or
   - document defer status explicitly in ADR + traceability.
4. If manual checks pass, cut GitHub `draft-pre-release` immediately, then promote to `ga` after signoff review.
5. Keep harness upstream backlog active (runner-app/perf/runtime-config unification) and backport once upstream lands.
