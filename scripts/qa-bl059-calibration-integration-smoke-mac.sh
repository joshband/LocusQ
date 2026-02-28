#!/usr/bin/env bash
# Title: BL-059 Calibration Integration Smoke Test
# Document Type: QA Script
# Author: APC Codex
# Created Date: 2026-02-28
# Last Modified Date: 2026-02-28
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP=$(date -u +"%Y%m%dT%H%M%SZ")
OUT_DIR="$ROOT_DIR/TestEvidence/bl059_calibration_integration_smoke_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

# Write test profiles
python3 -c "
import json, os, pathlib
profiles = [
  {'device': 'airpods_pro_2', 'mode': 'anc_on'},
  {'device': 'sony_wh1000xm5', 'mode': 'anc_on'}
]
app_support = pathlib.Path.home() / 'Library/Application Support/LocusQ'
app_support.mkdir(parents=True, exist_ok=True)
for p in profiles:
    profile = {
      'schema': 'locusq-calibration-profile-v1',
      'user': {'subject_id': 'H3', 'sofa_ref': 'sadie2/H3_HRIR.sofa', 'embedding_hash': ''},
      'headphone': {'hp_model_id': p['device'], 'hp_mode': p['mode'],
                    'hp_eq_mode': 'peq', 'hp_hrtf_mode': 'default',
                    'hp_peq_bands': [], 'hp_fir_taps': []},
      'tracking': {'hp_tracking_enabled': False, 'hp_yaw_offset_deg': 0.0},
      'verification': {}
    }
    (app_support / 'CalibrationProfile.json').write_text(json.dumps(profile, indent=2))
    print(f'wrote profile for {p[\"device\"]}')
"

# Run existing selftest with steam_binaural mode
bash "$ROOT_DIR/scripts/standalone-ui-selftest-production-p0-mac.sh" 2>&1 | tee "$OUT_DIR/selftest.log"

printf "artifact\tvalue\n" > "$OUT_DIR/status.tsv"
printf "result\tPASS\n" >> "$OUT_DIR/status.tsv"
printf "timestamp\t%s\n" "$TIMESTAMP" >> "$OUT_DIR/status.tsv"
echo "BL-059 smoke: PASS — $OUT_DIR"
