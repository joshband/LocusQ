#!/usr/bin/env bash
set -euo pipefail

if [[ "${OSTYPE:-}" != darwin* ]]; then
  echo "ERROR: scripts/capture-headtracking-rotation-mac.sh is macOS-only." >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATE_UTC="$(date -u +%Y-%m-%d)"

OUT_DIR="${ROOT_DIR}/TestEvidence/headtracking_rotation_capture_${TIMESTAMP}"
DURATION_SEC=40
FPS=30
EXTRACT_EVERY_SEC=0.5
COUNTDOWN_SEC=5
VIDEO_DEVICE=""
NO_EXTRACT=0
NO_EXTRACT_SET=0
OPEN_OUTPUT=0
NO_CUES=0
CUE_SPEECH=0
CUE_PROFILE="coarse"
CUE_PROFILE_SET=0
PROFILE_NAME=""
PROFILE_FILE=""
PROFILE_SOURCE="builtin:coarse"
PROFILE_SESSION_NAME=""
PROFILE_SCHEMA="locusq-capture-profile-v1"
PROFILE_CONTRACT_HASH=""
PROFILE_EXTRACT_FRAMES=1
DRY_RUN=0

DURATION_SET=0
FPS_SET=0
EXTRACT_SET=0
COUNTDOWN_SET=0

CUE_TIMES_SEC=()
CUE_MESSAGES=()

set_cue_profile() {
  case "$1" in
    coarse)
      CUE_TIMES_SEC=(0 12 24 36 48)
      CUE_MESSAGES=(
        "Center and sync"
        "Rotate clockwise to 90 degrees"
        "Rotate clockwise to 180 degrees"
        "Rotate clockwise to 225 degrees"
        "Rotate clockwise to 270 degrees"
      )
      PROFILE_SESSION_NAME="headtracking_coarse"
      PROFILE_SOURCE="builtin:coarse"
      ;;
    dense)
      CUE_TIMES_SEC=(0 8 16 24 32 40 48)
      CUE_MESSAGES=(
        "Center and sync"
        "Rotate clockwise to 45 degrees"
        "Rotate clockwise to 90 degrees"
        "Rotate clockwise to 135 degrees"
        "Rotate clockwise to 180 degrees"
        "Rotate clockwise to 225 degrees"
        "Rotate clockwise to 270 degrees"
      )
      PROFILE_SESSION_NAME="headtracking_dense"
      PROFILE_SOURCE="builtin:dense"
      ;;
    *)
      echo "ERROR: Invalid cue profile '$1'. Use coarse or dense." >&2
      exit 1
      ;;
  esac
}

usage() {
  cat <<'USAGE'
Usage: scripts/capture-headtracking-rotation-mac.sh [options]

Records the macOS screen while you rotate through head-tracking checkpoints,
then extracts frames for easier review.

Options:
  --out-dir <path>            Output directory (default: TestEvidence timestamp dir)
  --duration <seconds>        Recording duration in seconds (default: 40)
  --fps <fps>                 Recording frame rate (default: 30)
  --extract-every <seconds>   Extract one frame every N seconds (default: 0.5)
  --countdown <seconds>       Countdown before recording starts (default: 5)
  --device <id-or-name>       AVFoundation video device (default: auto-detect screen)
  --no-extract                Do not extract still frames
  --no-cues                   Disable timed rotation cues during recording
  --cue-profile <name>        Cue schedule: coarse or dense (default: coarse)
  --profile <name>            Load profile from scripts/capture_profiles/<name>.json
  --profile-file <path>       Load profile from explicit JSON file path
  --cue-speech                Speak cues using macOS 'say' command
  --dry-run                   Skip ffmpeg recording/extraction and emit contract artifacts only
  --open-output               Open output folder when complete
  --help                      Show this message

Examples:
  ./scripts/capture-headtracking-rotation-mac.sh
  ./scripts/capture-headtracking-rotation-mac.sh --profile dense
  ./scripts/capture-headtracking-rotation-mac.sh --profile-file scripts/capture_profiles/dense.json --dry-run
  ./scripts/capture-headtracking-rotation-mac.sh --duration 60 --extract-every 0.25
USAGE
}

