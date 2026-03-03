#!/usr/bin/env python3
"""Export machine-readable backlog summaries for automation and coding agents."""

from __future__ import annotations

import argparse
import csv
import io
import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_JSON = ROOT / "Documentation/reports/data/backlog-summary.json"
DEFAULT_CSV = ROOT / "Documentation/reports/data/backlog-summary.csv"
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
    files: list[Path] = []
    for pattern in GLOBS:
        files.extend(sorted(ROOT.glob(pattern)))
    return files


def parse_sections(text: str) -> list[Section]:
    lines = text.splitlines()
    idxs: list[tuple[int, str]] = []
    for i, line in enumerate(lines):
        m = re.match(r"^##\s+(.*?)\s*$", line)
        if m:
            idxs.append((i, m.group(1).strip()))

    sections: list[Section] = []
    for i, (start, heading) in enumerate(idxs):
        end = idxs[i + 1][0] if i + 1 < len(idxs) else len(lines)
        body = "\n".join(lines[start + 1 : end]).strip()
        sections.append(Section(heading=heading, body=body))
    return sections


def normalize_heading(heading: str) -> str:
    s = heading.strip().lower()
    s = re.sub(r"^\d+[.)]?\s*", "", s)
    s = re.sub(r"\s+", " ", s)
    return s


def section_map(sections: list[Section]) -> dict[str, Section]:
    out: dict[str, Section] = {}
    for section in sections:
        out.setdefault(normalize_heading(section.heading), section)
    return out


def extract_title(text: str, fallback: str) -> str:
    m = re.search(r"^#\s+(.*?)\s*$", text, re.M)
    return m.group(1).strip() if m else fallback


def parse_markdown_table(body: str) -> list[tuple[str, str]]:
    rows: list[tuple[str, str]] = []
    for line in body.splitlines():
        if not line.startswith("|"):
            continue
        cols = [c.strip() for c in line.strip("|").split("|")]
        if len(cols) < 2:
            continue
        if cols[0].lower() in {"field", "question"}:
            continue
        if set(cols[0]) == {"-"}:
            continue
        rows.append((cols[0], cols[1]))
    return rows


def table_to_dict(rows: list[tuple[str, str]]) -> dict[str, str]:
    out: dict[str, str] = {}
    for k, v in rows:
        out[k.strip().lower()] = v.strip()
    return out


def first_paragraph(text: str) -> str:
    lines: list[str] = []
    for line in text.splitlines():
        s = line.strip()
        if not s:
            if lines:
                break
            continue
        if s.startswith("|") or s.startswith("-"):
            continue
        lines.append(s)
    return re.sub(r"\s+", " ", " ".join(lines)).strip()


def sanitize(s: str) -> str:
    return re.sub(r"\s+", " ", s.replace("`", "")).strip()


def parse_6w(rows: list[tuple[str, str]]) -> dict[str, str]:
    out = {"who": "", "what": "", "why": "", "how": "", "when": "", "where": ""}
    for q, a in rows:
        qn = q.lower()
        if "who" in qn:
            out["who"] = sanitize(a)
        elif "what" in qn:
            out["what"] = sanitize(a)
        elif "why" in qn:
            out["why"] = sanitize(a)
        elif "how" in qn:
            out["how"] = sanitize(a)
        elif "when" in qn:
            out["when"] = sanitize(a)
        elif "where" in qn:
            out["where"] = sanitize(a)
    return out


