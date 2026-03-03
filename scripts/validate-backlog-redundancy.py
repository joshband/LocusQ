#!/usr/bin/env python3
"""Validate backlog runbooks for section redundancy and canonical pointer hygiene."""

from __future__ import annotations

import re
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BACKLOG_INDEX = ROOT / "Documentation/backlog/index.md"
GLOBS = [
    "Documentation/backlog/bl-*.md",
    "Documentation/backlog/hx-*.md",
    "Documentation/backlog/done/bl-*.md",
    "Documentation/backlog/done/hx-*.md",
]


@dataclass
class Section:
    heading: str
    body: str


def collect_files() -> list[Path]:
    out: list[Path] = []
    for pattern in GLOBS:
        out.extend(sorted(ROOT.glob(pattern)))
    return out


def parse_sections(text: str) -> list[Section]:
    lines = text.splitlines()
    headings: list[tuple[int, str]] = []
    for idx, line in enumerate(lines):
        m = re.match(r"^##\s+(.*?)\s*$", line)
        if m:
            headings.append((idx, m.group(1).strip()))

    sections: list[Section] = []
    for i, (start, heading) in enumerate(headings):
        end = headings[i + 1][0] if i + 1 < len(headings) else len(lines)
        body = "\n".join(lines[start + 1 : end]).strip()
        sections.append(Section(heading=heading, body=body))
    return sections


def norm_heading(heading: str) -> str:
    s = heading.strip().lower()
    s = re.sub(r"^\d+[.)]?\s*", "", s)
    s = re.sub(r"\s+", " ", s)
    return s


def file_is_backlog_support(text: str) -> bool:
    return bool(re.search(r"^Document Type:\s*Backlog Support\s*$", text, re.M))


def parse_markdown_table_row(line: str) -> list[str]:
    return [cell.strip() for cell in line.strip().split("|")[1:-1]]


def extract_active_queue_rows(index_text: str) -> list[tuple[int, list[str]]]:
    rows: list[tuple[int, list[str]]] = []
    in_active_queue = False
    for line_no, raw_line in enumerate(index_text.splitlines(), start=1):
        line = raw_line.strip()
        if line.startswith("## Active Queue"):
            in_active_queue = True
            continue
        if in_active_queue and line.startswith("## "):
            break
        if not in_active_queue or not line.startswith("|"):
            continue
        if re.match(r"^\|\s*#\s*\|", line):
            continue
        if re.match(r"^\|\s*[-:]+\s*\|", line):
            continue
        rows.append((line_no, parse_markdown_table_row(line)))
    return rows


def strip_markdown_inline(text: str) -> str:
    out = re.sub(r"`([^`]*)`", r"\1", text)
    out = re.sub(r"\*\*([^*]+)\*\*", r"\1", out)
    out = re.sub(r"\*([^*]+)\*", r"\1", out)
    return out.strip()


def status_is_done(status_cell: str) -> bool:
    normalized = strip_markdown_inline(status_cell).strip().lower()
    return bool(re.match(r"^done(?:\s|\(|$)", normalized))


def extract_runbook_target(runbook_cell: str) -> str:
    m = re.search(r"\[[^\]]+\]\(([^)]+)\)", runbook_cell)
    return m.group(1).strip() if m else ""


def check_active_queue_done_link_contract() -> list[str]:
    if not BACKLOG_INDEX.exists():
        return [f"{BACKLOG_INDEX.relative_to(ROOT)} missing; cannot validate Active Queue link contract"]

    text = BACKLOG_INDEX.read_text(encoding="utf-8")
    rows = extract_active_queue_rows(text)
    if not rows:
        return [
            f"{BACKLOG_INDEX.relative_to(ROOT)} missing parsable Active Queue rows; cannot validate Done runbook link contract"
        ]

    errors: list[str] = []
    for line_no, row in rows:
        if len(row) < 9:
            errors.append(
                f"{BACKLOG_INDEX.relative_to(ROOT)}:{line_no} malformed Active Queue row; expected 9 columns and found {len(row)}"
            )
            continue

        item_id = row[1]
        status_cell = row[4]
        runbook_cell = row[8]
        runbook_target = extract_runbook_target(runbook_cell)
        done_status = status_is_done(status_cell)
        links_done_path = runbook_target.startswith("done/")

        if not runbook_target:
            errors.append(
                f"{BACKLOG_INDEX.relative_to(ROOT)}:{line_no} {item_id} is missing a markdown runbook link target"
            )
            continue

        if done_status and not links_done_path:
            errors.append(
                f"{BACKLOG_INDEX.relative_to(ROOT)}:{line_no} {item_id} is Done but runbook link is not under done/: {runbook_target}"
            )
        if not done_status and links_done_path:
            errors.append(
                f"{BACKLOG_INDEX.relative_to(ROOT)}:{line_no} {item_id} is not Done but runbook link points to done/: {runbook_target}"
            )

    return errors


def check_file(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    if file_is_backlog_support(text):
        return []

    errors: list[str] = []
    sections = parse_sections(text)

    norms = [norm_heading(s.heading) for s in sections]
    counts = Counter(norms)
    dups = sorted([h for h, n in counts.items() if n > 1])
    if dups:
        errors.append(
            f"{path.relative_to(ROOT)} duplicate H2 headings after normalization: {', '.join(dups)}"
        )

    has_alignment = any(h.startswith("governance alignment") for h in norms)
    has_retrofit = any(h.startswith("governance retrofit") for h in norms)
    if has_alignment and has_retrofit:
        errors.append(
            f"{path.relative_to(ROOT)} has both Governance Alignment and Governance Retrofit; keep one canonical section"
        )

    for section in sections:
        h = norm_heading(section.heading)
        body = section.body

        if h.startswith("delivery flow diagram"):
            has_mermaid = "```mermaid" in body
            points_to_canonical = "Documentation/backlog/index.md" in body
            if not (has_mermaid or points_to_canonical):
                errors.append(
                    f"{path.relative_to(ROOT)} Delivery Flow Diagram must include a mermaid diagram or a canonical pointer to Documentation/backlog/index.md"
                )

        if h.startswith("handoff return contract"):
            if "SHARED_FILES_TOUCHED" not in body:
                errors.append(
                    f"{path.relative_to(ROOT)} Handoff Return Contract must include SHARED_FILES_TOUCHED ownership-safety field"
                )

        if h.startswith("governance alignment") or h.startswith("governance retrofit"):
            if "Documentation/backlog/index.md" not in body or "Documentation/standards.md" not in body:
                errors.append(
                    f"{path.relative_to(ROOT)} {section.heading} must reference both Documentation/backlog/index.md and Documentation/standards.md"
                )

    return errors


def main() -> int:
    files = collect_files()
    if not files:
        print("WARN: no backlog runbooks found for redundancy validation", file=sys.stderr)
        return 0

    all_errors: list[str] = []
    for path in files:
        all_errors.extend(check_file(path))
    all_errors.extend(check_active_queue_done_link_contract())

    if all_errors:
        for msg in all_errors:
            print(f"ERROR: {msg}", file=sys.stderr)
        print(f"FAIL: backlog redundancy checks found {len(all_errors)} issue(s).", file=sys.stderr)
        return 1

    print(f"PASS: backlog redundancy checks passed for {len(files)} runbook(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