hash_string_sha256() {
  local value="$1"
  printf '%s' "$value" | shasum -a 256 | awk '{print $1}'
}

hash_file_sha256() {
  local path="$1"
  if [[ -f "$path" ]]; then
    shasum -a 256 "$path" | awk '{print $1}'
  else
    echo "-"
  fi
}

file_size_bytes() {
  local path="$1"
  if [[ -f "$path" ]]; then
    stat -f '%z' "$path"
  else
    echo 0
  fi
}

load_profile_from_json() {
  local profile_path="$1"

  if [[ ! -f "$profile_path" ]]; then
    echo "ERROR: profile file not found: $profile_path" >&2
    exit 1
  fi

  if ! command -v python3 >/dev/null 2>&1; then
    echo "ERROR: python3 is required to parse profile JSON files." >&2
    exit 1
  fi

  local parsed
  parsed="$(python3 - "$profile_path" <<'PY'
import hashlib
import json
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])

with path.open("r", encoding="utf-8") as handle:
    data = json.load(handle)

required = [
    "schema",
    "session_name",
    "duration_sec",
    "fps",
    "extract_every_sec",
    "countdown_sec",
    "cue_points",
    "artifact_pack",
]
missing = [k for k in required if k not in data]
if missing:
    raise SystemExit(f"missing required keys: {', '.join(missing)}")

if data["schema"] != "locusq-capture-profile-v1":
    raise SystemExit("unsupported schema (expected locusq-capture-profile-v1)")

session_name = data["session_name"]
if not isinstance(session_name, str) or not re.fullmatch(r"[A-Za-z0-9._-]+", session_name):
    raise SystemExit("session_name must match [A-Za-z0-9._-]+")

for numeric_key in ["duration_sec", "fps", "extract_every_sec", "countdown_sec"]:
    value = data[numeric_key]
    if not isinstance(value, (int, float)):
      raise SystemExit(f"{numeric_key} must be numeric")
    if numeric_key in ("duration_sec", "fps", "extract_every_sec") and value <= 0:
      raise SystemExit(f"{numeric_key} must be > 0")
    if numeric_key == "countdown_sec" and value < 0:
      raise SystemExit("countdown_sec must be >= 0")

cue_points = data["cue_points"]
if not isinstance(cue_points, list) or len(cue_points) == 0:
    raise SystemExit("cue_points must be a non-empty list")

last_t = -1.0
for idx, point in enumerate(cue_points):
    if not isinstance(point, dict):
        raise SystemExit(f"cue_points[{idx}] must be an object")
    if "t" not in point or "label" not in point:
        raise SystemExit(f"cue_points[{idx}] must contain t and label")
    t_val = point["t"]
    label = point["label"]
    if not isinstance(t_val, (int, float)):
        raise SystemExit(f"cue_points[{idx}].t must be numeric")
    if t_val < 0:
        raise SystemExit(f"cue_points[{idx}].t must be >= 0")
    if float(t_val) <= float(last_t):
        raise SystemExit("cue_points must be strictly increasing by t")
    if not isinstance(label, str) or len(label.strip()) == 0:
        raise SystemExit(f"cue_points[{idx}].label must be a non-empty string")
    if "\t" in label or "\n" in label or "\r" in label:
        raise SystemExit(f"cue_points[{idx}].label must not contain tab/newline")
    last_t = float(t_val)

artifact_pack = data["artifact_pack"]
if not isinstance(artifact_pack, dict):
    raise SystemExit("artifact_pack must be an object")
extract_frames = artifact_pack.get("extract_frames", True)
if not isinstance(extract_frames, bool):
    raise SystemExit("artifact_pack.extract_frames must be boolean when provided")

canonical = json.dumps(data, sort_keys=True, separators=(",", ":"))
contract_hash = hashlib.sha256(canonical.encode("utf-8")).hexdigest()

