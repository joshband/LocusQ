Title: APC Workflow: Design
Document Type: Workflow
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

﻿---
description: "PHASE 3: Design - Create UI mockups based on selected framework"
---

Title: Design Phase
Document Type: Workflow
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18


# Design Phase

**Prerequisites:**
```powershell
. "$PSScriptRoot\..\scripts\state-management.ps1"

$state = Get-PluginState -PluginPath "plugins\$PluginName"

if ($state.current_phase -ne "plan_complete") {
    Write-Error "Planning phase not complete. Run /plan first."
    exit 1
}

if ($state.ui_framework -eq "pending") {
    Write-Error "UI framework not selected. Complete /plan first."
    exit 1
}
```

**Framework Router:**

**FOR ALL FRAMEWORKS (Visage and WebView):**
- Load `..codex\skills\design\SKILL.md`
- Create framework-agnostic design specifications and browser-previewable mockup
- NO production framework-specific code generation
- Design phase focuses on creative iteration and approval

**Validation:**
- Verify Design/ folder exists with appropriate files
- Verify v1-ui-spec.md exists
- Verify v1-style-guide.md exists
- Verify `Design/index.html` exists and is browser-previewable
- Verify `Design/HANDOFF.md` exists before marking design complete

## Claude Parity Defaults (Mandatory Unless User Overrides)

1. Run at least three design iterations by default (`v1`, `v2`, `v3`) before final handoff.
2. Each iteration must include all three artifacts: `vN-ui-spec.md`, `vN-style-guide.md`, and `vN-test.html`.
3. If the user approves early, stop immediately and record explicit approval in `Design/HANDOFF.md`.
4. Keep design phase framework-agnostic; do not emit production framework code here.
5. Do not proceed to `/impl` until handoff includes final control map and visual rationale.

**Completion:**
```
âœ… Design phase complete!

Framework: [Visage/WebView]
Design version: v1

Preview commands:
- Visage: .\scripts\preview-design.ps1 -PluginName [Name]
- WebView: Open plugins\[Name]\Design\index.html in browser

Next step: /impl [Name] (after design approval)
```
