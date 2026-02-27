Title: BL-048 Cross-Platform Shipping Hardening
Document Type: Backlog Runbook
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# BL-048 Cross-Platform Shipping Hardening

## Status Ledger

| Field | Value |
|---|---|
| ID | BL-048 |
| Priority | P1 |
| Status | In Planning |
| Track | G - Release/Governance |
| Effort | High / L |
| Depends On | BL-030, BL-042 |
| Blocks | Release hardening beyond macOS dev packaging |

## Objective

Harden shipping readiness via code-sign/notarization, Windows build validation, and installer packaging so release governance includes cross-platform distribution confidence.

## Scope

In scope:
- macOS signing/notarization workflow contract.
- Windows build/runtime validation lane contract.
- Installer packaging contract (DMG/PKG/MSI guidance).

Out of scope:
- Updater service design.
- Store-specific release automation.

## Implementation Slices

| Slice | Description | Exit Criteria |
|---|---|---|
| A | macOS code-sign/notarization lane | Signed/notarized artifact validation passes |
| B | Windows build validation lane | Build + standalone/VST3 smoke evidence captured |
| C | Installer packaging + governance integration | Release packet includes installer artifacts and checks |

## TODOs

- [ ] Define and validate code-sign/notarization lane contract.
- [ ] Execute Windows build validation and capture runtime/WebView evidence.
- [ ] Define installer packaging artifact and checksum contracts.
- [ ] Integrate cross-platform artifacts into BL-030 governance packet.
- [ ] Produce owner promotion packet for cross-platform readiness.

## Validation Plan

- `./scripts/qa-bl030-release-gate-mac.sh`
- `./scripts/qa-bl048-windows-build-validate.sh --out-dir TestEvidence/bl048_<slice>_<timestamp>`
- `./scripts/validate-docs-freshness.sh`

## Evidence Contract

- `status.tsv`
- `release_gate_matrix.tsv`
- `codesign_notary.log`
- `windows_build_validation.md`
- `installer_manifest.tsv`
- `docs_freshness.log`
