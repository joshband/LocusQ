Title: LocusQ Phase 2.3 Validation
Document Type: Validation Report
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

# LocusQ Phase 2.3 Validation (Codex Continuation)

Date: 2026-02-18

## Scope validated
- Phase 2.1 foundation integration remains build-stable.
- Phase 2.2 spatial path remains QA-pass.
- Phase 2.3 room-calibration implementation and UI bridge are present and integrated.

## Key remediation performed
- Normalized LocusQ QA scenarios to current harness stimulus contracts:
  - `white_noise/default` -> `noise/white`
  - `sine_wave/default` -> `sweep/linear_sine` with fixed 440 Hz via `start_hz=end_hz=440`
- Fixed `locusq_qa` runner wiring bug in `plugins/LocusQ/qa/main.cpp`:
  - runner factory now uses the scenario executor's effective wrapped DUT factory.
  - This restores stimulus injection + parameter variation wrappers required by harness execution.

## Evidence
- Harness ctest (audio-dsp-qa-harness):
  - Log: `plugins/LocusQ/TestEvidence/harness_ctest.log`
  - Result: 45/45 passed.
- LocusQ smoke suite:
  - Run log: `plugins/LocusQ/TestEvidence/locusq_qa_run.log`
  - Suite JSON: `plugins/LocusQ/qa_output/locusq_emitter/suite_result.json`
  - Result: 4/4 passed.
- Plugin build:
  - `plugins/LocusQ/TestEvidence/locusq_build.log`
  - `plugins/LocusQ/TestEvidence/locusq_vst3_build.log`
  - `plugins/LocusQ/TestEvidence/locusq_qa_build.log`
- pluginval:
  - Prior GUI-context success evidence:
    - `plugins/LocusQ/TestEvidence/pluginval_exit_code.txt` = 0
    - `plugins/LocusQ/TestEvidence/pluginval_stdout.log` contains SUCCESS.
  - Current restricted/headless blocked run evidence:
    - `plugins/LocusQ/TestEvidence/pluginval_blocked_note.txt`
    - `plugins/LocusQ/TestEvidence/pluginval_blocked_exit_code.txt`
    - `plugins/LocusQ/TestEvidence/pluginval_blocked_stdout.log`
    - `plugins/LocusQ/TestEvidence/pluginval_blocked_stderr.log`

## Outcome
- Phase 2.3 validation artifacts are complete and linked in `plugins/LocusQ/status.json`.
- Project state intentionally remains `code_in_progress` for active Phase 2.4 continuation.
