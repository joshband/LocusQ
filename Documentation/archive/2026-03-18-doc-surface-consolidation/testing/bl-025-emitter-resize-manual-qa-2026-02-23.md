Title: BL-025 Emitter Resize QA Checklist
Document Type: Test Evidence Checklist
Author: APC Codex
Created Date: 2026-02-23
Last Modified Date: 2026-02-23

# BL-025 Emitter Resize QA Checklist

## Purpose
Capture a compact resize-focused QA pass for the BL-025 EMITTER UI/UX rollout, with emphasis on authority cues, timeline visibility, and control density across narrow/medium/wide plugin window states.

## Scope
- Feature: `BL-025` Slice E (authority lock + responsive simplification)
- UI surface: `EMITTER` rail + timeline/footer
- Runtime target: `Standalone` production route (`Source/ui/public/index.html`)

## Automated-Assisted Run Context
| Field | Value |
|---|---|
| Run Date (UTC) | 2026-02-23T00:57:25Z |
| App | `build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app` |
| Validation Script | `./scripts/standalone-ui-selftest-production-p0-mac.sh` |
| Artifact | `TestEvidence/locusq_production_p0_selftest_20260223T005725Z.json` |
| Key Checks | `UI-P1-025A`, `UI-P1-025B`, `UI-P1-025C`, `UI-P1-025D`, `UI-P1-025E` |

## Compact Resize Checklist
| ID | Focus | Variant | Expected | Result |
|---|---|---|---|---|
| RZ-01 | Emitter rail width adapts without clipping | Base -> Compact -> Tight | Rail width decreases monotonically; no control loss | PASS (via `UI-P1-025E`) |
| RZ-02 | Timeline header remains visible | Base/Compact/Tight | Header never collapses below usable height | PASS (via `UI-P1-025E`) |
| RZ-03 | Timeline lanes remain actionable | Base/Compact/Tight | Lane viewport remains visible and non-zero | PASS (via `UI-P1-025E`) |
| RZ-04 | Preset name input remains usable | Base/Compact/Tight | Input width remains above minimum usable threshold | PASS (via `UI-P1-025E`) |
| RZ-05 | Preset action rows avoid clipping | Base/Compact/Tight | Save/Load/Rename/Delete buttons stay visible and in-row | PASS (via `UI-P1-025E`) |
| RZ-06 | Remote authority lock survives layout changes | Compact/Tight while remote selected | Authoring controls lock and unlock deterministically | PASS (via `UI-P1-025D`) |

## Operator Visual Spot-Check (Optional)
Use this only when a human visual sweep is required in host UI:
1. Resize plugin window to wide, medium, and narrow while in `EMITTER` mode.
2. Confirm bottom timeline remains fully visible and not cut off.
3. Select non-local emitter and confirm authority chip/note updates and controls lock.
4. Re-select local emitter and confirm controls unlock.

## Host Spot-Check Addendum (2026-02-23)
| Host Target | Command | Artifact | Result |
|---|---|---|---|
| Standalone (isolated rerun) | `./scripts/standalone-ui-selftest-production-p0-mac.sh` | `TestEvidence/locusq_production_p0_selftest_20260223T010422Z.json` | PASS (`UI-06`, `UI-07`, `UI-P1-025E` all pass) |
| REAPER headless (`--auto-bootstrap`) | `./scripts/reaper-headless-render-smoke-mac.sh --auto-bootstrap` | `TestEvidence/reaper_headless_render_20260223T010307Z/status.json` | PASS (`locusqFxFound=true`, `renderOutputDetected=true`) |

Notes:
- A transient parallel-launch run recorded `ui_selftest_timeout_before_pass_or_fail` with `NotFoundError` in `TestEvidence/locusq_production_p0_selftest_20260223T010307Z.json`.
- Immediate standalone rerun in isolation passed, so BL-025 responsive assertions remain green.

## Result
- Resize QA verdict (automated-assisted): `PASS`
- Manual-only visual sweep requirement: `NOT REQUIRED` for this pass; optional spot-check preserved above.

## References
- `Documentation/plans/bl-025-emitter-uiux-v2-spec-2026-02-22.md`
- `Source/ui/public/index.html`
- `Source/ui/public/js/index.js`
- `TestEvidence/locusq_production_p0_selftest_20260223T005725Z.json`
- `TestEvidence/locusq_production_p0_selftest_20260223T010422Z.json`
- `TestEvidence/reaper_headless_render_20260223T010307Z/status.json`