print(f"SESSION_NAME\t{session_name}")
print(f"SCHEMA\t{data['schema']}")
print(f"DURATION_SEC\t{data['duration_sec']}")
print(f"FPS\t{data['fps']}")
print(f"EXTRACT_EVERY_SEC\t{data['extract_every_sec']}")
print(f"COUNTDOWN_SEC\t{data['countdown_sec']}")
print(f"EXTRACT_FRAMES\t{1 if extract_frames else 0}")
print(f"PROFILE_CONTRACT_HASH\t{contract_hash}")

for point in cue_points:
    print(f"CUE\t{point['t']}\t{point['label']}")
PY
)"

  CUE_TIMES_SEC=()
  CUE_MESSAGES=()

  while IFS=$'\t' read -r kind a b; do
    case "$kind" in
      SESSION_NAME)
        PROFILE_SESSION_NAME="$a"
        ;;
      SCHEMA)
        PROFILE_SCHEMA="$a"
        ;;
      DURATION_SEC)
        DURATION_SEC="$a"
        ;;
      FPS)
        FPS="$a"
        ;;
      EXTRACT_EVERY_SEC)
        EXTRACT_EVERY_SEC="$a"
        ;;
      COUNTDOWN_SEC)
        COUNTDOWN_SEC="$a"
        ;;
      EXTRACT_FRAMES)
        PROFILE_EXTRACT_FRAMES="$a"
        ;;
      PROFILE_CONTRACT_HASH)
        PROFILE_CONTRACT_HASH="$a"
        ;;
      CUE)
        CUE_TIMES_SEC+=("$a")
        CUE_MESSAGES+=("$b")
        ;;
      *)
        ;;
    esac
  done <<< "$parsed"

  local base_name
  base_name="$(basename "$profile_path")"
  PROFILE_SOURCE="profile_file:${base_name}"
}

append_artifact_row() {
  local artifact_id="$1"
  local rel_path="$2"
  local required="$3"
  local notes="$4"

  local abs_path="${OUT_DIR}/${rel_path}"
  local present="no"
  local size_bytes=0
  local sha256="-"

  if [[ -f "$abs_path" ]]; then
    present="yes"
    size_bytes="$(file_size_bytes "$abs_path")"
    sha256="$(hash_file_sha256 "$abs_path")"
  elif [[ -d "$abs_path" ]]; then
    present="yes"
    size_bytes="$(find "$abs_path" -type f | wc -l | tr -d '[:space:]')"
    sha256="-"
  fi

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$artifact_id" \
    "$rel_path" \
    "$required" \
    "$present" \
    "$size_bytes" \
    "$sha256" \
    "${notes//$'\t'/ }" \
    >> "$ARTIFACT_SCHEMA_TSV"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --duration)
      DURATION_SEC="$2"
      DURATION_SET=1
      shift 2
      ;;
    --fps)
      FPS="$2"
      FPS_SET=1
      shift 2
      ;;
    --extract-every)
      EXTRACT_EVERY_SEC="$2"
      EXTRACT_SET=1
      shift 2
      ;;
    --countdown)
      COUNTDOWN_SEC="$2"
      COUNTDOWN_SET=1
      shift 2
      ;;
    --device)
      VIDEO_DEVICE="$2"
      shift 2
      ;;
    --no-extract)
      NO_EXTRACT=1
      NO_EXTRACT_SET=1
      shift
      ;;
    --no-cues)
      NO_CUES=1
      shift
      ;;
    --cue-profile)
      CUE_PROFILE="$2"
      CUE_PROFILE_SET=1
      shift 2
      ;;
    --profile)
      PROFILE_NAME="$2"
      shift 2
      ;;
    --profile-file)
      PROFILE_FILE="$2"
      shift 2
      ;;
    --cue-speech)
      CUE_SPEECH=1
      shift
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    --open-output)
      OPEN_OUTPUT=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -n "$PROFILE_NAME" && -n "$PROFILE_FILE" ]]; then
  echo "ERROR: Use either --profile or --profile-file, not both." >&2
  exit 1
fi

if [[ -n "$PROFILE_NAME" ]]; then
  PROFILE_FILE="${ROOT_DIR}/scripts/capture_profiles/${PROFILE_NAME}.json"
fi

