Title: LocusQ CalibrationProfile JSON Schema v1
Document Type: Schema Reference
Author: APC Codex
Created Date: 2026-02-27
Last Modified Date: 2026-02-27

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
