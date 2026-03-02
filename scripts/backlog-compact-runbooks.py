#!/usr/bin/env python3
"""Wave-A backlog runbook compaction for active P0/P1 open items.

Compacts repetitive boilerplate while preserving validation/evidence detail.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
INDEX = ROOT / "Documentation/backlog/index.md"


@dataclass
class Row:
    item_id: str
    priority: str
    status: str
    runbook_path: Path


def parse_active_queue(index_text: str) -> list[Row]:
    start = index_text.index("## Active Queue")
    end = index_text.index("## Priority and Parallel Session Safety")
    block = index_text[start:end]
    rows: list[Row] = []
    for line in block.splitlines():
        if not line.startswith("|"):
            continue
        cols = [c.strip() for c in line.strip("|").split("|")]
        if len(cols) < 9 or not cols[0].isdigit():
            continue
        item_id = cols[1]
        priority = cols[3]
        status = cols[4]
        runbook = cols[8]
        m = re.search(r"\(([^)]+)\)", runbook)
        if not m:
            continue
        rel = m.group(1)
        path = ROOT / "Documentation/backlog" / rel
        if not path.exists():
            path = ROOT / "Documentation/backlog" / Path(rel).name
        if not path.exists():
            continue
        rows.append(Row(item_id=item_id, priority=priority, status=status, runbook_path=path))
    return rows


def parse_done_runbooks() -> list[Row]:
    rows: list[Row] = []
    for path in sorted((ROOT / "Documentation/backlog/done").glob("*.md")):
        text = path.read_text(encoding="utf-8")
        id_match = re.search(r"\b((?:BL|HX)-\d{2,3})\b", text)
        item_id = id_match.group(1) if id_match else path.stem.split("-")[0].upper()
        pri_match = re.search(r"^\|\s*Priority\s*\|\s*(.*?)\s*\|\s*$", text, flags=re.M)
        priority = normalize_space(pri_match.group(1)) if pri_match else "P2"
        status = extract_status_from_ledger(text)
        rows.append(Row(item_id=item_id, priority=priority, status=status, runbook_path=path))
    return rows


def normalize_space(s: str) -> str:
    return re.sub(r"\s+", " ", s).strip()


def extract_status_from_ledger(text: str) -> str:
    m = re.search(r"^\|\s*Status\s*\|\s*(.*?)\s*\|\s*$", text, flags=re.M)
    if m:
        return normalize_space(m.group(1).replace("`", "").replace("**", ""))
    return "Open"


def extract_objective_sentence(text: str) -> str:
    m = re.search(r"^## Objective\s*\n(.*?)(?=^##\s|\Z)", text, flags=re.M | re.S)
    if not m:
        return "Deliver the scoped behavior with deterministic validation and evidence."
    block = m.group(1)
    for line in block.splitlines():
        s = line.strip()
        if not s or s.startswith("|") or s.startswith("-"):
            continue
        s = s.replace("`", "")
        s = normalize_space(s)
        if s:
            first = re.search(r"(.+?[.!?])(?:\s|$)", s)
            return first.group(1) if first else s
    return "Deliver the scoped behavior with deterministic validation and evidence."


def replace_section(text: str, heading: str, new_body: str) -> tuple[str, bool]:
    pat = re.compile(rf"^## {re.escape(heading)}\s*\n(.*?)(?=^##\s|\Z)", flags=re.M | re.S)
    if not pat.search(text):
        return text, False
    repl = f"## {heading}\n\n{new_body.strip()}\n\n"
    new_text = pat.sub(repl, text, count=1)
    return new_text, new_text != text


def replace_section_prefix(text: str, heading_prefix: str, new_body: str) -> tuple[str, bool]:
    pat = re.compile(rf"^(## {re.escape(heading_prefix)}[^\n]*?)\n(.*?)(?=^##\s|\Z)", flags=re.M | re.S)
    m = pat.search(text)
    if not m:
        return text, False
    heading_line = m.group(1)
    repl = f"{heading_line}\n\n{new_body.strip()}\n\n"
    new_text = pat.sub(repl, text, count=1)
    return new_text, new_text != text


def build_summary(item_id: str, text: str) -> str:
    status = extract_status_from_ledger(text)
    objective = extract_objective_sentence(text)
    return (
        f"{item_id} in plain terms: {objective} "
        f"Current state: {status}. For technical detail, see `## Objective` and `## Validation Plan`."
    )


def build_visual_aid_index(text: str) -> str:
    rows = [
        "| Visual Aid | Why it helps | Where to find it |",
        "|---|---|---|",
        "| Status ledger | Fast state/priority/dependency scan for humans and agents. | `## Status Ledger` |",
    ]
    if "## Validation Plan" in text:
        rows.append("| Validation and evidence tables | Shows pass/fail criteria and artifact contract. | `## Validation Plan` |")
    if "## Evidence Visual Snapshot" in text:
        rows.append("| Evidence visual snapshot | Consolidated replay/evidence view for promotion decisions. | `## Evidence Visual Snapshot` |")
    if "## Implementation Slices" in text:
        rows.append("| Implementation slices | Clarifies execution sequence and ownership. | `## Implementation Slices` |")
    rows.append("| Optional item-specific diagram | Include only when it clarifies behavior better than prose/tables. | Adjacent to the relevant section |")
    return "Use visuals only when they materially improve understanding.\n\n" + "\n".join(rows)


def compact_flow_diagram() -> str:
    return (
        "Include a runbook-specific diagram only when it clarifies behavior not already obvious from `Status Ledger`, `Implementation Slices`, and `Validation Plan`.\n\n"
        "Canonical lifecycle flow is governed by `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`)."
    )


def compact_handoff() -> str:
    return (
        "Use the canonical handoff block in `Documentation/backlog/index.md` (`Owner Sync Packet Contract`) and include `SHARED_FILES_TOUCHED: no|yes`.\n\n"
        "Only add runbook-specific handoff fields if they differ from the canonical contract."
    )


def compact_governance() -> str:
    return (
        "Canonical lifecycle/evidence rules are defined in:\n"
        "- `Documentation/backlog/index.md` (`Backlog Lifecycle Contract`, `Global Replay Cadence Policy`)\n"
        "- `Documentation/standards.md` (`Backlog Lifecycle Governance Standard`)\n\n"
        "This runbook should list only item-specific exceptions or additions."
    )


def count_lines(path: Path) -> int:
    return path.read_text(encoding="utf-8").count("\n") + 1


def is_done_status(status: str) -> bool:
    s = status.lower()
    return "done" in s and "done-candidate" not in s


def target_rows(rows: Iterable[Row], mode: str) -> list[Row]:
    out = []
    for r in rows:
        done = is_done_status(r.status)
        if mode == "done-all":
            out.append(r)
            continue
        if done:
            continue
        if mode == "p0p1-open" and r.priority not in {"P0", "P1"}:
            continue
        if mode == "remaining-open" and r.priority in {"P0", "P1"}:
            continue
        if mode not in {"p0p1-open", "remaining-open", "all-open"}:
            continue
        out.append(r)
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true", help="Write changes")
    parser.add_argument(
        "--mode",
        choices=["p0p1-open", "remaining-open", "all-open", "done-all"],
        default="p0p1-open",
        help="Target selection mode",
    )
    args = parser.parse_args()

    if args.mode == "done-all":
        source_rows = parse_done_runbooks()
    else:
        source_rows = parse_active_queue(INDEX.read_text(encoding="utf-8"))
    rows = target_rows(source_rows, args.mode)
    changed = 0
    total_before = 0
    total_after = 0

    for row in rows:
        path = row.runbook_path
        text = path.read_text(encoding="utf-8")
        before = count_lines(path)

        summary = build_summary(row.item_id, text)
        text2, c1 = replace_section(text, "Plain-Language Summary", summary)
        text3, c2 = replace_section(text2, "Visual Aid Index", build_visual_aid_index(text2))

        c3 = c4 = c5 = False
        text4 = text3
        if "## Delivery Flow Diagram" in text4:
            text4, c3 = replace_section(text4, "Delivery Flow Diagram", compact_flow_diagram())
        if "## Handoff Return Contract" in text4:
            text4, c4 = replace_section(text4, "Handoff Return Contract", compact_handoff())
        if "## Governance Alignment" in text4:
            text4, c5 = replace_section_prefix(text4, "Governance Alignment", compact_governance())
        elif "## Governance Retrofit" in text4:
            text4, c5 = replace_section_prefix(text4, "Governance Retrofit", compact_governance())

        text4 = re.sub(r"^(Last Modified Date:\s*).*$", "Last Modified Date: 2026-03-02", text4, count=1, flags=re.M)

        if text4 != text:
            changed += 1
            if args.apply:
                path.write_text(text4, encoding="utf-8")
            after = text4.count("\n") + 1
            total_before += before
            total_after += after
            print(f"UPDATED\t{path.relative_to(ROOT)}\t{before}->{after}\tchanges={int(c1)+int(c2)+int(c3)+int(c4)+int(c5)}")

    if changed == 0:
        print("NO_CHANGES")
        return 0

    delta = total_before - total_after
    print(
        f"SUMMARY\tmode={args.mode}\ttargets={len(rows)}\tchanged={changed}\t"
        f"lines_before={total_before}\tlines_after={total_after}\treduction={delta}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
