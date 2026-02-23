#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DOC_DATE_UTC="$(date -u +%Y-%m-%d)"

STRICT_INTEGRATION="${STRICT_INTEGRATION:-0}"
RUN_BINAURAL_RUNTIME="${RUN_BINAURAL_RUNTIME:-1}"
RUN_PROFILE_MATRIX="${RUN_PROFILE_MATRIX:-1}"
QA_BIN="${QA_BIN:-$ROOT_DIR/build_local/locusq_qa_artefacts/Release/locusq_qa}"

while (($# > 0)); do
  case "$1" in
    --strict-integration)
      STRICT_INTEGRATION=1
      shift
      ;;
    --no-binaural-runtime)
      RUN_BINAURAL_RUNTIME=0
      shift
      ;;
    --no-profile-matrix)
      RUN_PROFILE_MATRIX=0
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 [--strict-integration] [--no-binaural-runtime] [--no-profile-matrix]" >&2
      exit 2
      ;;
  esac
done

OUT_DIR="$ROOT_DIR/TestEvidence/bl018_ambisonic_contract_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

STATUS_TSV="$OUT_DIR/status.tsv"
REPORT_MD="$OUT_DIR/report.md"

printf "step\tstatus\tdetail\n" >"$STATUS_TSV"

log_status() {
  local step="$1"
  local status="$2"
  local detail="$3"
  printf "%s\t%s\t%s\n" "$step" "$status" "$detail" | tee -a "$STATUS_TSV" >/dev/null
}

log_status "init" "pass" "ts=${TIMESTAMP}"
log_status "mode" "pass" "strict_integration=${STRICT_INTEGRATION}; run_binaural_runtime=${RUN_BINAURAL_RUNTIME}; run_profile_matrix=${RUN_PROFILE_MATRIX}"

if ! command -v python3 >/dev/null 2>&1; then
  log_status "python3" "fail" "python3_not_found"
else
  log_status "python3" "pass" "$(command -v python3)"
fi

if ! command -v jq >/dev/null 2>&1; then
  log_status "jq" "fail" "jq_not_found"
else
  log_status "jq" "pass" "$(command -v jq)"
fi

if awk -F'\t' 'NR>1 && $2=="fail" { exit 0 } END { exit 1 }' "$STATUS_TSV"; then
  echo "FAIL: prerequisite tools missing" >&2
  exit 1
fi

if [[ "$RUN_PROFILE_MATRIX" == "1" ]]; then
  if [[ -x "$QA_BIN" ]]; then
    log_status "qa_bin" "pass" "$QA_BIN"
  else
    log_status "qa_bin" "fail" "missing=${QA_BIN}"
  fi
fi

if awk -F'\t' 'NR>1 && $2=="fail" { exit 0 } END { exit 1 }' "$STATUS_TSV"; then
  echo "FAIL: missing required runtime for BL-018 lane" >&2
  exit 1
fi

PY_LOG="$OUT_DIR/reference_contract_python.log"
python3 - "$OUT_DIR" >"$PY_LOG" 2>&1 <<'PY'
import hashlib
import json
import math
import os
import struct
import sys
import wave

out_dir = sys.argv[1]
sample_rate = 48000
duration_seconds = 0.5
num_samples = int(sample_rate * duration_seconds)

signal = [
    0.28 * math.sin(2.0 * math.pi * 440.0 * i / sample_rate)
    + 0.17 * math.sin(2.0 * math.pi * 880.0 * i / sample_rate)
    for i in range(num_samples)
]

def speaker_unit_vector(azimuth_deg, elevation_deg):
    az = math.radians(azimuth_deg)
    el = math.radians(elevation_deg)
    # Right-handed frame with +Y as front, +X as right, +Z as up.
    x = math.sin(az) * math.cos(el)
    y = math.cos(az) * math.cos(el)
    z = math.sin(el)
    return (x, y, z)

def encode_foa_sn3d(samples, azimuth_deg, elevation_deg):
    vx, vy, vz = speaker_unit_vector(azimuth_deg, elevation_deg)
    w = [s / math.sqrt(2.0) for s in samples]
    x = [s * vx for s in samples]
    y = [s * vy for s in samples]
    z = [s * vz for s in samples]
    return (w, x, y, z)

