#!/usr/bin/env python3
"""Run draft-only backlog T1/T2/T3 packet automation."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "Documentation" / "backlog" / "automation-contracts.json"
STAGE_ORDER = ["t1", "t2", "t3"]


def load_contracts() -> dict:
    return json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))


def merged_stage_commands(contracts: dict, item: dict, stage: str) -> list[str]:
    defaults = contracts.get("defaults", {}).get("commands", {})
    item_commands = item.get("commands", {})
    return list(defaults.get(stage, [])) + list(item_commands.get(stage, []))


def stage_index(stage: str) -> int:
    return STAGE_ORDER.index(stage)


def now_utc() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.rstrip() + "\n", encoding="utf-8")


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


def run_command(command: str, log_path: Path) -> tuple[int, str]:
    proc = subprocess.run(
        command,
        shell=True,
        cwd=ROOT,
        executable=os.environ.get("SHELL", "/bin/zsh"),
        capture_output=True,
        text=True,
    )
    output = []
    if proc.stdout:
        output.append(proc.stdout.rstrip())
    if proc.stderr:
        output.append(proc.stderr.rstrip())
    log = "\n\n".join(part for part in output if part).strip()
    if not log:
        log = "(no output)"
    write_text(log_path, log)
    return proc.returncode, rel(log_path)


def should_emit_handoff(item: dict, stage: str) -> bool:
    if stage != "t3":
        return False
    return item.get("shared_files_risk", "low") != "low"


def build_packet_docs(
    *,
    item_id: str,
    item: dict,
    stage: str,
    runbook: str,
    mode: str,
    out_dir: Path,
    evidence_roots: list[str],
    rows: list[dict],
    result_token: str,
) -> None:
    status_rows = [
        "field\tvalue",
        f"item_id\t{item_id}",
        f"stage\t{stage.upper()}",
        f"automation_mode\t{mode}",
        f"result\t{result_token}",
        f"runbook\t{runbook}",
        f"packet_dir\t{rel(out_dir)}",
    ]
    for idx, evidence in enumerate(evidence_roots, start=1):
        status_rows.append(f"evidence_{idx}\t{evidence}")
    write_text(out_dir / "status.tsv", "\n".join(status_rows))

    matrix = ["stage\tcheck\tresult\texit_code\tlog_path\tcommand"]
    for idx, row in enumerate(rows, start=1):
        matrix.append(
            "\t".join(
                [
                    stage.upper(),
                    f"check_{idx:02d}",
                    row["result"],
                    str(row["exit_code"]),
                    row["log_path"],
                    row["command"],
                ]
            )
        )
    write_text(out_dir / "validation_matrix.tsv", "\n".join(matrix))

    gate_rows = [
        "| Gate | Status | Evidence |",
        "|---|---|---|",
    ]
    for row in rows:
        gate_rows.append(f"| `{row['command']}` | `{row['result']}` | `{row['log_path']}` |")

    promotion = [
        render_metadata(f"{item_id} Automation Promotion Decision Draft", "Promotion Decision Draft"),
        f"# {item_id} Automation Promotion Decision Draft",
        "",
        "## Decision",
        "",
        f"- Result: `{result_token}`",
        f"- Mode: `{mode}`",
        "- This packet is a recommendation. It is not an authoritative promotion decision.",
        "",
        "## Scope Reviewed",
        "",
        f"- Runbook: `{runbook}`",
        "- Existing evidence roots listed below.",
        "- Governance and freshness checks listed in the gate table.",
        "",
        "## Evidence Roots",
        "",
    ]
    promotion.extend(f"- `{path}`" for path in evidence_roots)
    promotion.extend(["", "## Required Gate Matrix", ""])
    promotion.extend(gate_rows)
    promotion.extend(
        [
            "",
            "## Recommendation",
            "",
            "- If all gates are green and the referenced packet still reflects current code, owner may use this packet to prepare closeout.",
            "- If any gate is red, keep the item in its current state and resolve blockers first.",
        ]
    )
    write_text(out_dir / "promotion_decision.md", "\n".join(promotion))

    draft_summary = [
        render_metadata(f"{item_id} Draft Update Summary", "Automation Draft Summary"),
        f"# {item_id} Draft Update Summary",
        "",
        "## Proposed Next State",
        "",
        f"- Recommended token: `{result_token}`",
        "- Draft-only packet assembled.",
        "- No authoritative backlog, archive, or status mutation was applied.",
        "",
        "## Evidence Roots",
        "",
    ]
    draft_summary.extend(f"- `{path}`" for path in evidence_roots)
    draft_summary.extend(
        [
            "",
            "## Proposed Human Review",
            "",
            f"- Review `{rel(out_dir / 'promotion_decision.md')}`.",
            f"- Review `{rel(out_dir / 'validation_matrix.tsv')}` for command outcomes.",
            "- Confirm runbook/index/status/archive transitions manually after owner approval.",
        ]
    )
    write_text(out_dir / "draft_update_summary.md", "\n".join(draft_summary))

    if should_emit_handoff(item, stage):
        owner = [
            render_metadata(f"{item_id} Owner Decisions", "Owner Decisions"),
            f"# {item_id} Owner Decisions",
            "",
            "## Current Recommendation",
            "",
            f"- Result token: `{result_token}`",
            f"- Automation mode: `{mode}`",
            "- Shared-file or coordination risk is non-trivial, so owner review should include overlap safety.",
            "",
            "## Inputs",
            "",
        ]
        owner.extend(f"- `{path}`" for path in evidence_roots)
        write_text(out_dir / "owner_decisions.md", "\n".join(owner))

        handoff = [
            render_metadata(f"{item_id} Handoff Resolution", "Handoff Resolution"),
            f"# {item_id} Handoff Resolution",
            "",
            "## Ownership",
            "",
            "SHARED_FILES_TOUCHED: no",
            "",
            "## Automation Boundary",
            "",
            "- Packet assembly was automated.",
            "- Promotion, archive, and authoritative status changes remain owner-confirmed.",
        ]
        write_text(out_dir / "handoff_resolution.md", "\n".join(handoff))


def main() -> int:
    parser = argparse.ArgumentParser(description="Run draft-only backlog T1/T2/T3 packet automation.")
    parser.add_argument("--id", required=True, help="Backlog ID, for example BL-080")
    parser.add_argument(
        "--stage",
        choices=["t1", "t2", "t3", "all"],
        default="all",
        help="Highest stage to run",
    )
    parser.add_argument("--out-dir", default="", help="Optional explicit output directory")
    args = parser.parse_args()

    item_id = args.id.upper()
    contracts = load_contracts()
    item = contracts.get("items", {}).get(item_id)
    if not item:
        print(f"BLOCKED: missing automation contract for {item_id}", file=sys.stderr)
        return 2

    mode = item.get("automation_mode", contracts["defaults"]["automation_mode"])
    if mode == "manual_only":
        print("MANUAL_ONLY")
        return 0

    stage_cap = item.get("automation_stage_cap", "T1").lower()
    requested = "t3" if args.stage == "all" else args.stage
    if stage_index(requested) > stage_index(stage_cap):
        print(f"BLOCKED: requested stage {requested.upper()} exceeds cap {stage_cap.upper()}", file=sys.stderr)
        return 2

    if args.out_dir:
        out_dir = ROOT / args.out_dir
    else:
        out_dir = ROOT / "TestEvidence" / f"{item_id.lower()}_auto_{requested}_{now_utc()}"

    rows: list[dict] = []
    stages = STAGE_ORDER[: stage_index(requested) + 1] if args.stage == "all" else [requested]
    for stage in stages:
        for command in merged_stage_commands(contracts, item, stage):
            log_path = out_dir / f"{stage}_{len(rows)+1:02d}.log"
            exit_code, log_ref = run_command(command, log_path)
            rows.append(
                {
                    "stage": stage,
                    "command": command,
                    "exit_code": exit_code,
                    "result": "PASS" if exit_code == 0 else "FAIL",
                    "log_path": log_ref,
                }
            )

    result_token = "DRAFT_READY" if rows and all(row["exit_code"] == 0 for row in rows) else "BLOCKED"
    build_packet_docs(
        item_id=item_id,
        item=item,
        stage=requested,
        runbook=item["runbook"],
        mode=mode,
        out_dir=out_dir,
        evidence_roots=item.get("evidence_roots", []),
        rows=rows,
        result_token=result_token,
    )

    print(result_token)
    print(rel(out_dir))
    return 0 if result_token == "DRAFT_READY" else 2


if __name__ == "__main__":
    raise SystemExit(main())
