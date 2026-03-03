Title: BL-067 Packaging Manifest Evidence
Document Type: Test Evidence
Author: APC Codex
Created Date: 2026-03-03
Last Modified Date: 2026-03-03

# BL-067 Packaging Manifest

- generated_utc: 20260303T005659Z
- lane: BL-067 AUv3 lifecycle and host validation
- mode: execute
- run_index: 1
- expected_bundle_root: TestEvidence/bl067_*/

## Build + Packaging Contract

- cmake_contract_file: /Users/artbox/Documents/Repos/LocusQ-bl073-verify/CMakeLists.txt
- auv3_gate_option: LOCUSQ_ENABLE_AUV3
- apple_formats_contract: VST3 AU Standalone (+AUv3 when LOCUSQ_ENABLE_AUV3=ON)
- signing_contract: AUv3 packaging requires valid Apple signing identity and provisioning in the Xcode-generated project.
- extension_boundary_contract: ADR-0017 (host-name branching forbidden; capability/state driven fallback only)

## Evidence Summary

- host_matrix: PASS=0, BLOCKED=3, FAIL=0
- lifecycle_transitions: PASS=4, FAIL=0
- parity_regression: PASS=4, FAIL=0

## Required Artifacts

- status.tsv
- host_matrix.tsv
- lifecycle_transitions.tsv
- parity_regression.tsv
- packaging_manifest.md
