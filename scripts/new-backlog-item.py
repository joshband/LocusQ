#!/usr/bin/env python3
"""Scaffold backlog intake/runbook docs from templates.

Creates pre-filled files that already include metadata + plain-language 6W fields.
"""

from __future__ import annotations

import argparse
import datetime as dt
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BACKLOG_DIR = ROOT / "Documentation" / "backlog"


def slugify(text: str) -> str:
    text = text.strip().lower()
    text = re.sub(r"[^a-z0-9]+", "-", text)
    text = re.sub(r"-+", "-", text).strip("-")
    return text or "item"


def read_template(name: str) -> str:
    path = BACKLOG_DIR / name
    if not path.exists():
        raise FileNotFoundError(f"Missing template: {path}")
    return path.read_text(encoding="utf-8")


def write_if_absent(path: Path, content: str, overwrite: bool) -> None:
    if path.exists() and not overwrite:
        raise FileExistsError(f"Refusing to overwrite existing file: {path}")
    path.write_text(content, encoding="utf-8")


def fill_intake(template: str, *, item_id: str, title: str, author: str, date: str, priority: str, track: str, intake_path: Path) -> str:
    short_title = f"{item_id} {title}"
    content = template
    content = content.replace("[SHORT TITLE]", short_title)
    content = content.replace("[AUTHOR]", author)
    content = content.replace("[YYYY-MM-DD]", date)
    content = content.replace("[2-3 sentences describing the idea, problem, or opportunity.]", f"{item_id} proposes {title}. This intake captures the problem in plain language, the expected user/operator benefit, and the evidence plan required before promotion.")
    content = content.replace("[P1 / P2 / P3] — [One sentence justification.]", f"{priority} — [One sentence justification.]")
    content = content.replace("[Track A-G from master index, or \"new track needed\"]", track)

    content = content.replace("[1-3 sentences in non-technical language explaining what changes for people and why this matters now.]", f"{item_id} addresses {title}. In simple terms, this backlog item explains what will change, why it matters to operators/users, and how we will prove it works before promotion.")
    content = content.replace("[End users / operators / QA / release owners / coding agents]", "Plugin users, operators, QA/release owners, and coding agents/scripts.")
    content = content.replace("[Simple statement of the change]", title)
    content = content.replace("[Risk reduction, user value, quality, or delivery reason]", "Reduce delivery risk and improve decision confidence with clearer requirements and evidence.")
    content = content.replace("[High-level implementation + validation approach]", "Implement in scoped slices, validate with deterministic replay lanes, and capture evidence under TestEvidence.")
    content = content.replace("[Done signal in plain language]", "Done means acceptance checks pass, evidence is complete, and backlog/index status is synchronized.")
    content = content.replace("[`Documentation/backlog/...` + `TestEvidence/...`]", f"`{intake_path.as_posix()}` (intake), promoted runbook path, and `TestEvidence/...`.")

    content = content.replace("[User request / Research / Regression / Audit finding]", "User request")
    content = content.replace("[Name or agent ID]", author)
    content = content.replace("[BL-XXX, BL-YYY, or \"none known\"]", "none known")
    content = content.replace("[BL-ZZZ, or \"none known\"]", "none known")
    content = content.replace("[T0/T1/T2/T3/T4 per `Documentation/backlog/index.md`]", "T1")
    content = content.replace("[yes/no]", "no")
    content = content.replace("[integer or N/A]", "N/A")
    content = content.replace("[1/3 with rationale]", "3")
    content = content.replace("[5 or owner-approved alternative]", "5")
    content = content.replace("[10 or owner-approved alternative]", "10")
    content = content.replace("[path/glob]", "[FILL]")
    content = content.replace("`TestEvidence/[item]_[slice]_<timestamp>/`", f"`TestEvidence/{item_id.lower()}_<slice>_<timestamp>/`")

    return content


