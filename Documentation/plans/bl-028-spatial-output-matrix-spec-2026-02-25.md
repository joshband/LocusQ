Title: BL-028 Spatial Output Matrix Enforcement Spec
Document Type: Plan
Author: APC Codex
Created Date: 2026-02-25
Last Modified Date: 2026-02-25

# BL-028 Spatial Output Matrix Enforcement Spec (Slice A1)

## Purpose
Define an implementation-ready, deterministic spatial output matrix contract for BL-028 covering legal/illegal routing behavior, mismatch fallback policy, diagnostics schema, and QA acceptance boundaries.

## Authority and Scope
- Backlog runbook: `Documentation/backlog/done/bl-028-spatial-output-matrix.md`
- QA contract: `Documentation/testing/bl-028-spatial-output-matrix-qa.md`
- ADR guardrails:
  - `Documentation/adr/ADR-0006-device-compatibility-profiles-and-monitoring-contract.md`
  - `Documentation/adr/ADR-0012-renderer-domain-exclusivity-and-matrix-gating.md`

This contract is normative for BL-028 implementation slices and must be mirrored by QA lane assertions.

## Acceptance IDs (Normative)

| Acceptance ID | Contract Statement |
|---|---|
| BL028-A1-001 | Authoritative matrix behavior is explicitly defined for `binaural(stereo)`, `stereo`, `quad`, `5.1`, `7.1`, and `7.4.2` layouts. |
| BL028-A1-002 | Mismatch handling defines deterministic fallback mode precedence and fail-safe routing behavior. |
| BL028-A1-003 | Diagnostics payload fields and enums are fully specified for requested/active/rule/fallback/state visibility. |
| BL028-A1-004 | User-visible status text mapping is deterministic and reason-code driven. |
| BL028-A1-005 | Deterministic QA lane contract includes scenario set, artifact schema, and pass/fail thresholds. |
| BL028-A1-006 | Acceptance IDs are cross-referenced consistently across runbook, spec, and QA docs. |

## Spatial Output Matrix (Authoritative)

### Domain Enum
```cpp
enum class RendererDomain
{
    InternalBinaural,
    Multichannel,
    ExternalSpatial
};
```

### Host Layout Keys
- `stereo_2_0` (2 channels)
- `quad_4_0` (4 channels)
- `surround_5_1` (6 channels)
- `surround_7_1` (8 channels)
- `immersive_7_4_2` (12 channels)

### Matrix Behavior

| Rule ID | Requested Domain | Host Layout | Head Tracking | Decision | Active Domain | Route Policy |
|---|---|---|---|---|---|---|
| SOM-028-01 | InternalBinaural | stereo_2_0 | off | ALLOW | InternalBinaural | Internal HRTF/binaural stereo render |
| SOM-028-02 | InternalBinaural | stereo_2_0 | on | ALLOW | InternalBinaural | Internal binaural + bridge pose |
| SOM-028-03 | InternalBinaural | quad_4_0 / 5.1 / 7.1 / 7.4.2 | any | BLOCK | fallback | `binaural_requires_stereo` |
| SOM-028-04 | Multichannel | quad_4_0 | off | ALLOW | Multichannel | Discrete 4ch routing |
| SOM-028-05 | Multichannel | surround_5_1 | off | ALLOW | Multichannel | Discrete 5.1 routing |
| SOM-028-06 | Multichannel | surround_7_1 | off | ALLOW | Multichannel | Discrete 7.1 routing |
| SOM-028-07 | Multichannel | immersive_7_4_2 | off | ALLOW | Multichannel | Discrete 7.4.2 routing |
| SOM-028-08 | Multichannel | stereo_2_0 | any | BLOCK | fallback | `multichannel_requires_min_4ch` |
| SOM-028-09 | Multichannel | any >= 4ch | on | BLOCK | fallback | `headtracking_not_supported_in_multichannel` |
| SOM-028-10 | ExternalSpatial | any >= 4ch | os-managed | ALLOW | ExternalSpatial | External bed pass-through (internal binaural disabled) |
| SOM-028-11 | ExternalSpatial | stereo_2_0 | os-managed | BLOCK | fallback | `external_spatial_requires_multichannel_bed` |

