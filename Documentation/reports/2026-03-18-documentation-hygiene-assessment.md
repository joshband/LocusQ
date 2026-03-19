Title: LocusQ Documentation Hygiene Assessment
Document Type: Assessment Report
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# LocusQ Documentation Hygiene Assessment

## Purpose

Assess the current `Documentation/` surface for clarity, brevity, usefulness, authority drift, and artifact bloat.
This report is planning and governance support. Canonical status authority remains:

- `Documentation/README.md`
- `Documentation/backlog/index.md`
- `status.json`
- `TestEvidence/build-summary.md`
- `TestEvidence/validation-trend.md`

## Current Snapshot

- `Documentation/backlog/`: 118 files
- `Documentation/reports/`: 33 files
- `Documentation/archive/`: 186 files
- `Documentation/plans/`: 32 files
- `Documentation/testing/`: 33 files
- `Documentation/Calibration POC/`: 17 files
- `Documentation/adr/`: 23 files

## Assessment Summary

The main problem is not metadata compliance. The main problem is lifecycle sprawl.

The repo already has reasonable authority rules, archive rules, readability gates, and backlog templates. What is missing is stronger enforcement of one-document-per-job and more aggressive retirement of temporary planning, review, and output bundles once their decisions have been absorbed into canonical surfaces.

## End-State Update

This assessment started as a planning report.
It now also records the main cleanup result.

What changed in this hygiene wave:
- active plans were compacted into short execution contracts
- active reports and review packets were reduced to decision-grade surfaces
- `Documentation/testing/` was reclassified around reusable guides and short support contracts
- `TestEvidence/build-summary.md` and `TestEvidence/validation-trend.md` were reduced to summary-first governance docs
- backlog templates, automation docs, and root governance docs were tightened
- the largest legacy-style done runbooks were compacted into short closeout authorities with archived legacy copies
- `Documentation/Calibration POC/` was reduced to a curated research-prototype surface with duplicate exploratory notes moved to archive

Current end-state:
- `Documentation/backlog/done/` still has `86` files, but only `4` are now above `140` lines
- `Documentation/reports/` is down to a much smaller active markdown surface
- archive-first preservation is now the default pattern for deep historical detail

Main conclusion:
- the repository is in much better shape
- the remaining work is selective, not broad
- future hygiene should be enforced by templates, ADRs, and archive discipline rather than another repo-wide rescue pass

## Folder-By-Folder Assessment

| Path | Current Role | Label | Main Issue | Recommended Direction |
|---|---|---|---|---|
| `Documentation/README.md` + top-level canonical docs | tier map, standards, invariants, ADR index, traceability | `canonical` | canonical set is mostly sound, but singleton review-era docs still sit beside canonical docs | keep canonical docs lean; archive or pointer-reduce one-off singleton docs |
| `Documentation/backlog/` | active runbooks, templates, backlog index | `canonical` + `supporting` | strongest structure in the repo, but some runbooks repeat status/evidence detail already visible in index and `TestEvidence/` | keep `index.md` as authority; shorten runbooks to intent, next action, dependencies, evidence pointer |
| `Documentation/backlog/done/` | closed runbooks | `archive`-leaning `supporting` | preserves traceability, but volume and narrative density are high | compact older done items to closeout summary + evidence links; archive superseded detail if no longer needed |
| `Documentation/plans/` | deep architecture and execution packets | `supporting` | too many parallel specs, duplicate vintages, and HTML/mock variants mixed with durable plans | keep latest active spec per BL in place; move superseded versions and mock variants to archive bundles |
| `Documentation/reports/` | active non-canonical reports | `supporting` + `generated` | mixes planning reports, reviews, tracker issues, visual packs, and generated data in one noisy surface | keep only currently referenced active reports; archive stale report packets quickly after backlog/ADR absorption |
| `Documentation/reports/data/` | machine-readable exports and dated summaries | `generated` | freshness drift already occurred; historical data snapshots sit beside live exports | treat only current exports as active; move dated historical JSON summaries to archive sets |
| `Documentation/reports/visuals/` | images, SVGs, HTML prototypes | `generated` | active and historical visuals are mixed; ownership and retention are unclear | keep only visuals referenced by active reports or current backlog items; archive the rest with manifests |
| `Documentation/testing/` | reusable guides plus BL-specific QA docs | `supporting` | BL-specific QA docs overlap with runbooks and `TestEvidence` packets | retain reusable testing guides as canonical support; migrate BL-specific QA truth into runbooks/evidence and archive duplicates |
| `Documentation/reviews/` | code and design review outputs | `supporting` | valuable findings, but review reports accumulate after their findings have already been translated into backlog items | keep only unresolved or latest anchor reviews active; archive reports once findings are captured in backlog/ADRs |
| `Documentation/research/` | active research index and references | `supporting` | small but stale: index paths do not fully match current file placement | reconcile index with actual active research locations and archive status |
| `Documentation/Calibration POC/` | curated prototype research, reference docs, scripts, PDF | `reference` | lower-risk after cleanup, but still needs strict scope control | keep only prototype code plus curated reference docs active; archive exploratory duplicates quickly |
| `Documentation/runbooks/` | legacy execution helpers | `supporting` / `archive-candidate` | mostly superseded by backlog templates and runbooks | reduce to pointer docs or archive |
| `Documentation/archive/` | historical preservation | `archive` | correct destination, but archive index and manifests are lagging current reality | add lightweight manifest discipline: source, reason, promoted replacement, re-open criteria |

