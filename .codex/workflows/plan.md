Title: APC Workflow: Plan
Document Type: Workflow
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18

﻿---
description: "PHASE 2: Architecture - Define structure and UI framework"
---

Title: Plan Phase (Architecture)
Document Type: Workflow
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18


# Plan Phase (Architecture)

**Prerequisites:**
1. Validate: `Test-PluginState -PluginPath "plugins\$PluginName" -RequiredPhase "ideation"`
2. Check required files exist from ideation phase

**Execute Skill:**
Load and follow `..codex/skills/plan/SKILL.md` exactly.

**CRITICAL UI Framework Decision:**
- Read user requirements
- Determine: VISAGE (pure C++) or WEBVIEW (hybrid)
- Update status.json with framework selection
- Set complexity score (1-5)

**Success Criteria:**
- `status.json` updated with `ui_framework` = "visage" or "webview"
- Architecture document created
- Framework selection rationale documented

**After completion:**
Stop and inform user: "Plan phase complete. Framework selected: [X]. Use `/design [Name]` to continue."