if [[ -n "$PROFILE_FILE" ]]; then
  if (( DURATION_SET == 1 || FPS_SET == 1 || EXTRACT_SET == 1 || COUNTDOWN_SET == 1 || CUE_PROFILE_SET == 1 )); then
    echo "ERROR: profile contract controls duration/fps/extract/countdown/cue schedule. Remove manual overrides when using --profile/--profile-file." >&2
    exit 1
  fi
  load_profile_from_json "$PROFILE_FILE"
else
  set_cue_profile "$CUE_PROFILE"
  local_profile_payload="schema=${PROFILE_SCHEMA};session=${PROFILE_SESSION_NAME};duration=${DURATION_SEC};fps=${FPS};extract=${EXTRACT_EVERY_SEC};countdown=${COUNTDOWN_SEC};cue_profile=${CUE_PROFILE}"
  for idx in "${!CUE_TIMES_SEC[@]}"; do
    local_profile_payload+="|${CUE_TIMES_SEC[$idx]}:${CUE_MESSAGES[$idx]}"
  done
  PROFILE_CONTRACT_HASH="$(hash_string_sha256 "$local_profile_payload")"
fi

if (( NO_EXTRACT_SET == 0 )) && [[ "$PROFILE_EXTRACT_FRAMES" == "0" ]]; then
  NO_EXTRACT=1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
  if [[ "$DRY_RUN" -eq 0 ]]; then
    echo "ERROR: ffmpeg is required but not found on PATH." >&2
    exit 1
  fi
fi

VIDEO_PATH="${OUT_DIR}/rotation_capture.mp4"
RECORD_LOG="${OUT_DIR}/ffmpeg_record.log"
FRAMES_DIR="${OUT_DIR}/frames"
EXTRACT_LOG="${OUT_DIR}/ffmpeg_extract.log"
SUMMARY_MD="${OUT_DIR}/capture_summary.md"
CHECKPOINTS_TSV="${OUT_DIR}/checkpoints.tsv"
CUE_EVENTS_TSV="${OUT_DIR}/cue_events.tsv"
SESSION_MANIFEST_JSON="${OUT_DIR}/session_manifest.json"
ARTIFACT_SCHEMA_TSV="${OUT_DIR}/artifact_schema_inventory.tsv"
REPLAY_HASHES_TSV="${OUT_DIR}/replay_hashes.tsv"
DRY_RUN_NOTE="${OUT_DIR}/dry_run_note.txt"

mkdir -p "$OUT_DIR"

printf "checkpoint_id\tscheduled_sec\tlabel\n" > "$CHECKPOINTS_TSV"
for idx in "${!CUE_TIMES_SEC[@]}"; do
  printf "cp_%02d\t%s\t%s\n" "$((idx + 1))" "${CUE_TIMES_SEC[$idx]}" "${CUE_MESSAGES[$idx]}" >> "$CHECKPOINTS_TSV"
done

printf "checkpoint_id\tscheduled_sec\tlabel\tevent\temitted_unix_sec\n" > "$CUE_EVENTS_TSV"

if [[ "$NO_CUES" -eq 1 ]]; then
  for idx in "${!CUE_TIMES_SEC[@]}"; do
    printf "cp_%02d\t%s\t%s\tdisabled\t-\n" "$((idx + 1))" "${CUE_TIMES_SEC[$idx]}" "${CUE_MESSAGES[$idx]}" >> "$CUE_EVENTS_TSV"
  done
fi

detect_screen_device() {
  local device_dump
  device_dump="$(ffmpeg -f avfoundation -list_devices true -i "" 2>&1 || true)"
  local detected
  detected="$({
    printf '%s\n' "$device_dump" | awk '
      /AVFoundation video devices:/ { inVideo = 1; next }
      /AVFoundation audio devices:/ { inVideo = 0 }
      inVideo && /Capture screen/ {
        if (match($0, /\[[0-9]+\]/)) {
          idx = substr($0, RSTART + 1, RLENGTH - 2);
          print idx;
          exit;
        }
      }
    '
  })"
  if [[ -z "$detected" ]]; then
    echo "1"
  else
    echo "$detected"
  fi
}

if [[ -z "$VIDEO_DEVICE" ]]; then
  VIDEO_DEVICE="$(detect_screen_device)"
