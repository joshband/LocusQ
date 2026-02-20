---
name: skill_troubleshooting
description: "Troubleshooting and issue-resolution workflow for APC plugin build and runtime problems."
---

Title: SKILL: TROUBLESHOOTING & ISSUE RESOLUTION
Document Type: Skill
Author: APC Codex
Created Date: 2026-02-18
Last Modified Date: 2026-02-18


# SKILL: TROUBLESHOOTING & ISSUE RESOLUTION

## STEP 1: CHECK KNOWN ISSUES FIRST

**Before trying random solutions:**
```powershell
# Search known issues database
$errorPattern = "duplicate target juce"
$knownIssues = Get-Content ..codex\troubleshooting\known-issues.yaml | ConvertFrom-Yaml

$matches = $knownIssues.issues | Where-Object {
    $_.error_patterns -match $errorPattern
}

if ($matches) {
    Write-Host "âœ“ Known issue found: $($matches.title)"
    Write-Host "Resolution: $($matches.resolution_file)"
    
    # Load and apply solution
    Get-Content "..codex\troubleshooting\$($matches.resolution_file)"
}
```

## STEP 2: ATTEMPT RESOLUTION

[Your existing troubleshooting steps]

## STEP 3: AUTO-CAPTURE NEW ISSUES

If after 3 attempts you haven't solved it:
```powershell
# Create new issue entry
$newIssue = @{
    id = "cmake-$(Get-Random -Max 999)"
    title = "[Auto-generated from error]"
    category = "build"
    severity = "high"
    symptoms = @($errorMessage)
    resolution_status = "investigating"
}

# Append to known-issues.yaml
```

## STEP 4: DOCUMENT SOLUTION

Once resolved:
```powershell
# Update status to "solved"
# Fill out resolution document with:
# - What worked
# - Why it worked
# - How to prevent it
```