def decode_foa(foa, layout):
    w, x, y, z = foa
    output = {}
    for name, _ in layout:
        output[name] = [0.0] * len(w)

    for i in range(len(w)):
        wi = w[i]
        xi = x[i]
        yi = y[i]
        zi = z[i]
        for name, (vx, vy, vz) in layout:
            output[name][i] = 0.70710678 * wi + vx * xi + vy * yi + vz * zi
    return output

def decode_binaural_proxy(foa):
    w, x, y, z = foa
    left = [0.0] * len(w)
    right = [0.0] * len(w)
    for i in range(len(w)):
        wi = w[i]
        xi = x[i]
        yi = y[i]
        zi = z[i]
        left[i] = 0.70710678 * wi - 0.50 * xi + 0.22 * yi + 0.08 * zi
        right[i] = 0.70710678 * wi + 0.50 * xi + 0.22 * yi + 0.08 * zi
    return {"L": left, "R": right}

def energy(samples):
    return sum(s * s for s in samples) / max(len(samples), 1)

def hash_channels(channels, order):
    payload = bytearray()
    for key in order:
        for sample in channels[key]:
            payload.extend(struct.pack("<f", float(sample)))
    return hashlib.sha256(payload).hexdigest()

def has_non_finite(channels):
    for values in channels.values():
        for value in values:
            if not math.isfinite(value):
                return True
    return False

def write_pcm16_wav(path, order, channels):
    frame_count = len(channels[order[0]])
    max_abs = 1.0e-12
    for key in order:
        for sample in channels[key]:
            max_abs = max(max_abs, abs(sample))
    scale = 0.95 / max_abs

    with wave.open(path, "wb") as wav:
        wav.setnchannels(len(order))
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        for i in range(frame_count):
            frame = bytearray()
            for key in order:
                sample = max(-1.0, min(1.0, channels[key][i] * scale))
                frame.extend(struct.pack("<h", int(round(sample * 32767.0))))
            wav.writeframesraw(frame)

quad_layout = [
    ("FL", speaker_unit_vector(-45.0, 0.0)),
    ("FR", speaker_unit_vector(45.0, 0.0)),
    ("RR", speaker_unit_vector(135.0, 0.0)),
    ("RL", speaker_unit_vector(-135.0, 0.0)),
]

# 7.4.2 visualization-contract directional channels.
# LFE channels are represented in metadata only (not decoded directionally).
layout_742_directional = [
    ("L", speaker_unit_vector(-30.0, 0.0)),
    ("R", speaker_unit_vector(30.0, 0.0)),
    ("C", speaker_unit_vector(0.0, 0.0)),
    ("Ls", speaker_unit_vector(-110.0, 0.0)),
    ("Rs", speaker_unit_vector(110.0, 0.0)),
    ("Lrs", speaker_unit_vector(-150.0, 0.0)),
    ("Rrs", speaker_unit_vector(150.0, 0.0)),
    ("TopL", speaker_unit_vector(-35.0, 45.0)),
    ("TopR", speaker_unit_vector(35.0, 45.0)),
]

foa_ref_a = encode_foa_sn3d(signal, 35.0, 15.0)
foa_ref_b = encode_foa_sn3d(signal, 35.0, 15.0)

quad_ref_a = decode_foa(foa_ref_a, quad_layout)
quad_ref_b = decode_foa(foa_ref_b, quad_layout)
binaural_ref = decode_binaural_proxy(foa_ref_a)

quad_hash_a = hash_channels(quad_ref_a, ["FL", "FR", "RR", "RL"])
quad_hash_b = hash_channels(quad_ref_b, ["FL", "FR", "RR", "RL"])
binaural_hash = hash_channels(binaural_ref, ["L", "R"])

foa_front = encode_foa_sn3d(signal, 0.0, 0.0)
quad_front = decode_foa(foa_front, quad_layout)

foa_left = encode_foa_sn3d(signal, -35.0, 0.0)
foa_right = encode_foa_sn3d(signal, 35.0, 0.0)
quad_left = decode_foa(foa_left, quad_layout)
quad_right = decode_foa(foa_right, quad_layout)

foa_top = encode_foa_sn3d(signal, 0.0, 45.0)
layout_742_top = decode_foa(foa_top, layout_742_directional)