def fill_runbook(template: str, *, item_id: str, title: str, author: str, date: str, priority: str, track: str, runbook_path: Path) -> str:
    content = template
    content = content.replace("BL-XXX", item_id)
    content = content.replace("[TITLE]", title)
    content = content.replace("APC Codex", author)
    content = content.replace("[YYYY-MM-DD]", date)
    content = content.replace("[P1/P2]", priority)
    content = content.replace("[Track X — Name]", track)

    content = content.replace("[1-3 non-technical sentences that explain the change, user/operator impact, and why this work is needed now.]", f"{item_id} covers {title}. This runbook explains the change in plain language, why it matters for users/operators, and how we will verify it safely before promotion.")
    content = content.replace("[Users/operators/QA/release owners/coding agents]", "Plugin users, operators, QA/release owners, and coding agents/scripts.")
    content = content.replace("[Simple plain-language statement]", title)
    content = content.replace("[Risk/value rationale]", "Reduce delivery risk and improve confidence by making behavior and evidence easy to review.")
    content = content.replace("[High-level implementation + validation approach]", "Deliver in slices, run deterministic validation lanes, and capture evidence under TestEvidence.")
    content = content.replace("[Clear outcome/gate in plain language]", "Done means acceptance checks pass, promotion evidence is complete, and backlog/index/status surfaces are synchronized.")
    content = content.replace("[Runbook path + evidence path]", f"`{runbook_path.as_posix()}` and `TestEvidence/{item_id.lower()}_<slice>_<timestamp>/`.")

    content = content.replace("[In Planning / In Progress / In Validation / Done]", "In Planning")
    content = content.replace("[BL-YYY, BL-ZZZ]", "none")
    content = content.replace("[BL-AAA]", "none")
    content = content.replace("[Documentation/plans/bl-XXX-....md]", f"Documentation/plans/{item_id.lower()}-{slugify(title)}-spec-{date}.md")
    content = content.replace("[T0/T1/T2/T3/T4 per `Documentation/backlog/index.md`]", "T1")
    content = content.replace("[Standard / High-cost wrapper]", "Standard")

    return content


def main() -> int:
    parser = argparse.ArgumentParser(description="Create new backlog intake/runbook files from templates.")
    parser.add_argument("--id", required=True, help="Backlog ID (for example BL-078 or HX-07)")
    parser.add_argument("--title", required=True, help="Human-readable title")
    parser.add_argument("--priority", default="P2", choices=["P0", "P1", "P2", "P3"], help="Initial priority")
    parser.add_argument("--track", default="Track TBD", help="Initial owner track label")
    parser.add_argument("--author", default="APC Codex", help="Metadata author")
    parser.add_argument("--date", default=dt.date.today().isoformat(), help="Metadata date (YYYY-MM-DD)")
    parser.add_argument("--slug", default="", help="Optional filename slug override")
    parser.add_argument("--mode", choices=["both", "intake", "runbook"], default="both", help="Which docs to create")
    parser.add_argument("--overwrite", action="store_true", help="Allow overwriting existing files")
    parser.add_argument("--dry-run", action="store_true", help="Print planned outputs without writing")
    args = parser.parse_args()

    item_id = args.id.upper()
    if not re.match(r"^(BL|HX)-\d{3}$", item_id):
        raise SystemExit("--id must match BL-### or HX-###")

    slug = args.slug or slugify(args.title)
    base = f"{item_id.lower()}-{slug}"

    intake_path = BACKLOG_DIR / f"{base}-intake.md"
    runbook_path = BACKLOG_DIR / f"{base}.md"

    plans = []
    if args.mode in {"both", "intake"}:
        intake_template = read_template("_template-intake.md")
        intake_content = fill_intake(
            intake_template,
            item_id=item_id,
            title=args.title,
            author=args.author,
            date=args.date,
            priority=args.priority,
            track=args.track,
            intake_path=intake_path,
        )
        plans.append((intake_path, intake_content))

    if args.mode in {"both", "runbook"}:
        runbook_template = read_template("_template-runbook.md")
        runbook_content = fill_runbook(
            runbook_template,
            item_id=item_id,
            title=args.title,
            author=args.author,
            date=args.date,
            priority=args.priority,
            track=args.track,
            runbook_path=runbook_path,
        )
        plans.append((runbook_path, runbook_content))

    for path, content in plans:
        rel = path.relative_to(ROOT)
        if args.dry_run:
            print(f"DRY-RUN: would write {rel}")
            continue
        write_if_absent(path, content, overwrite=args.overwrite)
        print(f"WROTE: {rel}")

    print("NEXT: add a row in Documentation/backlog/index.md and reconcile dependencies/status surfaces.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
