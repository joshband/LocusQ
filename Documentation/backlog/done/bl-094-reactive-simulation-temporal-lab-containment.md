Title: BL-094 Reactive/Simulation/Temporal Lab Containment
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-03-17
Last Modified Date: 2026-03-18

# BL-094 Reactive/Simulation/Temporal Lab Containment

## Plain-Language Summary

BL-094 in plain terms: prevent reactive, simulation-driven, and temporal-feature work from turning into top-level product sprawl. Current state: Done. This item converts the 2026-03-17 scope guardrails into an explicit backlog lane so new ideas extend existing cards, overlays, or lab drawers instead of fragmenting LocusQ into too many modes and side quests. The guardrail is now written into the backlog and architecture, the live `EMITTER` motion card explicitly contains `Physics` plus `Choreography` inside a `Motion Lab` drawer instead of presenting them as equal first-layer paths, and the archive/closeout sync is complete.

## 6W Snapshot (Who/What/Why/How/When/Where)

| Question | Plain-language answer |
|---|---|
| Who is this for? | Product owners, design/implementation agents, and maintainers working on reactive, simulation, temporal, or experimental UI ideas. |
| What is changing? | LocusQ gets an explicit design-scope contract for where advanced experiments may live and where they may not. |
| Why is this important? | The reviewed product risk is feature sprawl, not lack of ideas. Without guardrails, advanced overlays and experiments will compete with core operator tasks. |
| How will we deliver it? | Freeze the containment rules, link them to current backlog/design artifacts, and require future feature proposals to map into existing surfaces first. |
| When is it done? | When new reactive/simulation/temporal proposals have a clear placement test and the default UX remains focused on scene truth, device trust, and render authority. |
| Where is the source of truth? | This runbook, `Documentation/backlog/index.md`, the 2026-03-17 design reports, and any future design/runbook items that propose new experimental surfaces. |

## Visual Aid Index

| Visual Aid | Why it helps | Where to find it |
|---|---|---|
| Status ledger | Fast scan of scope and priority. | `## Status Ledger` |
| Scope guardrail table | Gives future work a clear pass/fail placement test. | `## Objective` |
| Scope ownership diagram | Shows plugin vs companion vs lab ownership. | `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/scope-boundary.svg` |
| Containment validation bundle | Shows the live `EMITTER` motion lab containment plus validation evidence. | `TestEvidence/bl094_motion_lab_containment_20260318T045654Z/summary.md` |
| Owner sync packet | Records the promotion decision and final owner closeout checks. | `TestEvidence/bl094_owner_sync_z1_20260318T045927Z/promotion_decision.md` |

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-094 |
| Priority | P2 |
| Status | Done |
| Track | E - R&D Expansion |
| Effort | Small / S |
| Depends On | BL-090, BL-091 |
| Blocks | future reactive/simulation/temporal expansion lanes |
| Default Replay Tier | T0 (docs-first governance lane; escalate only if implementation work is added) |
| Heavy Lane Budget | None |

## Objective

Freeze the following containment rules:

1. The plugin owns:
   - scene truth
   - emitter authoring
   - render authority
2. The companion owns:
   - device readiness
   - sync / center
   - profile capture and apply
3. `Lab` owns:
   - parity evidence
   - deep telemetry
   - reactive/simulation/temporal experiments
4. New reactive, simulation, and temporal ideas must first answer:
   - can this live in an existing card?
   - can this live as an overlay lens?
   - can this live in the existing timeline?
   - can this live in `Lab`?
5. If the answer to all four is `no`, the idea needs an explicit planning lane and product justification before it gets a new top-level surface.
6. If a metric or experiment would not change the operator's next 10 seconds of action, it defaults to `Lab`, not the first-layer product surface.

## Implementation Update (2026-03-18)

