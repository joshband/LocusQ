#!/usr/bin/env python3
"""Prepare a draft closeout diff summary from a draft-ready automation packet."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "Documentation" / "backlog" / "automation-contracts.json"
INDEX_PATH = ROOT / "Documentation" / "backlog" / "index.md"
STATUS_PATH = ROOT / "status.json"
BUILD_SUMMARY_PATH = ROOT / "TestEvidence" / "build-summary.md"
VALIDATION_TREND_PATH = ROOT / "TestEvidence" / "validation-trend.md"


def load_contracts() -> dict:
    return json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def now_utc() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def render_metadata(title: str, doc_type: str) -> str:
    today = dt.date.today().isoformat()
    return "\n".join(
        [
            f"Title: {title}",
            f"Document Type: {doc_type}",
            "Author: APC Codex",
            f"Created Date: {today}",
            f"Last Modified Date: {today}",
            "",
        ]
    )


def extract_index_row(item_id: str) -> str:
    text = INDEX_PATH.read_text(encoding="utf-8")
    matches = re.findall(rf"^\| .* \| {re.escape(item_id)} \|.*$", text, flags=re.MULTILINE)
    if not matches:
        return "(row not found)"
    for line in reversed(matches):
        if f"[{item_id.lower()}]" in line or "**Done**" in line or "Done-candidate" in line or "In Validation" in line or "Open" in line:
            return line
    return matches[-1]


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare a draft closeout diff summary.")
    parser.add_argument("--id", required=True, help="Backlog ID, for example BL-080")
    parser.add_argument("--packet", required=True, help="Path to a draft automation packet")
    parser.add_argument("--out-dir", default="", help="Optional explicit output directory")
    args = parser.parse_args()

    item_id = args.id.upper()
    packet_dir = ROOT / args.packet
    status_tsv = packet_dir / "status.tsv"
    if not status_tsv.exists():
        raise SystemExit(f"Missing packet status file: {status_tsv}")

    status_text = status_tsv.read_text(encoding="utf-8")
    if "\tDRAFT_READY" not in status_text:
        raise SystemExit("Packet is not DRAFT_READY")

    contracts = load_contracts()
    item = contracts.get("items", {}).get(item_id)
    if not item:
        raise SystemExit(f"Missing automation contract for {item_id}")

    runbook = ROOT / item["runbook"]
    done_dir = ROOT / "Documentation" / "backlog" / "done"
    target_done_path = done_dir / runbook.name
    if args.out_dir:
        out_dir = ROOT / args.out_dir
    else:
        out_dir = ROOT / "TestEvidence" / f"{item_id.lower()}_closeout_draft_{now_utc()}"

    move_action = (
        f"Move `{rel(runbook)}` -> `{rel(target_done_path)}`"
        if runbook.exists() and runbook.parent != done_dir
        else f"Runbook already under done: `{rel(runbook)}`"
    )

    actions = [
        ("runbook_path", rel(runbook)),
        ("packet_path", rel(packet_dir)),
        ("move_action", move_action),
        ("index_action", f"Update `{rel(INDEX_PATH)}` row for {item_id} to final Done wording/path"),
        ("status_action", f"Append draft owner-confirmed note in `{rel(STATUS_PATH)}`"),
        ("build_summary_action", f"Add closeout row in `{rel(BUILD_SUMMARY_PATH)}`"),
        ("validation_trend_action", f"Add closeout entry in `{rel(VALIDATION_TREND_PATH)}`"),
    ]

    tsv = ["field\tvalue"]
    tsv.extend(f"{key}\t{value}" for key, value in actions)
    write_text(out_dir / "closeout_actions.tsv", "\n".join(tsv))

    summary = [
        render_metadata(f"{item_id} Closeout Draft Summary", "Closeout Draft Summary"),
        f"# {item_id} Closeout Draft Summary",
        "",
        "## Current Packet",
        "",
        f"- Source packet: `{rel(packet_dir)}`",
        f"- Runbook: `{rel(runbook)}`",
        "",
        "## Proposed Closeout Actions",
        "",
    ]
    summary.extend(f"- {value}" for _, value in actions[2:])
    summary.extend(
        [
            "",
            "## Current Backlog Index Row",
            "",
            extract_index_row(item_id),
            "",
            "## Rule",
            "",
            "- This file is a draft diff summary only.",
            "- No authoritative status, archive, or runbook move has been applied.",
        ]
    )
    write_text(out_dir / "closeout_diff_summary.md", "\n".join(summary))

    print("DRAFT_READY")
    print(rel(out_dir))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