## Lessons Learned

1. Machine-readable artifacts fail first when documentation hygiene slips.
   The backlog readability and redundancy gates passed, but the exported backlog summary files were stale until refreshed.
2. Review and plan artifacts are being preserved faster than they are being distilled.
   Many reports remain active after their decisions have already been converted into backlog items or code.
3. The backlog system is the healthiest part of the documentation set.
   It already has the clearest authority model, strongest templates, and working validation gates.
4. `Documentation/testing/` and `Documentation/plans/` are acting like secondary backlog systems.
   They often carry status-adjacent execution detail that should either live in runbooks or become archived support.
5. `Documentation/Calibration POC/` only became safer once exploratory notes were reduced and the kept surface was made explicit.
   Prototype folders need curated scope, not just a folder-level disclaimer.
6. Archiving is present, but archive promotion rules are not yet strict enough.
   Files move into `archive/`, but the active surfaces are not always reduced enough afterward.

## Future-State Hygiene Model

### 1. Canonical

Keep only stable authority here:

- top-level canonical docs in `Documentation/`
- `Documentation/backlog/index.md`
- active open runbooks
- ADRs
- invariants and traceability
- reusable testing guides

### 2. Active Working

Keep short-lived execution support here:

- one current plan/spec per active BL lane
- one active report packet per unresolved decision
- current machine-readable exports

Everything else should be pointer-reduced or archived.

### 3. Generated Current

Generated artifacts should be current-only by default:

- current `backlog-summary.json`
- current `backlog-summary.csv`
- current visuals that are directly referenced

Historical generated outputs should not stay in active folders unless a canonical doc links to them.

### 4. Archive

Archive anything that is:

- superseded,
- historical,
- no longer referenced by canonical docs,
- or too detailed for day-to-day operation.

Every archive bundle should say:

- what moved,
- why it moved,
- what replaces it,
- whether re-promotion is allowed.

## Recommended Cleanup Sequence

1. Stabilize generated truth.
   Refresh current machine-readable exports and keep freshness gates green.
2. Reduce competing status surfaces.
   Remove status-heavy prose from plan/testing/report docs when the same truth already exists in `Documentation/backlog/index.md` or `TestEvidence/`.
3. Compact `plans/`.
   Keep only the latest active spec per BL lane outside archive.
4. Compact `reports/`.
   Archive review and design packets after their findings are converted into backlog items, ADRs, or canonical docs.
5. Normalize `testing/`.
   Keep reusable guides; archive BL-specific QA narratives that duplicate runbooks/evidence.
6. Tighten archive manifests.
   Update `Documentation/archive/README.md` and `Documentation/README.md` together whenever archive scope changes.

## Immediate Action Items

| Priority | Action | Outcome |
|---|---|---|
| P0 | keep `Documentation/reports/data/backlog-summary.*` fresh on every backlog change | preserve trust in machine-readable backlog consumers |
| P0 | preserve active-surface brevity when new runbooks or reports are created | stop bloat from re-entering through new work |
| P1 | keep `Documentation/Calibration POC/` on a curated-reference diet | prevent exploratory note sprawl from returning |
| P1 | selectively review the remaining `Documentation/backlog/done/*.md` files over `140` lines | finish only the items that still read like historical packets |
| P2 | keep archive manifests current whenever long-form copies are preserved | make archive-first cleanup durable |

## Validation Status

`tested`

- `./scripts/validate-backlog-plain-language.sh` -> PASS
- `./scripts/validate-backlog-redundancy.py` -> PASS
- `./scripts/export-backlog-summaries.py --check` -> PASS
- `./scripts/validate-docs-freshness.sh` -> PASS
- `./scripts/git-artifact-hygiene-audit.sh --ref HEAD` -> PARTIAL PASS with two blockers:
  - tracked ignored `TestEvidence/*` paths
  - large historical blobs reachable in git history
