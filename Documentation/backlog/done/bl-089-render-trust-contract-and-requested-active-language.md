Title: BL-089 Render Trust Contract and Requested-vs-Active Language
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-089 Render Trust Contract and Requested-vs-Active Language

## Plain-Language Summary

BL-089 in plain terms: define one honest render-trust language contract for LocusQ so the plugin and companion always tell the operator what they asked for, what is actually active, why anything changed, and whether quality-style scores are measured, estimated, or unavailable. Current state: Done. The operator-facing copy audit highlighted by the 2026-03-17 second-opinion review is now implemented in the plugin and companion shells, representative parity evidence is captured, the owner packet is written, and the archive/closeout sync is complete, while BL-095 and BL-099 continue independently as truthfulness follow-on lanes.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Plugin operators, companion users, QA/release owners, and maintainers who need one reliable trust model across render states. |
| What is changing? | LocusQ gets a normalized render-trust state contract and a shared copy system for `Requested`, `Active`, fallback reason, profile source, control owner, and score/provenance status. |
| Why is this important? | Without it, advanced spatial states are easy to over-claim, fallback behavior becomes confusing, and plugin/companion messaging drifts into language that sounds more measured or personalized than the code/evidence supports. |
| How will we deliver it? | Freeze the additive state fields and copy rules first, then wire them into plugin and companion surfaces with deterministic degraded-state behavior. |
| When is it done? | When plugin and companion can both present requested-vs-active truth using the same field set and plain-language copy, including degraded and limited-capability states. |
| Where is the source of truth? | This runbook, `Documentation/backlog/index.md`, the 2026-03-17 UI/UX reports, and repo-local evidence under `TestEvidence/...`. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast scan of priority, dependencies, and scope. | `## Status Ledger` |
| Field contract table | Makes the trust model explicit and implementation-ready. | `## Objective` |
| Acceptance + validation tables | Clarify what must ship before this can be promoted. | `## Acceptance IDs`, `## Validation Plan` |
| Existing refinement visuals | Anchor the runbook to current reviewed mockups. | `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/trust-state-ladder.svg` |
| Trust-wave validation bundle | Fresh plugin/companion/browser captures backing validation posture. | `TestEvidence/ui_ux_trust_wave_validation_20260318T023805Z/summary.md` |
| Owner sync packet | Records the promotion decision and final owner closeout checks. | `TestEvidence/ui_ux_trust_wave_owner_sync_z1_20260318T040618Z/promotion_decision.md` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-089 |
| Priority | P1 |
| Status | Done |
| Track | B - Scene/UI Runtime |
| Effort | Medium / M |
| Depends On | BL-040 (Done), BL-053 (Done), BL-058 (Done), BL-095, BL-099 |
| Blocks | BL-090, BL-091, BL-092 |
| Default Replay Tier | T1 (dev-loop deterministic replay; escalate per Global Replay Cadence Policy) |
| Heavy Lane Budget | Standard |

## Objective

Define one additive render-trust contract that both the plugin and companion can present consistently.

Required first-layer fields:

| Field | Meaning | First-Layer Label |
|---|---|---|
| `requestedRenderPath` | What the operator or session asked for | `Requested` |
| `activeRenderPath` | What the system is actually doing now | `Active` |
| `renderFallbackReason` | Why the active path differs from requested | `Why this changed` |
| `renderProfileSource` | Where the active profile came from | `Profile source` |
| `renderControlOwner` | Which domain currently owns the decision | `Control owner` |
| `renderEvidenceStatus` | Whether user-visible quality/profile claims are measured, estimated, generic, or unavailable | `Evidence status` |

Contract rules:

1. `Requested` and `Active` must always appear together anywhere fallback is possible.
2. `Why this changed` is required whenever `Requested != Active`.
3. First-layer copy must be plain-language, not raw runtime aliases or enum names.
4. Missing or limited capability must degrade honestly:
   - `Limited in AUv3`
   - `Steam unavailable`
   - `Companion not ready`
   - `Using generic profile`
5. Score-like or profile-quality language must name its evidence class:
   - `Measured from listening evidence`
   - `Estimated from reference heuristics`
   - `Generic compensation active`
   - `Measurement unavailable`
