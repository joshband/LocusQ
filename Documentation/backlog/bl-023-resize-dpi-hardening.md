Title: BL-023 Resize/DPI Hardening
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-24
Last Modified Date: 2026-02-24

# BL-023: Resize/DPI Hardening

## Status Ledger

| Field | Value |
|---|---|
| Priority | P2 |
| Status | Todo |
| Owner Track | Track C — UX Authoring |
| Depends On | BL-025 (Done) |
| Blocks | — |
| Annex Spec | (inline — references BL-025 resize manual QA) |

## Effort Estimate

| Slice | Complexity | Scope | Notes |
|---|---|---|---|
| A | Low | S | Host resize matrix documentation |
| B | Med | M | Fix identified resize issues |
| C | Med | M | DPI scaling validation |

## Objective

Harden resize and DPI scaling behavior across hosts and display configurations, building on the BL-025 baseline. Ensure the WebView UI renders correctly and without clipping at all reasonable window sizes and DPI scale factors on macOS (1x, 1.5x, 2x Retina).

## Scope & Non-Scope

**In scope:**
- Cataloging resize behavior in standalone, REAPER, Logic, Ableton at standard and Retina DPI
- Fixing any clipping, overflow, or layout breakage found during matrix testing
- Verifying correct DPI detection and scaling in WebView across display configurations
- Dual-monitor scenarios (different DPI per monitor)

**Out of scope:**
- Windows/Linux resize behavior (macOS only for now)
- UI redesign or layout restructuring (fix only, no new features)
- Touch/trackpad gesture behavior (separate concern)

## Architecture Context

- WebView resize: `Source/PluginEditor.cpp` manages `juce::WebBrowserComponent` sizing
- CSS layout: `Source/ui/public/index.html` uses flex/grid layout for responsive behavior
- JS layout: `Source/ui/public/js/index.js` handles dynamic layout adjustments
- BL-025 resize baseline: `Documentation/testing/bl-025-emitter-resize-manual-qa-2026-02-23.md` — 6 checks (RZ-01..RZ-06), all PASS
- Invariants: None directly, but viewport overlays must remain visual-only (Scene Graph invariant)
- ADR-0008: Viewport scope v1 vs post-v1 — defines what viewport features are in scope

## Implementation Slices

| Slice | Description | Files | Entry Gate | Exit Criteria |
|---|---|---|---|---|
| A | Host resize matrix | `Documentation/testing/` | BL-025 done | Matrix documented with pass/fail per host+resolution |
| B | Fix resize issues | `Source/PluginEditor.cpp`, `Source/ui/public/index.html`, `Source/ui/public/js/index.js` | Slice A findings | All identified issues fixed |
| C | DPI scaling validation | `Source/PluginEditor.cpp`, `Source/ui/public/js/index.js` | Slice B done | 1x, 1.5x, 2x all render correctly |

## Agent Mega-Prompt

### Slice A — Skill-Aware Prompt

```
/test BL-023 Slice A: Host resize matrix documentation
Load: $juce-webview-runtime, $skill_testing

Objective: Create a comprehensive resize test matrix documenting behavior across
hosts and resolutions.

Matrix dimensions:
- Hosts: Standalone, REAPER, Logic Pro, Ableton Live
- Resolutions: 800x600, 1024x768, 1280x800, 1440x900, 1920x1080, 2560x1440
- DPI: 1x (standard), 2x (Retina)
- Modes: EMITTER, RENDERER, CALIBRATE (test each)

Per cell: PASS / FAIL (with description) / N/A (host unavailable)

Check for:
- Control clipping (controls cut off at edges)
- Overflow (scrollbars appearing unexpectedly)
- Layout breakage (elements overlapping or misaligned)
- Text truncation (labels cut off)
- 3D viewport resize (Three.js canvas scales correctly)
- Minimum viable size (smallest usable window)

Constraints:
- Build BL-025 baseline as reference (existing RZ-01..RZ-06 all PASS)
- Document exact steps to reproduce any failures
- Capture screenshots for failures if possible

Evidence:
- Documentation/testing/bl-023-resize-matrix-<date>.md
- TestEvidence/bl023_resize_dpi_<timestamp>/matrix_results.tsv
```

### Slice A — Standalone Fallback Prompt

```
You are validating BL-023 Slice A for LocusQ, a JUCE spatial audio plugin with WebView UI.

PROJECT CONTEXT:
- LocusQ uses juce::WebBrowserComponent for its UI (WKWebView on macOS)
- Resize handling: Source/PluginEditor.cpp manages component sizing
- CSS layout: Source/ui/public/index.html uses flex and grid
- Three.js viewport: Source/ui/public/js/index.js handles canvas resize
- BL-025 baseline: Documentation/testing/bl-025-emitter-resize-manual-qa-2026-02-23.md
  has 6 resize checks (RZ-01..RZ-06), all PASS in standalone and REAPER

TASK:
1. Build: cmake --build build --target all
2. Launch standalone at each resolution (800x600, 1024x768, 1280x800, 1920x1080)
3. For each resolution, switch through EMITTER/RENDERER/CALIBRATE modes
4. Check for: clipping, overflow, layout breakage, text truncation, viewport scaling
5. If REAPER available: repeat resize tests as plugin in REAPER
6. Test Retina (2x) display if available (or simulate via display preferences)
7. Document results in TSV: host | resolution | dpi | mode | check | result | notes
8. Create Documentation/testing/bl-023-resize-matrix-2026-02-24.md with full matrix

CONSTRAINTS:
- Do not modify source code in this slice — documentation and testing only
- Use BL-025 RZ-01..RZ-06 as baseline reference
- Document exact reproduction steps for any failures

EVIDENCE:
- Documentation/testing/bl-023-resize-matrix-2026-02-24.md
- TestEvidence/bl023_resize_dpi_<timestamp>/matrix_results.tsv
```