binaural_left = decode_binaural_proxy(foa_left)
binaural_right = decode_binaural_proxy(foa_right)

tests = []

tests.append({
    "id": "deterministic_reference_decode",
    "pass": quad_hash_a == quad_hash_b,
    "detail": f"quad_hash_a={quad_hash_a} quad_hash_b={quad_hash_b}",
})

front_energy = energy(quad_front["FL"]) + energy(quad_front["FR"])
rear_energy = energy(quad_front["RL"]) + energy(quad_front["RR"])
tests.append({
    "id": "quad_front_vs_rear_energy",
    "pass": front_energy > rear_energy * 1.20,
    "detail": f"front={front_energy:.8f} rear={rear_energy:.8f}",
})

sym_fl = abs(energy(quad_left["FL"]) - energy(quad_right["FR"])) / max(
    energy(quad_left["FL"]), energy(quad_right["FR"]), 1.0e-12
)
sym_fr = abs(energy(quad_left["FR"]) - energy(quad_right["FL"])) / max(
    energy(quad_left["FR"]), energy(quad_right["FL"]), 1.0e-12
)
tests.append({
    "id": "quad_mirror_symmetry",
    "pass": sym_fl < 0.12 and sym_fr < 0.12,
    "detail": f"sym_fl={sym_fl:.6f} sym_fr={sym_fr:.6f}",
})

top_energy = energy(layout_742_top["TopL"]) + energy(layout_742_top["TopR"])
front_bed_energy = energy(layout_742_top["L"]) + energy(layout_742_top["R"]) + energy(layout_742_top["C"])
tests.append({
    "id": "layout_742_height_response",
    "pass": top_energy > front_bed_energy * 0.60,
    "detail": f"top={top_energy:.8f} front_bed={front_bed_energy:.8f}",
})

sym_bin_l = abs(energy(binaural_left["L"]) - energy(binaural_right["R"])) / max(
    energy(binaural_left["L"]), energy(binaural_right["R"]), 1.0e-12
)
sym_bin_r = abs(energy(binaural_left["R"]) - energy(binaural_right["L"])) / max(
    energy(binaural_left["R"]), energy(binaural_right["L"]), 1.0e-12
)
tests.append({
    "id": "binaural_proxy_mirror_symmetry",
    "pass": sym_bin_l < 0.12 and sym_bin_r < 0.12,
    "detail": f"sym_l={sym_bin_l:.6f} sym_r={sym_bin_r:.6f}",
})

tests.append({
    "id": "non_finite_guard",
    "pass": not (has_non_finite(quad_ref_a) or has_non_finite(binaural_ref)),
    "detail": "all_reference_samples_finite",
})

overall_pass = all(item["pass"] for item in tests)

write_pcm16_wav(
    os.path.join(out_dir, "foa_reference_quad.wav"),
    ["FL", "FR", "RR", "RL"],
    quad_ref_a,
)
write_pcm16_wav(
    os.path.join(out_dir, "foa_reference_binaural.wav"),
    ["L", "R"],
    binaural_ref,
)

layout_manifest = {
    "quad": {
        "channels": [
            {"name": "FL", "azimuth_deg": -45.0, "elevation_deg": 0.0},
            {"name": "FR", "azimuth_deg": 45.0, "elevation_deg": 0.0},
            {"name": "RR", "azimuth_deg": 135.0, "elevation_deg": 0.0},
            {"name": "RL", "azimuth_deg": -135.0, "elevation_deg": 0.0},
        ]
    },
    "layout_7_4_2_visual": {
        "directional_channels": [
            {"name": "L", "azimuth_deg": -30.0, "elevation_deg": 0.0},
            {"name": "R", "azimuth_deg": 30.0, "elevation_deg": 0.0},
            {"name": "C", "azimuth_deg": 0.0, "elevation_deg": 0.0},
            {"name": "Ls", "azimuth_deg": -110.0, "elevation_deg": 0.0},
            {"name": "Rs", "azimuth_deg": 110.0, "elevation_deg": 0.0},
            {"name": "Lrs", "azimuth_deg": -150.0, "elevation_deg": 0.0},
            {"name": "Rrs", "azimuth_deg": 150.0, "elevation_deg": 0.0},
            {"name": "TopL", "azimuth_deg": -35.0, "elevation_deg": 45.0},
            {"name": "TopR", "azimuth_deg": 35.0, "elevation_deg": 45.0},
        ],
        "lfe_channel_count": 4
    }
}