6. Diagnostics and parity evidence remain additive and may live in `Lab`, but must never replace first-layer truth.

Required operator-language mappings:

| Raw/internal state | Operator-facing language |
|---|---|
| `disabled_disconnected` | `Device not connected` |
| `active_not_ready` | `Device connected, waiting for sync` |
| `mode_synthetic` or `mode=synthetic` | `Synthetic mode active - motion is simulated` |
| `steam_unavailable` | `Steam Audio unavailable on this path` |

Required `Why this changed` rules:

1. Never expose internal enum names, aliases, or debug token strings.
2. Time-based degradations must be actionable, for example `No pose received for 2.4 seconds`.
3. Capability-boundary degradations must name the limit, for example `Limited in AUv3 app extension`.
4. Synthetic/test modes must be named explicitly when they affect trust interpretation.

## Source Inputs

- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Documentation/reviews/2026-03-17-second-opinion-code-dsp-supplement.md`
- `Documentation/reports/visuals/ui-ux-refinement-2026-03-17/trust-state-ladder.svg`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/render-trust-ladder.svg`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/second-opinion-prototype.html`
- `Documentation/reports/ui-ux-refinement-2026-03-17/component-specs.md`
- `Documentation/spatial-audio-profiles-usage.md`
- `Documentation/research/locusq-headtracking-binaural-methodology-2026-02-28.md`
- `Source/ui/public/index.html`
- `Source/ui/src/index.ts`
- `companion/Sources/LocusQHeadTrackingCompanion/main.swift`

## Acceptance IDs

- `BL089-A1` Requested and active render state are additive, separately named, and always available to first-layer UI surfaces.
- `BL089-A2` Fallback reason is present and plain-language whenever requested and active diverge.
- `BL089-A3` Profile source is explicit and distinguishes companion/device/local/generic sources.
- `BL089-A4` Control-owner copy is deterministic and consistent with BL-040 authority-state semantics.
- `BL089-A5` AUv3 and backend-limited cases degrade with explicit limited-capability wording rather than silent omission.
- `BL089-A6` Plugin and companion use the same trust vocabulary set for equivalent states.
- `BL089-A7` Raw internal state strings never appear on operator-facing surfaces.
- `BL089-A8` Time-based degradations and synthetic-mode states are expressed in plain language, including stale-pose duration where relevant.
- `BL089-A9` Score-like or profile-quality surfaces explicitly distinguish measured, estimated, generic, and unavailable states rather than implying objective validation by default.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Freeze field contract, raw-state mapping, and `Why this changed` copy table | runbook + scene-state references + UI copy map | field names, labels, stale-state phrasing, fallback tokens, and evidence-status labels are explicitly approved |
| B | Add plugin-side normalization and first-layer consumption | `Source/ui/public/index.html`, `Source/ui/src/index.ts`, native scene-state producers as needed | plugin renders requested/active/fallback/profile source/control owner/evidence status coherently and never leaks raw state strings |
| C | Add companion-side trust summary parity and synthetic-mode/operator copy cleanup | `companion/Sources/LocusQHeadTrackingCompanion/main.swift` | companion trust summary matches plugin vocabulary for equivalent states, including synthetic-mode, stale-pose, and measurement-status language |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL089-DOCS | Automated | `./scripts/validate-backlog-plain-language.sh` and `./scripts/validate-docs-freshness.sh` | exit 0 |
| BL089-PLUGIN-PREVIEW | Automated | browser preview / Playwright capture of plugin trust states | requested, active, fallback reason, and evidence status are visible together when relevant |
| BL089-COMPANION-REVIEW | Manual or harness-assisted | companion mock/runtime review | active profile, fallback source, and evidence-status wording match the contract |
| BL089-PARITY | Focused QA | representative plugin + companion state comparison | equivalent states use equivalent language |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T1 | 1-3 | focused preview/build/copy review | screenshots + validation notes |
| Candidate intake | T2 | 5 or owner-approved equivalent | state-parity replay and visual capture | replay summary + captured states |
| Promotion | T3 | 10 or owner-approved equivalent | owner-reviewed parity packet | owner packet + evidence bundle |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

This runbook adds item-specific trust-language and degraded-state requirements only.