fi

echo "== Headtracking Rotation Capture =="
echo "out_dir: $OUT_DIR"
echo "duration_sec: $DURATION_SEC"
echo "fps: $FPS"
echo "extract_every_sec: $EXTRACT_EVERY_SEC"
echo "device: $VIDEO_DEVICE"
echo "profile_source: $PROFILE_SOURCE"
echo "session_name: $PROFILE_SESSION_NAME"
echo "schema: $PROFILE_SCHEMA"
echo "dry_run: $([[ "$DRY_RUN" -eq 1 ]] && echo enabled || echo disabled)"
echo "cues: $([[ "$NO_CUES" -eq 1 ]] && echo disabled || echo enabled)"
echo "cue_speech: $([[ "$CUE_SPEECH" -eq 1 ]] && echo enabled || echo disabled)"
echo

if [[ "$NO_CUES" -eq 0 ]]; then
  echo "Cue schedule:"
  for idx in "${!CUE_TIMES_SEC[@]}"; do
    printf "  t=%ss  %s\n" "${CUE_TIMES_SEC[$idx]}" "${CUE_MESSAGES[$idx]}"
  done
  echo
fi

if [[ "$COUNTDOWN_SEC" -gt 0 && "$DRY_RUN" -eq 0 ]]; then
  for ((s=COUNTDOWN_SEC; s>=1; s--)); do
    echo "Recording starts in ${s}s..."
    sleep 1
  done
fi

cue_pid=""
if [[ "$NO_CUES" -eq 0 && "$DRY_RUN" -eq 0 ]]; then
  (
    previous_time=0
    cue_count="${#CUE_TIMES_SEC[@]}"
    for ((idx=0; idx<cue_count; idx++)); do
      cue_time="${CUE_TIMES_SEC[$idx]}"
      cue_text="${CUE_MESSAGES[$idx]}"
      delay="$(awk -v now="$cue_time" -v prev="$previous_time" 'BEGIN { d = now - prev; if (d < 0) d = 0; printf "%.3f", d }')"
      sleep "$delay"
      printf "[cue t=%ss] %s\n" "$cue_time" "$cue_text"
      printf "cp_%02d\t%s\t%s\temitted\t%s\n" "$((idx + 1))" "$cue_time" "$cue_text" "$(date -u +%s)" >> "$CUE_EVENTS_TSV"
      if [[ "$CUE_SPEECH" -eq 1 ]] && command -v say >/dev/null 2>&1; then
        say "$cue_text" >/dev/null 2>&1 || true
      fi
      previous_time="$cue_time"
    done
  ) &
  cue_pid="$!"
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "Dry-run mode enabled; skipping ffmpeg capture and extraction."
  {
    echo "dry_run=true"
    echo "reason=contract_artifact_probe"
  } > "$RECORD_LOG"

  if [[ "$NO_EXTRACT" -eq 0 ]]; then
    mkdir -p "$FRAMES_DIR"
    {
      echo "dry_run=true"
      echo "reason=contract_artifact_probe"
    } > "$EXTRACT_LOG"
  fi

  {
    echo "dry_run capture lane"
    echo "video intentionally skipped"
  } > "$DRY_RUN_NOTE"
else
  echo "Recording..."

  set +e
  ffmpeg -y \
    -hide_banner \
    -loglevel info \
    -f avfoundation \
    -framerate "$FPS" \
    -capture_cursor 1 \
    -capture_mouse_clicks 1 \
    -i "${VIDEO_DEVICE}:none" \
    -t "$DURATION_SEC" \
    -pix_fmt yuv420p \
    "$VIDEO_PATH" >"$RECORD_LOG" 2>&1
  record_ec=$?
  set -e

  if [[ -n "$cue_pid" ]]; then
    kill "$cue_pid" >/dev/null 2>&1 || true
    wait "$cue_pid" 2>/dev/null || true
  fi

  if [[ "$record_ec" -ne 0 ]]; then
    echo "ERROR: ffmpeg recording failed (exit ${record_ec}). See ${RECORD_LOG}" >&2
    exit "$record_ec"
  fi

  if [[ "$NO_EXTRACT" -eq 0 ]]; then
    mkdir -p "$FRAMES_DIR"
    echo "Extracting frames..."
    set +e
    ffmpeg -y \
      -hide_banner \
      -loglevel warning \
      -i "$VIDEO_PATH" \
      -vf "fps=1/${EXTRACT_EVERY_SEC}" \
      "${FRAMES_DIR}/frame_%04d.png" >"$EXTRACT_LOG" 2>&1
    extract_ec=$?
    set -e
    if [[ "$extract_ec" -ne 0 ]]; then
      echo "ERROR: frame extraction failed (exit ${extract_ec}). See ${EXTRACT_LOG}" >&2
      exit "$extract_ec"
    fi
  fi