### Slice B — Skill-Aware Prompt

```
/impl BL-023 Slice B: Fix identified resize issues
Load: $juce-webview-runtime, $skill_impl

Objective: Fix all FAIL items from the Slice A resize matrix.

Common fixes:
- Clipping: add CSS overflow handling, min-width/min-height guards
- Layout breakage: fix flex/grid properties, add media queries for small viewports
- Text truncation: add text-overflow: ellipsis or reduce font size at small widths
- Viewport scaling: ensure Three.js renderer.setSize() called on resize event

Files likely to modify:
- Source/PluginEditor.cpp — WebView component resize constraints
- Source/ui/public/index.html — CSS layout fixes
- Source/ui/public/js/index.js — dynamic layout adjustments, Three.js resize handler

Constraints:
- Fix only — do not redesign layout
- Preserve existing functionality (no visual regressions at standard sizes)
- Test each fix against the matrix before moving to next

Evidence:
- TestEvidence/bl023_resize_dpi_<timestamp>/fixes_applied.md
```

### Slice C — Skill-Aware Prompt

```
/test BL-023 Slice C: DPI scaling validation
Load: $juce-webview-runtime, $skill_testing

Objective: Validate WebView renders correctly at 1x, 1.5x, and 2x DPI scale factors.

Check:
- Text sharpness (no blurring on Retina)
- Control hit targets (not too small at high DPI)
- Three.js canvas pixel ratio matches display DPI
- Image assets (if any) have @2x variants or use vector
- Dual-monitor: move window between 1x and 2x displays

Validation:
- Visual inspection at each scale factor
- Canvas devicePixelRatio matches window.devicePixelRatio
- No rendering artifacts when moving between monitors

Evidence:
- TestEvidence/bl023_resize_dpi_<timestamp>/dpi_validation.tsv
- TestEvidence/bl023_resize_dpi_<timestamp>/status.tsv
```

## Validation Plan

| Lane ID | Type | Command | Pass Criteria |
|---|---|---|---|
| BL-023-matrix | Manual | Resize matrix testing | All cells PASS or documented N/A |
| BL-023-fixes | Mixed | Fix verification | All FAIL items resolved |
| BL-023-dpi | Manual | DPI scaling checks | Correct rendering at 1x, 2x |
| BL-023-regression | Automated | Production self-test | No regressions from fixes |
| BL-023-freshness | Automated | `./scripts/validate-docs-freshness.sh` | Exit 0 |

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| Host-specific resize behavior varies | Med | High | Test top 3 hosts, document N/A for others |
| DPI detection unreliable in some hosts | Med | Med | Fall back to safe defaults, document limitations |
| CSS fixes break existing layout | Med | Med | Test at standard sizes after each fix |
| Dual-monitor DPI change causes flicker | Low | Med | Debounce resize handler, test explicitly |

## Failure & Rollback Paths

- If CSS fix breaks existing layout: revert fix, test at standard resolution, iterate
- If DPI detection fails in host: document as known limitation, add to troubleshooting/known-issues.yaml
- If Three.js canvas doesn't scale: check renderer.setPixelRatio() call, verify resize event fires

## Evidence Bundle Contract

| Artifact | Path | Required Fields |
|---|---|---|
| Resize matrix | `Documentation/testing/bl-023-resize-matrix-<date>.md` | host, resolution, dpi, mode, result |
| Matrix TSV | `TestEvidence/bl023_resize_dpi_<timestamp>/matrix_results.tsv` | host, resolution, dpi, mode, check, result |
| Fixes log | `TestEvidence/bl023_resize_dpi_<timestamp>/fixes_applied.md` | file, change, before, after |
| DPI validation | `TestEvidence/bl023_resize_dpi_<timestamp>/dpi_validation.tsv` | scale_factor, check, result |
| Status TSV | `TestEvidence/bl023_resize_dpi_<timestamp>/status.tsv` | lane, result, timestamp |

## Closeout Checklist

- [ ] Resize matrix documented for all tested hosts/resolutions
- [ ] All identified resize issues fixed
- [ ] DPI scaling validated at 1x, 2x
- [ ] No regressions in production self-test
- [ ] Evidence captured at designated paths
- [ ] status.json updated
- [ ] Documentation/backlog/index.md row updated
- [ ] TestEvidence surfaces updated
- [ ] ./scripts/validate-docs-freshness.sh passes
