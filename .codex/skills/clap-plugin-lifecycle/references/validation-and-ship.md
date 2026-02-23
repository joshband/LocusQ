Title: CLAP Validation and Ship Checklist
Document Type: Reference
Author: APC Codex
Created Date: 2026-02-22
Last Modified Date: 2026-02-22

# Validation Matrix

| Lane | Goal | Command Pattern | Required |
|---|---|---|---|
| Build configure | Confirm CLAP wiring and target graph | `cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release` | Yes |
| Target discovery | Confirm CLAP target(s) exist | `cmake --build build_local --config Release --target help | rg -i "clap|locusq"` | Yes |
| CLAP artifact build | Produce `.clap` bundle/binary | host-specific CMake target command | Yes |
| CLAP inspection | Verify descriptor/metadata | `clap-info <artifact.clap>` | Recommended |
| CLAP validation | Format/threading contract checks | `clap-validator <artifact.clap>` | Recommended |
| Regression safety | Guard existing formats | existing `pluginval` and harness scenarios | Yes |
| Docs freshness | Keep metadata/routing docs coherent | `./scripts/validate-docs-freshness.sh` | Yes |

# Capability-Mode Tests

Validate adapter behavior across negotiated CLAP capability surfaces:

- `PolyVoice`: voice-info + poly modulation/note expression available.
- `VoiceOnly`: voice-info available, no per-voice modulation.
- `GlobalOnly`: no stable voice metadata; deterministic global fallback.

For each mode verify:
- no crashes or undefined routing behavior
- deterministic voice allocation/reuse policy
- stable fallback semantics and telemetry output shape

# Ship Closeout Checklist

1. Confirm CLAP artifact is produced and versioned in release outputs.
2. Confirm CLAP validation lane outputs are attached to evidence docs.
3. Confirm VST3/AU validation remains green.
4. Update BL-011 status with date and evidence references.
5. Update changelog/release notes for new CLAP format support.
