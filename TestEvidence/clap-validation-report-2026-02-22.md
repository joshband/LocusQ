Title: LocusQ CLAP Validation Report
Document Type: Test Report
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# LocusQ CLAP Validation Report

## Scope
Validate the current CLAP artifact for BL-011 using external CLAP tooling.

## Artifact Under Test
- Bundle: `build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap`
- Bundle timestamp: `2026-02-22 12:11:31 -0500`
- Binary hash file: `TestEvidence/clap_validation_20260222T181504Z/artifact_sha256.txt`

## Tooling
- `clap-info`: `/Users/artbox/.local/bin/clap-info` (`0.9.0`)
- `clap-validator`: `/Users/artbox/.local/bin/clap-validator` (`0.3.2`)

## Commands
```sh
clap-info build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
clap-validator validate build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
clap-validator validate --json build_local/LocusQ_artefacts/Release/CLAP/LocusQ.clap
```

## Result
- Initial run (`TestEvidence/clap_validation_20260222T181504Z`)
  - `clap-info`: `PASS`
  - `clap-validator`: `FAIL`
  - Summary: `21 tests run, 15 passed, 1 failed, 5 skipped, 0 warnings`
  - Failing test: `param-set-wrong-namespace`
  - Failure detail: plugin parameter values changed when `CLAP_EVENT_PARAM_VALUE` events were sent with mismatching namespace ID.

## Fix Applied
- File: `Source/PluginProcessor.cpp`
- Change:
  - Prevent CLAP activation-time parameter mutation for `emit_color` seeding (`is_clap` gate in `syncSceneGraphRegistrationForMode()`).
  - Added explicit CLAP direct-event handling overrides for `CLAP_EVENT_PARAM_VALUE` with strict core-namespace filtering.

## Revalidation Result
- Revalidation run (`TestEvidence/clap_validation_20260222T182619Z`)
  - `clap-info`: `PASS`
  - `clap-validator`: `PASS`
  - Summary: `21 tests run, 16 passed, 0 failed, 5 skipped, 0 warnings`
  - `param-set-wrong-namespace`: `PASSED`

## Artifacts
- `TestEvidence/clap_validation_20260222T181504Z/clap-info.json`
- `TestEvidence/clap_validation_20260222T181504Z/clap-validator.txt`
- `TestEvidence/clap_validation_20260222T181504Z/clap-validator.json`
- `TestEvidence/clap_validation_20260222T181504Z/clap-validator-quiet.json`
- `TestEvidence/clap_validation_20260222T181504Z/clap-validator-quiet.cleaned.json`
- `TestEvidence/clap_validation_20260222T181504Z/clap-validator-quiet.stderr.log`
- `TestEvidence/clap_validation_20260222T181504Z/artifact_stat.txt`
- `TestEvidence/clap_validation_20260222T181504Z/artifact_sha256.txt`
- `TestEvidence/clap_validation_20260222T182619Z/clap-info.json`
- `TestEvidence/clap_validation_20260222T182619Z/clap-validator.txt`
- `TestEvidence/clap_validation_20260222T182619Z/clap-validator.json`
- `TestEvidence/clap_validation_20260222T182619Z/clap-validator-quiet.json`
- `TestEvidence/clap_validation_20260222T182619Z/clap-validator-quiet.cleaned.json`
- `TestEvidence/clap_validation_20260222T182619Z/clap-validator-quiet.stderr.log`
- `TestEvidence/clap_validation_20260222T182619Z/artifact_stat.txt`
- `TestEvidence/clap_validation_20260222T182619Z/artifact_sha256.txt`

## Parsing Note
- `clap-validator --json` output in this environment is prefixed by a JUCE CLAP wrapper warning line.
- `clap-validator-quiet.cleaned.json` strips the prefix so downstream tooling can parse valid JSON.

## Closeout Impact
- External CLAP validation gate (`clap-info` + `clap-validator`) is now green for the current BL-011 artifact.
- BL-011 remains `In Progress` for remaining lifecycle scope outside this validator gate.