fi

if [[ "$NO_CUES" -eq 0 && "$DRY_RUN" -eq 1 ]]; then
  for idx in "${!CUE_TIMES_SEC[@]}"; do
    printf "cp_%02d\t%s\t%s\tplanned\t-\n" "$((idx + 1))" "${CUE_TIMES_SEC[$idx]}" "${CUE_MESSAGES[$idx]}" >> "$CUE_EVENTS_TSV"
  done
fi

{
  echo "Title: Headtracking Rotation Capture Summary"
  echo "Document Type: Test Evidence Summary"
  echo "Author: APC Codex"
  echo "Created Date: ${DATE_UTC}"
  echo "Last Modified Date: ${DATE_UTC}"
  echo
  echo "# Headtracking Rotation Capture"
  echo
  echo "- Timestamp (UTC): ${TIMESTAMP}"
  echo "- Session Name: ${PROFILE_SESSION_NAME}"
  echo "- Profile Source: ${PROFILE_SOURCE}"
  echo "- Profile Schema: ${PROFILE_SCHEMA}"
  echo "- Dry Run: $([[ "$DRY_RUN" -eq 1 ]] && echo true || echo false)"
  echo "- Video: ${VIDEO_PATH}"
  echo "- Checkpoints: ${CHECKPOINTS_TSV}"
  echo "- Cue Events: ${CUE_EVENTS_TSV}"
  if [[ "$NO_EXTRACT" -eq 0 ]]; then
    echo "- Frames: ${FRAMES_DIR}"
    echo "- Extract interval: ${EXTRACT_EVERY_SEC}s"
  else
    echo "- Frames: skipped (--no-extract or profile extract_frames=false)"
  fi
  echo "- Record log: ${RECORD_LOG}"
  if [[ "$NO_EXTRACT" -eq 0 ]]; then
    echo "- Extract log: ${EXTRACT_LOG}"
  fi
  if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "- Dry-run note: ${DRY_RUN_NOTE}"
  fi
} >"$SUMMARY_MD"