- `Source/ui/public/index.html` now labels `Physics` and `Choreography` as `Lab` motion paths and contains them inside a dedicated `Motion Lab` drawer.
- `Source/ui/src/index.ts` now auto-opens the `Motion Lab` drawer only when a lab motion source is active, preserves timeline as the first-layer path, and encodes the containment contract in the production selftest.
- `ARCHITECTURE.md` now records the plugin/companion/lab containment rule in the runtime architecture.
- related UI runbooks now reference BL-094 so future visual or hierarchy work inherits the same placement test.

## Source Inputs

- `Documentation/reports/2026-03-17-locusq-ui-ux-design-review.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-refinement-pass.md`
- `Documentation/reports/2026-03-17-locusq-ui-ux-second-opinion-claude.md`
- `Documentation/reports/visuals/ui-ux-review-2026-03-17/scope-compass.svg`
- `Documentation/reports/visuals/ui-ux-second-opinion-claude-2026-03-17/scope-boundary.svg`
- `Documentation/reports/ui-ux-refinement-2026-03-17/component-specs.md`
- `ARCHITECTURE.md`

## Acceptance IDs

- `BL094-A1` Plugin, companion, and lab ownership boundaries are explicitly documented.
- `BL094-A2` Future reactive/simulation/temporal work is required to map into an existing surface before proposing a new top-level mode.
- `BL094-A3` New experimental visuals default to lab containment unless they clearly improve first-layer operator success.
- `BL094-A4` This guardrail is referenced by future relevant backlog items and design proposals.
- `BL094-A5` Ideas that do not change the operator's next 10 seconds of action are explicitly kept out of default plugin or companion focus surfaces.

## Implementation Slices

| Slice | Description | Files / Surfaces | Exit Criteria |
|---|---|---|---|
| A | Freeze scope guardrail contract | this runbook + backlog/index references | placement test and ownership rules are explicit |
| B | Link the guardrail from follow-on UI/design lanes | BL-090/BL-091/BL-093 and future experimental runbooks | future proposals reference the containment rule |
| C | Live first-layer containment for current experimental motion paths | `Source/ui/public/index.html`, `Source/ui/src/index.ts` | `Physics` and `Choreography` remain available but are clearly contained inside `Motion Lab` |
| D | Reassess if a future feature truly needs a new surface | future planning item if required | explicit product justification exists before scope expands |

## Validation Plan

| Lane ID | Type | Command / Method | Pass Criteria |
|---|---|---|---|
| BL094-PLUGIN-BUILD | Automated | `cd Source/ui && npm run typecheck && npm run build` | exit 0 |
| BL094-STANDALONE | Automated | `cmake --build build_local --config Release --target LocusQ_Standalone -j 8` plus `./scripts/standalone-ui-selftest-production-p0-mac.sh build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app` | exit 0 and motion-source containment checks remain green |
| BL094-DOCS | Automated | `./scripts/validate-backlog-plain-language.sh`, `./scripts/validate-backlog-redundancy.py`, `./scripts/validate-docs-freshness.sh` | exit 0 |
| BL094-REFERENCE-CHECK | Manual | confirm related future runbooks cite this guardrail when applicable | citations present |

## Replay Cadence Plan (Required)

Reference policy: `Documentation/backlog/index.md` -> `Global Replay Cadence Policy`.

| Stage | Tier | Runs | Command Pattern | Evidence |
|---|---|---|---|---|
| Dev loop | T0/T1 | 1 | docs validation or targeted plugin build | logs |
| Candidate intake | T1 | 1-3 | targeted doc/runtime review | notes + linked proposals |
| Promotion | T2 | owner-approved equivalent | owner packet + evidence bundle |

## Governance Alignment (2026-03-17)

Canonical lifecycle/evidence rules are defined in:
- `Documentation/backlog/index.md`
- `Documentation/standards.md`

BL-094 is intentionally a scope-governance lane. It exists to keep future UI experimentation aligned with the reviewed product boundaries instead of letting exploratory work redefine the main UX by accident.
