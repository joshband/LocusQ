Title: Git Artifact Hygiene Automation Reference
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Git Artifact Hygiene Automation

## Goal
Prevent repository drift from generated artifacts, stale archive bundles, and oversized blobs in reachable history.

## Local Cleanup Loop
1. Run a full audit:
   - `./scripts/git-artifact-hygiene-audit.sh --ref HEAD`
2. Generate cleanup candidates from tracked ignored/archive files:
   - `./scripts/git-artifact-cleanup-index.sh --manifest TestEvidence/git_artifact_cleanup_candidates.tsv`
3. Review candidate list and apply index cleanup when approved:
   - `./scripts/git-artifact-cleanup-index.sh --apply --manifest TestEvidence/git_artifact_cleanup_candidates.tsv`
4. Verify staged delta does not include blocked artifacts:
   - `./scripts/git-artifact-hygiene-guard.sh`

## Main Branch Cleanup Strategy (No History Rewrite)
1. Land `.gitignore` and guardrail scripts first.
2. Land tracked-index cleanup (`git rm --cached`) in a dedicated commit.
3. Keep cleanup commit scoped to artifact removal plus guard scripts/workflow only.
4. Re-run:
   - `./scripts/git-artifact-hygiene-audit.sh --ref HEAD`
   - `./scripts/validate-docs-freshness.sh`

## Reachable History Rewrite Strategy (High Risk)
Use only in a dedicated maintenance window.

1. Freeze pushes and notify collaborators.
2. Create mirror backup:
   - `git clone --mirror <repo-url> locusq-backup.git`
3. Rewrite with `git filter-repo` in a fresh clone (example targets):
   - `third_party/steam-audio/steamaudio_*.zip`
   - build-output binaries under `build*/` and `build_bl*/`
   - accidental archive bundles under `TestEvidence/archive/`
4. Validate rewritten history:
   - `./scripts/git-artifact-hygiene-audit.sh --strict --ref HEAD`
5. Force-push branches/tags with explicit team sign-off.
6. Require fresh clone for all collaborators after rewrite.

## Guard Automation
- Local pre-commit hook:
  - `./scripts/install-git-hygiene-hooks.sh`
- CI guard + advisory full audit:
  - `.github/workflows/git-artifact-hygiene.yml`

## Safety Rules
- Never mix history rewrite with feature delivery commits.
- Keep artifact cleanup commits independent from implementation changes.
- Treat forced pushes as coordinated operations with rollback backup retained.