`BL028-A1-001` is satisfied only when every matrix row is encoded in evaluator logic and validated by deterministic scenarios.

## Mismatch Handling Contract

### Evaluation Trigger
Matrix evaluation must run on requested-domain/layout/tracking/profile transitions (message-thread control path), never in `processBlock()`.

### Fallback Mode Precedence
1. `retain_last_legal`: keep previous legal requested->active state when mismatch occurs.
2. `derive_from_host_layout`: used only when no legal previous state exists.
3. `safe_stereo_passthrough`: terminal fail-safe if no legal domain/layout state can be derived.

### Fail-Safe Routing Behavior
- `retain_last_legal`: keep last legal active routing unchanged.
- `derive_from_host_layout`:
  - `stereo_2_0` => `InternalBinaural` with head tracking disabled.
  - `quad_4_0` / `5.1` / `7.1` / `7.4.2` => `Multichannel` with head tracking disabled.
- `safe_stereo_passthrough`:
  - route canonical stereo monitor channels to output channels 1/2;
  - mute channels >2;
  - publish hard warning diagnostics.

No silent route mutation is permitted.

`BL028-A1-002` requires fallback selection and fail-safe routing to be deterministic for identical requested state and bus layout.

## Diagnostics Contract

Required diagnostics fields in scene-state payload:

| Field | Type | Allowed Values |
|---|---|---|
| `rendererMatrixRequestedDomain` | string | `InternalBinaural`, `Multichannel`, `ExternalSpatial` |
| `rendererMatrixActiveDomain` | string | same enum |
| `rendererMatrixRequestedLayout` | string | `stereo_2_0`, `quad_4_0`, `surround_5_1`, `surround_7_1`, `immersive_7_4_2` |
| `rendererMatrixActiveLayout` | string | same layout set |
| `rendererMatrixRuleId` | string | `SOM-028-01`..`SOM-028-11` |
| `rendererMatrixRuleState` | string | `allowed`, `blocked` |
| `rendererMatrixReasonCode` | string | see reason map |
| `rendererMatrixFallbackMode` | string | `none`, `retain_last_legal`, `derive_from_host_layout`, `safe_stereo_passthrough` |
| `rendererMatrixFailSafeRoute` | string | `none`, `last_legal`, `layout_derived`, `stereo_passthrough` |
| `rendererMatrixStatusText` | string | mapped user-visible text |
| `rendererMatrixEventSeq` | uint64 | monotonic transition sequence |

`BL028-A1-003` requires all fields to be present and finite for every transition event.

## Status Text Mapping (User-Visible)

| Reason Code | UI Status Text |
|---|---|
| `ok` | `Spatial output matrix valid.` |
| `binaural_requires_stereo` | `Binaural requires stereo output. Previous legal routing retained.` |
| `multichannel_requires_min_4ch` | `Multichannel requires at least 4 output channels.` |
| `headtracking_not_supported_in_multichannel` | `Head tracking is available only in internal binaural mode.` |
| `external_spatial_requires_multichannel_bed` | `External spatial mode requires a multichannel bed.` |
| `fallback_derived_from_layout` | `No legal prior state; routing derived from current host layout.` |
| `fallback_safe_stereo_passthrough` | `Fail-safe stereo passthrough active; review output configuration.` |

`BL028-A1-004` requires one-to-one mapping from reason code to status text with no ambiguous fallback strings.

## QA Contract Link
Deterministic scenario coverage, artifact schema, and pass/fail thresholds are defined in:
- `Documentation/testing/bl-028-spatial-output-matrix-qa.md`

`BL028-A1-005` is satisfied only when that lane schema validates all matrix + fallback + diagnostics assertions.

## Cross-Reference Requirement
Acceptance IDs `BL028-A1-001..006` must appear unchanged in:
1. `Documentation/backlog/done/bl-028-spatial-output-matrix.md`
2. `Documentation/plans/bl-028-spatial-output-matrix-spec-2026-02-25.md`
3. `Documentation/testing/bl-028-spatial-output-matrix-qa.md`

`BL028-A1-006` fails if any ID mapping diverges.
