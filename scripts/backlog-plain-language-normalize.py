#!/usr/bin/env python3
"""Normalize backlog runbooks with plain-language and visual-aid sections.

This script is intentionally additive: it only inserts required readability sections
when they are missing and updates Last Modified Date for touched files.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import re
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
DOC_PATTERNS = (
    "Documentation/backlog/bl-*.md",
    "Documentation/backlog/hx-*.md",
    "Documentation/backlog/done/bl-*.md",
    "Documentation/backlog/done/hx-*.md",
)

REQUIRED_HEADING = "## Plain-Language Summary"


def collapse_whitespace(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def strip_inline_markup(text: str) -> str:
    text = re.sub(r"`([^`]*)`", r"\1", text)
    text = re.sub(r"\[([^\]]+)\]\([^\)]+\)", r"\1", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"\1", text)
    text = re.sub(r"\*([^*]+)\*", r"\1", text)
    return collapse_whitespace(text)


def first_sentence(text: str, fallback: str) -> str:
    clean = strip_inline_markup(text)
    if not clean:
        return fallback
    match = re.search(r"(.+?[.!?])(?:\s|$)", clean)
    sentence = match.group(1).strip() if match else clean
    return sentence[:300]


def find_objective_sentence(content: str) -> str:
    block_match = re.search(r"^## Objective\s*\n(.*?)(?=^##\s|\Z)", content, flags=re.M | re.S)
    if not block_match:
        return "This runbook defines a scoped change with explicit validation and evidence requirements."

    block = block_match.group(1)
    lines = []
    for line in block.splitlines():
        stripped = line.strip()
        if not stripped:
            if lines:
                break
            continue
        if stripped.startswith("|") or stripped.startswith("-"):
            continue
        lines.append(stripped)

    paragraph = " ".join(lines)
    return first_sentence(
        paragraph,
        "This runbook defines a scoped change with explicit validation and evidence requirements.",
    )


def extract_status(content: str) -> str:
    match = re.search(r"^\|\s*Status\s*\|\s*(.*?)\s*\|\s*$", content, flags=re.M)
    if match:
        return strip_inline_markup(match.group(1))
    return "Open"


def extract_h1(content: str) -> str:
    match = re.search(r"^#\s+(.+)$", content, flags=re.M)
    if not match:
        return "Backlog Item"
    return strip_inline_markup(match.group(1))


def extract_id(content: str, title: str) -> str:
    title_match = re.search(r"\b((?:BL|HX)-\d{3})\b", title)
    if title_match:
        return title_match.group(1)
    id_match = re.search(r"\b((?:BL|HX)-\d{3})\b", content)
    if id_match:
        return id_match.group(1)
    return "BL/HX"


def select_how_sentence(content: str) -> str:
    if "## Implementation Slices" in content:
        return "Use the implementation slices and validation plan in this runbook to deliver incrementally and verify each slice before promotion."
    if "## What Was Built" in content:
        return "Use the documented implementation summary and promotion gates in this closeout runbook to confirm what shipped and why it is safe."
    if "## Validation Plan" in content:
        return "Use the validation plan and evidence bundle contract in this runbook to prove behavior and safety before promotion."
    return "Use the runbook steps, validation lanes, and evidence expectations to deliver and verify the work safely."


def select_when_sentence(status: str) -> str:
    lower = status.lower()
    if "done" in lower:
        return "This item is complete when promotion gates, evidence sync, and backlog/index status updates are all recorded as done."
    if "validation" in lower:
        return "This item is complete when required replay gates pass and owner promotion packet decisions are recorded without blockers."
    return "This item is complete when required acceptance criteria, validation lanes, and evidence synchronization are all marked pass."


def build_visual_rows(content: str) -> list[tuple[str, str, str]]:
    rows: list[tuple[str, str, str]] = []
    if "## Status Ledger" in content:
        rows.append((
            "Status Ledger table",
            "Gives a fast plain-language view of priority, state, dependencies, and ownership.",
            "`## Status Ledger`",
        ))
    if "## Validation Plan" in content:
        rows.append((
            "Validation table",
            "Shows exactly how we verify success and safety.",
            "`## Validation Plan`",
        ))
    elif "## Promotion Gate Summary" in content:
        rows.append((
            "Promotion gate table",
            "Shows what passed/failed for closeout decisions.",
            "`## Promotion Gate Summary`",
        ))
    if "## Implementation Slices" in content:
        rows.append((
            "Implementation slices table",
            "Explains step-by-step delivery order and boundaries.",
            "`## Implementation Slices`",
        ))
    rows.append((
        "Optional diagram/screenshot/chart",
        "Use only when it makes complex behavior easier to understand than text alone.",
        "Link under the most relevant section (usually validation or evidence).",
    ))
    return rows


def build_section(path: Path, content: str) -> str:
    title = extract_h1(content)
    item_id = extract_id(content, title)
    status = extract_status(content)
    objective_sentence = find_objective_sentence(content)
    how_sentence = select_how_sentence(content)
    when_sentence = select_when_sentence(status)
    rows = build_visual_rows(content)

    table_rows = "\n".join(f"| {a} | {b} | {c} |" for a, b, c in rows)

    return (
        "## Plain-Language Summary\n\n"
        f"This runbook tracks **{item_id}** (" + title + "). "
        f"Current status: **{status}**. "
        f"In plain terms: {objective_sentence}\n\n"
        "## 6W Snapshot (Who/What/Why/How/When/Where)\n\n"
        "| Question | Plain-language answer |\n"
        "|---|---|\n"
        "| Who is this for? | Plugin users, operators, QA/release owners, and coding agents/scripts that need one reliable source of truth. |\n"
        f"| What is changing? | {title} |\n"
        f"| Why is this important? | {objective_sentence} |\n"
        f"| How will we deliver it? | {how_sentence} |\n"
        f"| When is it done? | {when_sentence} |\n"
        f"| Where is the source of truth? | Runbook: `{path.as_posix()}` plus repo-local evidence under `TestEvidence/...`. |\n\n"
        "## Visual Aid Index\n\n"
        "Use visuals only when they improve understanding; prefer compact tables first.\n\n"
        "| Visual Aid | Why it helps | Where to find it |\n"
        "|---|---|---|\n"
        f"{table_rows}\n"
    )


def update_last_modified_date(content: str, date_value: str) -> str:
    if re.search(r"^Last Modified Date:\s*.*$", content, flags=re.M):
        return re.sub(
            r"^(Last Modified Date:\s*).*$",
            lambda m: f"{m.group(1)}{date_value}",
            content,
            count=1,
            flags=re.M,
        )

    repaired = re.sub(
        r"^P\d{2}-\d{2}-\d{2}$",
        f"Last Modified Date: {date_value}",
        content,
        count=1,
        flags=re.M,
    )
    if repaired != content:
        return repaired

    return re.sub(
        r"^(Created Date:\s*.*)$",
        lambda m: f"{m.group(1)}\nLast Modified Date: {date_value}",
        content,
        count=1,
        flags=re.M,
    )


def insert_after_h1(content: str, section: str) -> str:
    h1 = re.search(r"^#\s+.+$", content, flags=re.M)
    if not h1:
        return content

    start = h1.end()
    next_h2 = re.search(r"\n##\s", content[start:])
    if next_h2:
        insert_at = start + next_h2.start() + 1
    else:
        insert_at = len(content)

    prefix = content[:insert_at].rstrip("\n")
    suffix = content[insert_at:].lstrip("\n")
    return f"{prefix}\n\n{section}\n\n{suffix}" if suffix else f"{prefix}\n\n{section}\n"


def iter_docs(root: Path) -> Iterable[Path]:
    for pattern in DOC_PATTERNS:
        for path in sorted(root.glob(pattern)):
            if path.is_file():
                yield path


def process_file(path: Path, date_value: str, check_only: bool) -> tuple[bool, str]:
    original = path.read_text(encoding="utf-8")
    if re.search(r"^Document Type:\s*Backlog Support\s*$", original, flags=re.M):
        return False, "backlog support document skipped"
    updated = original
    messages: list[str] = []

    if REQUIRED_HEADING not in updated:
        section = build_section(path.relative_to(ROOT), updated)
        updated = insert_after_h1(updated, section)
        messages.append("inserted readability sections")

    date_updated = update_last_modified_date(updated, date_value)
    if date_updated != updated:
        updated = date_updated
        messages.append("updated Last Modified Date")

    if updated == original:
        return False, "no changes"

    if not check_only:
        path.write_text(updated, encoding="utf-8")

    return True, "; ".join(messages)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--date", help="Last Modified Date value (YYYY-MM-DD)")
    parser.add_argument("--check", action="store_true", help="check only; do not write files")
    args = parser.parse_args()

    date_value = args.date or _dt.date.today().isoformat()
    changed = 0
    checked = 0

    for path in iter_docs(ROOT):
        checked += 1
        did_change, reason = process_file(path, date_value, check_only=args.check)
        rel = path.relative_to(ROOT)
        if did_change:
            changed += 1
            action = "WOULD-CHANGE" if args.check else "CHANGED"
            print(f"{action}: {rel} ({reason})")

    print(f"SUMMARY: scanned={checked} changed={changed} mode={'check' if args.check else 'write'}")

    if args.check and changed > 0:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
