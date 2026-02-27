Title: BL-051 Ambisonics and ADM Roadmap
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-051 Ambisonics and ADM Roadmap

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-051 |
| Priority | P3 |
| Status | In Planning |
| Track | E - R&D Expansion |
| Effort | Very High / XL |
| Depends On | BL-046, BL-050 |
| Blocks | — |

## Objective

Define v2 roadmap and ADR decisions for Ambisonics intermediate bus adoption and ADM/IAMF delivery/export readiness.

## Scope

In scope:
- ADR-backed decision package for ambisonics intermediate representation.
- Migration phases for decode/output targets.
- ADM/IAMF interoperability roadmap and risk register.

Out of scope:
- Immediate production implementation in v1.x.
- End-user export UI completion.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | Architecture decision package (ADR) | ADR approved with migration phases |
| B | Prototype and parity lane contract | Prototype lane reports deterministically |
| C | Backlog decomposition for implementation | Follow-on BL items created and linked |

## TODOs

- [ ] Produce ADR for ambisonics intermediate bus decision.
- [ ] Define phased migration and rollback criteria.
- [ ] Define ADM/IAMF interoperability milestones and dependencies.
- [ ] Draft prototype validation lane and evidence schema.
- [ ] Decompose approved roadmap into implementation backlog items.

## Validation Plan

- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `adr_decision.md`
- `migration_plan.tsv`
- `risk_register.tsv`
- `docs_freshness.log`