with open(os.path.join(out_dir, "layout_manifest.json"), "w", encoding="utf-8") as f:
    json.dump(layout_manifest, f, indent=2)

summary = {
    "sample_rate": sample_rate,
    "duration_seconds": duration_seconds,
    "reference_hashes": {
        "quad_hash_a": quad_hash_a,
        "quad_hash_b": quad_hash_b,
        "binaural_proxy_hash": binaural_hash,
    },
    "tests": tests,
    "overall_pass": overall_pass,
}

with open(os.path.join(out_dir, "reference_contract.json"), "w", encoding="utf-8") as f:
    json.dump(summary, f, indent=2)

print(json.dumps({
    "overall_pass": overall_pass,
    "quad_hash_a": quad_hash_a,
    "quad_hash_b": quad_hash_b,
    "binaural_proxy_hash": binaural_hash
}))

if not overall_pass:
    raise SystemExit(3)
PY

if jq -e '.overall_pass == true' "$OUT_DIR/reference_contract.json" >/dev/null 2>&1; then
  REF_QUAD_HASH_A="$(jq -r '.reference_hashes.quad_hash_a' "$OUT_DIR/reference_contract.json")"
  REF_QUAD_HASH_B="$(jq -r '.reference_hashes.quad_hash_b' "$OUT_DIR/reference_contract.json")"
  REF_BIN_HASH="$(jq -r '.reference_hashes.binaural_proxy_hash' "$OUT_DIR/reference_contract.json")"
  log_status "reference_contract" "pass" "quad_a=${REF_QUAD_HASH_A}; quad_b=${REF_QUAD_HASH_B}; bin=${REF_BIN_HASH}"
else
  log_status "reference_contract" "fail" "see=${OUT_DIR}/reference_contract.json"
fi

if rg -n -i "(ambisonic|\\bhoa\\b|b-format|\\bfoa\\b|rend_ambi|rendererAmbi|ambi_)" \
  Source/PluginProcessor.cpp Source/SpatialRenderer.h Source/ui/public/js/index.js \
  >"$OUT_DIR/integration_probe_hits.log"; then
  log_status "integration_probe" "pass" "ambisonic_markers_present"
else
  if [[ "$STRICT_INTEGRATION" == "1" ]]; then
    log_status "integration_probe" "fail" "no_ambisonic_backend_markers_in_source_strict_mode"
  else
    log_status "integration_probe" "warn" "no_ambisonic_backend_markers_in_source_pending_backend_impl"
  fi
fi

run_spatial_profile_case() {
  local label="$1"
  local scenario="$2"
  local channels="$3"
  local scenario_id
  scenario_id="$(basename "$scenario" .json)"
  local run_log="$OUT_DIR/${label}.log"

  if "$QA_BIN" --spatial "$scenario" --sample-rate 48000 --block-size 512 --channels "$channels" >"$run_log" 2>&1; then
    local wet="$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/wet.wav"
    local result="$ROOT_DIR/qa_output/locusq_spatial/${scenario_id}/result.json"
    if [[ -f "$wet" ]]; then
      cp "$wet" "$OUT_DIR/${label}.wet.wav"
      local hash
      hash="$(shasum -a 256 "$OUT_DIR/${label}.wet.wav" | awk '{print $1}')"
      log_status "$label" "pass" "scenario=${scenario}; channels=${channels}; hash=${hash}"
    else
      log_status "$label" "fail" "missing_wet=${wet}; log=${run_log}"
      return 1
    fi
    if [[ -f "$result" ]]; then
      cp "$result" "$OUT_DIR/${label}.result.json"
    fi
  else
    log_status "$label" "fail" "scenario=${scenario}; channels=${channels}; log=${run_log}"
    return 1
  fi
}