cat > "$SESSION_MANIFEST_JSON" <<EOF_MANIFEST
{
  "schema": "locusq-capture-session-v1",
  "timestamp_utc": "${TIMESTAMP}",
  "date_utc": "${DATE_UTC}",
  "session_name": "${PROFILE_SESSION_NAME}",
  "profile_source": "${PROFILE_SOURCE}",
  "profile_schema": "${PROFILE_SCHEMA}",
  "mode": "$( [[ "$DRY_RUN" -eq 1 ]] && echo dry_run || echo execute )",
  "config": {
    "duration_sec": ${DURATION_SEC},
    "fps": ${FPS},
    "extract_every_sec": ${EXTRACT_EVERY_SEC},
    "countdown_sec": ${COUNTDOWN_SEC},
    "no_extract": $( [[ "$NO_EXTRACT" -eq 1 ]] && echo true || echo false ),
    "no_cues": $( [[ "$NO_CUES" -eq 1 ]] && echo true || echo false ),
    "cue_speech": $( [[ "$CUE_SPEECH" -eq 1 ]] && echo true || echo false )
  },
  "artifacts": {
    "summary_md": "capture_summary.md",
    "checkpoints_tsv": "checkpoints.tsv",
    "cue_events_tsv": "cue_events.tsv",
    "record_log": "ffmpeg_record.log",
    "extract_log": "ffmpeg_extract.log",
    "video": "rotation_capture.mp4",
    "frames_dir": "frames",
    "artifact_inventory_tsv": "artifact_schema_inventory.tsv",
    "replay_hashes_tsv": "replay_hashes.tsv"
  },
  "cue_count": ${#CUE_TIMES_SEC[@]}
}
EOF_MANIFEST

printf "artifact_id\trelative_path\trequired_in_execute\tpresent\tsize_bytes\tsha256\tnotes\n" > "$ARTIFACT_SCHEMA_TSV"
append_artifact_row "video_capture" "rotation_capture.mp4" "yes" "primary capture output"
append_artifact_row "record_log" "ffmpeg_record.log" "yes" "capture backend log"
append_artifact_row "extract_log" "ffmpeg_extract.log" "no" "frame extraction log"
append_artifact_row "frames_dir" "frames" "no" "frame extraction directory"
append_artifact_row "checkpoints" "checkpoints.tsv" "yes" "scheduled checkpoint contract"
append_artifact_row "cue_events" "cue_events.tsv" "yes" "cue emission event stream"
append_artifact_row "summary" "capture_summary.md" "yes" "human-readable summary"
append_artifact_row "manifest" "session_manifest.json" "yes" "machine-readable session manifest"
append_artifact_row "dry_run_note" "dry_run_note.txt" "no" "dry-run-only note"

cue_schedule_payload="$(awk -F'\t' 'NR>1 { printf "%s|%s|%s\n", $1, $2, $3 }' "$CHECKPOINTS_TSV")"
cue_schedule_hash="$(hash_string_sha256 "$cue_schedule_payload")"

artifact_contract_payload="$(awk -F'\t' 'NR>1 { printf "%s|%s\n", $1, $3 }' "$ARTIFACT_SCHEMA_TSV")"
artifact_contract_hash="$(hash_string_sha256 "$artifact_contract_payload")"

artifact_presence_payload="$(awk -F'\t' 'NR>1 { printf "%s|%s\n", $1, $4 }' "$ARTIFACT_SCHEMA_TSV")"
artifact_presence_hash="$(hash_string_sha256 "$artifact_presence_payload")"

artifact_inventory_hash="$(hash_file_sha256 "$ARTIFACT_SCHEMA_TSV")"
session_manifest_hash="$(hash_file_sha256 "$SESSION_MANIFEST_JSON")"
script_contract_hash="$(hash_file_sha256 "$0")"

printf "key\tvalue\tdetail\n" > "$REPLAY_HASHES_TSV"
printf "profile_contract_hash\t%s\tprofile schema/value contract hash\n" "$PROFILE_CONTRACT_HASH" >> "$REPLAY_HASHES_TSV"
printf "cue_schedule_hash\t%s\tcheckpoint schedule hash\n" "$cue_schedule_hash" >> "$REPLAY_HASHES_TSV"
printf "artifact_contract_hash\t%s\tartifact id/required contract hash\n" "$artifact_contract_hash" >> "$REPLAY_HASHES_TSV"
printf "artifact_presence_hash\t%s\tartifact present/absent matrix hash\n" "$artifact_presence_hash" >> "$REPLAY_HASHES_TSV"
printf "artifact_inventory_hash\t%s\tfull artifact inventory file hash\n" "$artifact_inventory_hash" >> "$REPLAY_HASHES_TSV"
printf "session_manifest_hash\t%s\tsession manifest hash\n" "$session_manifest_hash" >> "$REPLAY_HASHES_TSV"
printf "capture_script_hash\t%s\tscript file hash\n" "$script_contract_hash" >> "$REPLAY_HASHES_TSV"
printf "cue_count\t%s\tnumber of scheduled cues\n" "${#CUE_TIMES_SEC[@]}" >> "$REPLAY_HASHES_TSV"

echo "Summary: $SUMMARY_MD"
echo "Manifest: $SESSION_MANIFEST_JSON"
echo "Inventory: $ARTIFACT_SCHEMA_TSV"
echo "Replay hashes: $REPLAY_HASHES_TSV"

if [[ "$OPEN_OUTPUT" -eq 1 ]]; then
  open "$OUT_DIR"
fi

echo "Done."
