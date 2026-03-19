Title: LocusQ CalibrationProfile JSON Schema v1
Document Type: Schema Reference
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-03-18

# CalibrationProfile JSON Schema v1

Schema key: `locusq-calibration-profile-v1`

Written by: companion app
Read by: LocusQ plugin (C++) and companion app (Swift)
Location: `~/Library/Application Support/LocusQ/CalibrationProfile.json`

## Top-level structure

| Field | Type | Description |
|-------|------|-------------|
| `schema` | string | Always `"locusq-calibration-profile-v1"` |
| `user` | object | Subject/HRTF selection |
| `headphone` | object | Headphone model and EQ config |
| `tracking` | object | Head-tracking settings |
| `verification` | object | Listening test scores (optional fields) |
| `provenance` | object? | Optional additive BL-101 metadata describing profile source, evidence strength, and freshness expectations |

## `user` object

| Field | Type | Description |
|-------|------|-------------|
| `subject_id` | string | SADIE II subject ID (e.g. `"H3"`) |
| `sofa_ref` | string | Relative path to SOFA file (e.g. `"sadie2/H3_HRIR.sofa"`) |
| `embedding_hash` | string | SHA-256 of ear photo embedding, or empty string |

## `headphone` object

| Field | Type | Description |
|-------|------|-------------|
| `hp_model_id` | string | Device ID: `"generic"`, `"airpods_pro_1"`, `"airpods_pro_2"`, `"airpods_pro_3"`, `"sony_wh1000xm5"`, `"custom_sofa"` |
| `hp_mode` | string | ANC mode: `"anc_on"`, `"anc_off"`, `"default"` |
| `hp_eq_mode` | string | EQ engine: `"off"`, `"peq"`, `"fir"` |
| `hp_hrtf_mode` | string | HRTF source: `"default"`, `"sofa"` |
| `hp_peq_bands` | array of objects | PEQ bands (empty if eq_mode != "peq"). Each: `{"type":"PK"\|"LSC"\|"HSC", "fc_hz":float, "gain_db":float, "q":float}` |
| `hp_fir_taps` | array of floats | FIR coefficients (empty if eq_mode != "fir") |

## `tracking` object

| Field | Type | Description |
|-------|------|-------------|
| `hp_tracking_enabled` | bool | Whether head-tracking is enabled |
| `hp_yaw_offset_deg` | float | Manual yaw offset in degrees |

## `verification` object (all fields optional)

| Field | Type | Description |
|-------|------|-------------|
| `externalization_score` | float? | 0.0-1.0, from Phase B listening test |
| `front_back_confusion_rate` | float? | 0.0-1.0 (lower is better) |
| `localization_accuracy` | float? | 0.0-1.0 |
| `preference_score` | float? | 0.0-1.0 participant preference aggregate |

## `provenance` object (optional additive BL-101 extension)

This object is optional and additive. Consumers must preserve or ignore unknown fields safely.

| Field | Type | Description |
|-------|------|-------------|
| `profile_source` | string | How this profile entered the system: `"bundled_measured"`, `"companion_estimated"`, `"user_imported"`, `"manual_authored"`, `"runtime_generated"`, `"unknown"` |
| `subject_provenance` | string | Evidence strength for the `user` section: `"measured"`, `"detected"`, `"inferred"`, `"estimated"`, `"generic"`, `"unavailable"` |
| `headphone_provenance` | string | Evidence strength for the `headphone` section: same enum domain as `subject_provenance` |
| `verification_provenance` | string | Evidence strength for the `verification` section: same enum domain as `subject_provenance` |
| `generated_at_utc_ms` | integer? | UTC milliseconds when the profile or provenance block was generated |
| `updated_at_utc_ms` | integer? | UTC milliseconds when the profile or provenance block was last updated |
| `stale_after_ms` | integer? | Optional freshness budget for external/device-derived metadata before UI should warn that it may be stale |
| `manual_overrides` | array of strings? | Optional list of profile areas manually overridden after auto/imported population, e.g. `["headphone.hp_eq_mode"]` |

Rules:

1. `profile_source` must not be interpreted as evidence strength. Use the `*_provenance` fields for that.
2. Missing `provenance` object means no BL-101 guarantees are available; consumers must treat provenance as unknown/unavailable rather than measured.
3. Saved/exported profiles should preserve `provenance` fields when available.
4. Imported profiles that omit provenance must not be upgraded into measured truth by consumers.

## Example

```json
{
  "schema": "locusq-calibration-profile-v1",
  "user": {
    "subject_id": "H3",
    "sofa_ref": "sadie2/H3_HRIR.sofa",
    "embedding_hash": ""
  },
  "headphone": {
    "hp_model_id": "airpods_pro_2",
    "hp_mode": "anc_on",
    "hp_eq_mode": "peq",
    "hp_hrtf_mode": "default",
    "hp_peq_bands": [
      {"type": "PK", "fc_hz": 3000.0, "gain_db": -2.5, "q": 1.2}
    ],
    "hp_fir_taps": []
  },
  "tracking": {
    "hp_tracking_enabled": true,
    "hp_yaw_offset_deg": 0.0
  },
  "verification": {
    "externalization_score": 0.74,
    "preference_score": 0.81
  }
}
```
