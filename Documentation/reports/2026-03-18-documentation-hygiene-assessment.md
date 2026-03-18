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

- `Documentation/backlog/`: 115 files
- `Documentation/reports/`: 60 files
- `Documentation/archive/`: 47 files
- `Documentation/plans/`: 44 files
- `Documentation/testing/`: 34 files
- `Documentation/Calibration POC/`: 22 files
- `Documentation/adr/`: 20 files

## Assessment Summary

The main problem is not metadata compliance. The main problem is lifecycle sprawl.

The repo already has reasonable authority rules, archive rules, readability gates, and backlog templates. What is missing is stronger enforcement of one-document-per-job and more aggressive retirement of temporary planning, review, and output bundles once their decisions have been absorbed into canonical surfaces.

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
| `Documentation/Calibration POC/` | prototype research, reference docs, scripts, PDFs | `reference` + `archive-candidate` | folder mixes research, prototype code, PDFs, and historically large dataset/blob lineage | reclassify explicitly as research prototype surface, or archive/externalize most of it; do not let it read as active product documentation |
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
5. `Documentation/Calibration POC/` is the highest-risk ambiguity surface.
   It looks half active, half historical, and it has already contributed oversized artifact history.
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
6. Resolve `Calibration POC/`.
   Either make it an explicitly indexed research-prototype surface or archive/externalize most of it.
7. Tighten archive manifests.
   Update `Documentation/archive/README.md` and `Documentation/README.md` together whenever archive scope changes.

## Immediate Action Items

| Priority | Action | Outcome |
|---|---|---|
| P0 | keep `Documentation/reports/data/backlog-summary.*` fresh | restore trust in machine-readable backlog consumers |
| P0 | inventory `plans/` for duplicate vintages per BL/HX | remove parallel-spec confusion |
| P1 | inventory `reports/` for active-reference vs archive eligibility | shrink noisy active surface |
| P1 | classify every `testing/*.md` file as reusable guide vs BL-specific packet | cut duplicate QA narrative |
| P1 | decide whether `Calibration POC/` remains active research or becomes archived prototype material | remove the largest ambiguity surface |
| P2 | compact older `backlog/done/*.md` items into slimmer closeout format | preserve traceability with less reading tax |

## Validation Status

`partially tested`

- `./scripts/validate-backlog-plain-language.sh` -> PASS
- `./scripts/validate-backlog-redundancy.py` -> PASS
- `./scripts/export-backlog-summaries.py --check` -> FAIL before refresh; corrected by refreshing exports
- `./scripts/validate-docs-freshness.sh` -> rerun after export refresh
- `./scripts/git-artifact-hygiene-audit.sh --ref HEAD` -> PARTIAL PASS with two blockers:
  - tracked ignored `TestEvidence/*` paths
  - large historical blobs reachable in git history
