Title: BL-032 Modularization Structural Guardrails QA
Document Type: Testing Guide
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-032 Modularization Structural Guardrails QA

## Purpose
Define deterministic QA guardrails for BL-032 Slice C to prevent regressions toward monolithic `PluginProcessor`/`PluginEditor` architecture and enforce Slice A module ownership boundaries.

## Guardrail Lane

Script:
- `scripts/qa-bl032-structure-guardrails-mac.sh`

Primary artifacts:
1. `status.tsv`
2. `guardrail_report.tsv`
3. `blocker_taxonomy.tsv`
4. `guardrail_contract.md`

## Guard IDs and Thresholds

| Guard ID | Category | Rule | Pass Condition |
|---|---|---|---|
| BL032-G-001 | `line_count_threshold` | `Source/PluginProcessor.cpp` max lines | line count <= 3200 |
| BL032-G-002 | `line_count_threshold` | `Source/PluginEditor.cpp` max lines | line count <= 800 |
| BL032-G-101 | `forbidden_dependency_edge` | `PluginProcessor.cpp/.h` forbidden include: `PluginEditor.h` | zero matches |
| BL032-G-102 | `forbidden_dependency_edge` | `Source/shared_contracts/*` forbidden include: `PluginProcessor.h` or `PluginEditor.h` | zero matches |
| BL032-G-103 | `forbidden_dependency_edge` | `Source/processor_core/*` forbidden include: `PluginEditor.h` | zero matches |
| BL032-G-104 | `forbidden_dependency_edge` | `Source/processor_bridge/*` forbidden include: `PluginEditor.h` | zero matches |
| BL032-G-105 | `forbidden_dependency_edge` | `Source/editor_webview/*` forbidden include: `PluginProcessor.h` | zero matches |
| BL032-G-106 | `forbidden_dependency_edge` | `Source/editor_shell/*` forbidden include: `SpatialRenderer.h` | zero matches |
| BL032-G-201 | `required_module_directory` | `Source/shared_contracts` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-202 | `required_module_directory` | `Source/processor_core` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-203 | `required_module_directory` | `Source/processor_bridge` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-204 | `required_module_directory` | `Source/editor_shell` presence | directory exists with >=1 `*.h`/`*.cpp` |
| BL032-G-205 | `required_module_directory` | `Source/editor_webview` presence | directory exists with >=1 `*.h`/`*.cpp` |

## Validation Commands

```bash
bash -n scripts/qa-bl032-structure-guardrails-mac.sh
./scripts/qa-bl032-structure-guardrails-mac.sh --help
./scripts/qa-bl032-structure-guardrails-mac.sh --out-dir TestEvidence/bl032_slice_c_guardrails_<timestamp>
./scripts/validate-docs-freshness.sh
```

## Exit and Failure Contract

1. Exit `0`: all guard IDs pass.
2. Exit `1`: one or more guard IDs fail.
3. Exit `2`: lane usage/configuration error.

`guardrail_report.tsv` is authoritative for per-guard decisions; `blocker_taxonomy.tsv` groups failures by deterministic blocker class.

## Remediation Steps

1. `line_count_threshold` violations:
   - Extract logic from monolithic file into the Slice A module boundaries.
   - Re-run guardrails after each extraction tranche.
2. `forbidden_dependency_edge` violations:
   - Remove direct include edge.
   - Route dependency through the allowed module interface direction from the Slice A map.
3. `required_module_directory` violations:
   - Create missing module directory and add at least one owned header/source file.
   - Ensure ownership aligns with BL-032 Slice B/C no-overlap contract.

## Determinism Notes

1. Guard IDs are stable and machine-readable.
2. Threshold values are explicit and versionable.
3. The lane emits fixed-schema TSV outputs suitable for CI gating and promotion packets.
