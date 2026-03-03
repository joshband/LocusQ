Title: BL-073 Promotion Gate Policy
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-03
Last Modified Date: 2026-03-03

# BL-073 Promotion Gate Policy

- Generated: 20260303T005659Z
- Scope: BL-067 and BL-068 execute-mode truthfulness
- Mode under test: execute
- Replay runs: 3

## Exact Execute Failure Criteria

1. For each lane run, count rows in lane TSV artifacts where any cell is `TODO` or `SCAFFOLD`.
2. If any execute-mode run contains `TODO`/`SCAFFOLD` rows, BL-073 returns non-zero and blocks promotion.
3. In execute mode, `scaffold_rows > 0` and lane exit code `0` is a hard failure (false-green).
4. In execute mode, `scaffold_rows = 0` and lane exit code non-zero is a hard failure (false-red).
5. Execute-mode pass criteria requires both: zero scaffold rows across all runs and exit parity for each lane run.

## Contract-Only Semantics

1. Contract-only runs may contain `TODO`/`SCAFFOLD` rows.
2. Contract-only runs must still exit `0`.

## Promotion Packet Requirements

Promotion packets for BL-067/BL-068 must include:

- `mode_semantics_contract.tsv`
- `todo_row_enforcement.tsv`
- `bl067_bl068_matrix_reconcile.tsv`
- `promotion_gate_policy.md`
