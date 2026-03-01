Title: LocusQ ADR Index
Document Type: ADR Index
Author: APC Codex
Created Date: 2026-03-01
Last Modified Date: 2026-03-01

# Architecture Decision Records Index

## Purpose

Provide a compact index of accepted architecture decisions and make ADR discovery
deterministic during implementation, review, and closeout work.

## ADR Catalog

| ADR | Title | Status | Primary Scope |
|---|---|---|---|
| `ADR-0001` | Documentation Governance Baseline | Accepted | documentation governance baseline |
| `ADR-0002` | Routing Model V1 | Accepted | SceneGraph routing and same-block audio snapshot handoff |
| `ADR-0003` | Automation Authority Precedence | Accepted | APVTS -> timeline -> physics authority layering |
| `ADR-0004` | V1 AI Deferral | Accepted | no AI in critical runtime path for v1 closure |
| `ADR-0005` | Phase Closeout Docs Freshness Gate | Accepted | closeout sync bundle and freshness enforcement |
| `ADR-0006` | Device Compatibility Profiles and Monitoring Contract | Accepted | device profile and portable monitoring contract |
| `ADR-0007` | Emitter Directivity and Initial Velocity UI Exposure | Accepted | production UI exposure for directivity/velocity controls |
| `ADR-0008` | Viewport Scope v1 vs Post-v1 | Accepted | viewport scope and telemetry overlay posture |
| `ADR-0009` | CLAP Closeout Documentation Consolidation | Accepted | BL-011 CLAP closeout documentation authority |
| `ADR-0010` | Repository Artifact Tracking and Retention Policy | Accepted | tracked/local-only artifact lifecycle policy |
| `ADR-0011` | Standalone Renderer Audition Source | Accepted | renderer audition authority and deterministic control surface |
| `ADR-0012` | Renderer Domain Exclusivity and Matrix Gating | Accepted | renderer domain legality and matrix fallback policy |
| `ADR-0013` | Audition Authority and Cross-Mode Control | Accepted | renderer-authoritative audition with proxy control model |
| `ADR-0014` | BL-051 Ambisonics + ADM Roadmap Governance | Accepted | phase-gated ambisonics/ADM roadmap governance |
| `ADR-0015` | Skill Runtime Doc Standards Boundary | Accepted | repository docs standards vs skill/workflow/rule runtime boundary |
| `ADR-0016` | Head-Tracking Wire Protocol Compatibility and Sunset Policy | Accepted | v1/v2 packet compatibility and deprecation gate policy |
| `ADR-0017` | AUv3 App-Extension Boundary and Lifecycle Contract | Accepted | AUv3 extension boundary and cross-format parity contract |
| `ADR-0018` | Temporal Effects Realtime Architecture Contract | Accepted | delay/echo/looper safety and deterministic temporal DSP contract |
| `ADR-0019` | Custom SOFA Profile Readiness and Fallback Contract | Accepted | capability-gated custom-SOFA activation and deterministic fallback |

## Usage Notes

1. Treat ADRs as canonical architecture decisions, not optional guidance.
2. If implementation behavior deviates from an accepted ADR, add a new ADR or update
   the existing ADR in the same change set.
3. Keep ADR references synchronized in `ARCHITECTURE.md`, `Documentation/invariants.md`,
   and active runbooks/specs when decision scope changes.