def parse_runbook(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    sections = parse_sections(text)
    smap = section_map(sections)

    title = extract_title(text, path.stem)
    item_id = ""
    id_match = re.search(r"\b((?:BL|HX)-\d{2,3})\b", text)
    if id_match:
        item_id = id_match.group(1)

    summary = sanitize(first_paragraph(smap.get("plain-language summary", Section("", "")).body))
    objective = sanitize(first_paragraph(smap.get("objective", Section("", "")).body))

    ledger_rows = parse_markdown_table(smap.get("status ledger", Section("", "")).body)
    ledger = table_to_dict(ledger_rows)

    sixw_rows = parse_markdown_table(
        smap.get("6w snapshot (who/what/why/how/when/where)", Section("", "")).body
    )
    sixw = parse_6w(sixw_rows)

    state_bucket = "done" if "/done/" in path.as_posix() else "open"

    return {
        "id": item_id,
        "title": title,
        "state_bucket": state_bucket,
        "status": ledger.get("status", ""),
        "priority": ledger.get("priority", ""),
        "track": ledger.get("track", ""),
        "depends_on": ledger.get("depends on", ""),
        "blocks": ledger.get("blocks", ""),
        "who": sixw["who"],
        "what": sixw["what"],
        "why": sixw["why"],
        "how": sixw["how"],
        "when": sixw["when"],
        "where": sixw["where"],
        "plain_language_summary": summary,
        "objective": objective,
        "runbook_path": path.relative_to(ROOT).as_posix(),
    }


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def write_json(path: Path, items: list[dict[str, str]]) -> None:
    payload = {
        "schema_version": "locusq-backlog-summary-v1",
        "source": "Documentation/backlog",
        "counts": {
            "total": len(items),
            "open": sum(1 for item in items if item["state_bucket"] == "open"),
            "done": sum(1 for item in items if item["state_bucket"] == "done"),
        },
        "items": items,
    }
    path.write_text(render_json(payload), encoding="utf-8")


def write_csv(path: Path, items: list[dict[str, str]]) -> None:
    path.write_text(render_csv(items), encoding="utf-8")


def render_json(payload: dict) -> str:
    return json.dumps(payload, indent=2) + "\n"


def render_csv(items: list[dict[str, str]]) -> str:
    fieldnames = [
        "id",
        "title",
        "state_bucket",
        "status",
        "priority",
        "track",
        "depends_on",
        "blocks",
        "who",
        "what",
        "why",
        "how",
        "when",
        "where",
        "plain_language_summary",
        "objective",
        "runbook_path",
    ]
    buffer = io.StringIO()
    writer = csv.DictWriter(buffer, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    writer.writerows(items)
    return buffer.getvalue()


def check_outputs(json_path: Path, csv_path: Path, items: list[dict[str, str]]) -> int:
    payload = {
        "schema_version": "locusq-backlog-summary-v1",
        "source": "Documentation/backlog",
        "counts": {
            "total": len(items),
            "open": sum(1 for item in items if item["state_bucket"] == "open"),
            "done": sum(1 for item in items if item["state_bucket"] == "done"),
        },
        "items": items,
    }
    expected_json = render_json(payload)
    expected_csv = render_csv(items)
    failures = 0

    if not json_path.exists():
        print(f"ERROR: missing summary JSON: {json_path.relative_to(ROOT)}")
        failures += 1
    elif json_path.read_text(encoding="utf-8") != expected_json:
        print(f"ERROR: stale summary JSON: {json_path.relative_to(ROOT)}")
        failures += 1

    if not csv_path.exists():
        print(f"ERROR: missing summary CSV: {csv_path.relative_to(ROOT)}")
        failures += 1
    elif csv_path.read_text(encoding="utf-8") != expected_csv:
        print(f"ERROR: stale summary CSV: {csv_path.relative_to(ROOT)}")
        failures += 1

    if failures:
        print("FIX: run ./scripts/export-backlog-summaries.py to refresh backlog summary artifacts.")
        return 1

    print(
        "PASS: backlog summary artifacts are fresh "
        f"({json_path.relative_to(ROOT)}, {csv_path.relative_to(ROOT)})."
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Export machine-readable backlog summaries.")
    parser.add_argument("--json-out", default=str(DEFAULT_JSON), help="Output JSON path")
    parser.add_argument("--csv-out", default=str(DEFAULT_CSV), help="Output CSV path")
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate that committed summary outputs are present and up to date",
    )
    args = parser.parse_args()

    files = collect_files()
    if not files:
        print("WARN: no backlog runbooks found")
        return 0

    items = [parse_runbook(path) for path in files]
    items.sort(key=lambda x: (x["id"] or x["runbook_path"]))

    json_out = Path(args.json_out)
    csv_out = Path(args.csv_out)

    if args.check:
        return check_outputs(json_out, csv_out, items)

    ensure_parent(json_out)
    ensure_parent(csv_out)
    write_json(json_out, items)
    write_csv(csv_out, items)

    print(f"WROTE: {json_out.relative_to(ROOT)}")
    print(f"WROTE: {csv_out.relative_to(ROOT)}")
    print(f"SUMMARY: total={len(items)} open={sum(1 for item in items if item['state_bucket'] == 'open')} done={sum(1 for item in items if item['state_bucket'] == 'done')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