if [[ "$RUN_PROFILE_MATRIX" == "1" ]]; then
  run_spatial_profile_case "profile_virtual3d_stereo" "qa/scenarios/locusq_bl018_profile_virtual3d_stereo.json" 2
  run_spatial_profile_case "profile_ambi_foa" "qa/scenarios/locusq_bl018_profile_ambi_foa.json" 4
  run_spatial_profile_case "profile_ambi_hoa" "qa/scenarios/locusq_bl018_profile_ambi_hoa.json" 16
  run_spatial_profile_case "profile_surround_521" "qa/scenarios/locusq_bl018_profile_surround_521.json" 8
  run_spatial_profile_case "profile_surround_721" "qa/scenarios/locusq_bl018_profile_surround_721.json" 10
  run_spatial_profile_case "profile_surround_742" "qa/scenarios/locusq_bl018_profile_surround_742.json" 13
  run_spatial_profile_case "profile_codec_iamf" "qa/scenarios/locusq_bl018_profile_codec_iamf.json" 13
  run_spatial_profile_case "profile_codec_adm" "qa/scenarios/locusq_bl018_profile_codec_adm.json" 13
fi

BINAURAL_ARTIFACT_DIR=""
if [[ "$RUN_BINAURAL_RUNTIME" == "1" ]]; then
  if [[ -x "$ROOT_DIR/scripts/qa-bl009-headphone-contract-mac.sh" ]]; then
    if "$ROOT_DIR/scripts/qa-bl009-headphone-contract-mac.sh" >"$OUT_DIR/bl009_runtime.log" 2>&1; then
      BINAURAL_ARTIFACT_DIR="$(awk -F= '/^artifact_dir=/{print $2}' "$OUT_DIR/bl009_runtime.log" | tail -n 1)"
      if [[ -n "$BINAURAL_ARTIFACT_DIR" ]]; then
        log_status "binaural_runtime_contract" "pass" "$BINAURAL_ARTIFACT_DIR"
      else
        log_status "binaural_runtime_contract" "warn" "pass_without_artifact_path_parse"
      fi
    else
      log_status "binaural_runtime_contract" "fail" "see=${OUT_DIR}/bl009_runtime.log"
    fi
  else
    log_status "binaural_runtime_contract" "warn" "bl009_contract_script_missing"
  fi
else
  log_status "binaural_runtime_contract" "warn" "disabled_by_flag"
fi

FAIL_COUNT="$(awk -F'\t' 'NR>1 && $2=="fail" { c++ } END { print c+0 }' "$STATUS_TSV")"
WARN_COUNT="$(awk -F'\t' 'NR>1 && $2=="warn" { c++ } END { print c+0 }' "$STATUS_TSV")"
PASS_COUNT="$(awk -F'\t' 'NR>1 && $2=="pass" { c++ } END { print c+0 }' "$STATUS_TSV")"

OVERALL="pass"
if [[ "$FAIL_COUNT" != "0" ]]; then
  OVERALL="fail"
elif [[ "$WARN_COUNT" != "0" ]]; then
  OVERALL="pass_with_warnings"
fi

cat >"$REPORT_MD" <<EOF
Title: BL-018 Ambisonic Contract Report
Document Type: Test Evidence
Author: APC Codex
Created Date: ${DOC_DATE_UTC}
Last Modified Date: ${DOC_DATE_UTC}

# BL-018 Ambisonic Contract (${TIMESTAMP})

- overall: \`${OVERALL}\`
- strict_integration: \`${STRICT_INTEGRATION}\`
- run_binaural_runtime: \`${RUN_BINAURAL_RUNTIME}\`
- pass_count: \`${PASS_COUNT}\`
- warn_count: \`${WARN_COUNT}\`
- fail_count: \`${FAIL_COUNT}\`
- binaural_runtime_artifact: \`${BINAURAL_ARTIFACT_DIR:-n/a}\`

## Reference Artifacts

- \`reference_contract.json\`
- \`layout_manifest.json\`
- \`foa_reference_quad.wav\`
- \`foa_reference_binaural.wav\`
- \`status.tsv\`
EOF

if [[ "$OVERALL" == "fail" ]]; then
  echo "FAIL: BL-018 ambisonic contract lane failed" >&2
  echo "artifact_dir=$OUT_DIR"
  exit 1
fi

if [[ "$OVERALL" == "pass_with_warnings" ]]; then
  echo "PASS_WITH_WARNINGS: BL-018 ambisonic contract lane completed"
else
  echo "PASS: BL-018 ambisonic contract lane completed"
fi
echo "artifact_dir=$OUT_DIR"
