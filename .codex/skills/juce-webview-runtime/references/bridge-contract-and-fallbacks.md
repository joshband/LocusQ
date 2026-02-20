Title: Bridge Contract and Fallbacks
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Bridge Contract and Fallbacks

## Contract Requirements
- Stable function names and payload schema.
- Versioning when payload semantics change.
- Explicit timeout behavior for native calls.

## Fallback Requirements
- Browser-only preview must not crash when native bridge is absent.
- UI actions should degrade gracefully with status feedback.
- Critical controls should avoid modal-only UX paths that hosts may suppress.
