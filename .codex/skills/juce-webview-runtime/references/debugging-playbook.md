Title: JUCE WebView Debugging Playbook
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# Debugging Playbook

## Steps
1. Confirm event listener binding and DOM target existence.
2. Trace JS event -> native call -> native response -> UI status update.
3. Capture timeout/error telemetry and host logs.
4. Verify callback ordering around startup hydration.
5. Re-test with minimal UI and minimal plugin state.

## Typical Problem Classes
- Click not landing due to overlay/hit-testing/focus behavior.
- Native function unavailable or timing out.
- State write succeeds but UI state cache is stale.
