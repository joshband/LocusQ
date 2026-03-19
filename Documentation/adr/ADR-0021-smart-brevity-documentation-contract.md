Title: ADR-0021 Smart Brevity Documentation Contract
Document Type: Architecture Decision Record
Author: APC Codex
Created Date: 2026-03-18
Last Modified Date: 2026-03-18

# ADR-0021: Smart Brevity Documentation Contract

## Status
Accepted

## Context

LocusQ documentation has accumulated long narrative sections, repeated status prose, and oversized planning or review packets.
Many docs are technically complete but slow to scan.
That hurts:

- human comprehension,
- machine consumption,
- review speed,
- and long-term trust in the active documentation surface.

The repo already has metadata, tiering, backlog readability, and archive rules.
What it lacks is an explicit writing-style contract for concise, high-signal documentation.

## Decision

Adopt a Smart Brevity documentation contract for human-authored markdown in repository-governed documentation surfaces.

Core rules:

1. Lead with the point.
   Put the answer, decision, or outcome first.
2. Prefer short sentences.
   One main idea per sentence whenever practical.
3. Prefer short paragraphs.
   Most paragraphs should be 1-3 sentences.
4. Prefer bullets, tables, and compact labels over long narrative blocks when scannability improves.
5. Use concrete wording.
   Prefer explicit owners, file paths, dates, thresholds, and actions over vague phrasing.
6. Remove filler.
   Cut throat-clearing, repeated caveats, and duplicated policy text when a canonical pointer will do.
7. Use visuals when they help.
   Prefer tables first.
   Use mermaid, screenshots, charts, SVGs, or HTML prototypes only when they clarify faster than prose.
8. Keep visuals portable.
   Prefer Markdown-native or repo-local linked assets.
   Do not rely on inline HTML/CSS color semantics for meaning.
9. Preserve machine readability.
   Keep stable headings, explicit labels, compact tables, and structured outputs (`json`, `csv`, `tsv`, `yaml`) when automation depends on them.
10. Do not confuse brevity with loss of truth.
    Keep nuance, but move detail into tables, appendices, evidence links, or archive bundles instead of burying it inside long paragraphs.

## Scope

This ADR applies to:

- `README.md`
- `CHANGELOG.md`
- `Documentation/`
- `TestEvidence/`
- `.ideas/`
- backlog runbooks and templates
- review, report, testing, and planning markdown that remains in active documentation surfaces

Skill/runtime markdown under `.codex/skills/`, `.claude/skills/`, `.codex/workflows/`, `.claude/workflows/`, `.codex/rules/`, and `.claude/rules/` remains exempt by default unless explicitly requested.

## Consequences

### Positive

- Faster scanning for humans.
- Cleaner input for scripts, agents, and summaries.
- Less duplicate prose across plan, review, and status surfaces.
- Stronger bias toward canonical pointers and compact decision records.

### Costs

- Some legacy docs will need compaction or restructuring.
- Authors must spend more effort deciding what belongs in the active surface versus archive.
- A few docs that currently use long narrative style will need deliberate rewrites.

## Enforcement

- `Documentation/standards.md` is the repository writing-style standard.
- `Documentation/backlog/runbook-authoring-guide.md` carries backlog-specific authoring guidance.
- `documentation-hygiene-expert` and `skill_docs` must enforce this ADR when they are invoked for documentation work.

## Related

- `Documentation/adr/ADR-0001-documentation-governance.md`
- `Documentation/standards.md`
- `Documentation/backlog/runbook-authoring-guide.md`
