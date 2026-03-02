#!/usr/bin/env bash
set -euo pipefail

if [[ "${OSTYPE:-}" != darwin* ]]; then
  echo "ERROR: scripts/capture-headtracking-rotation-mac.sh is macOS-only." >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

OUT_DIR="${ROOT_DIR}/TestEvidence/headtracking_rotation_capture_${TIMESTAMP}"
DURATION_SEC=40
FPS=30
EXTRACT_EVERY_SEC=0.5
COUNTDOWN_SEC=5
VIDEO_DEVICE=""
NO_EXTRACT=0
OPEN_OUTPUT=0
NO_CUES=0
CUE_SPEECH=0

CUE_TIMES_SEC=(0 12 24 36 48)
CUE_MESSAGES=(
  "Center and sync"
  "Rotate clockwise to 90 degrees"
  "Rotate clockwise to 180 degrees"
  "Rotate clockwise to 225 degrees"
  "Rotate clockwise to 270 degrees"
)

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
  --cue-speech                Speak cues using macOS 'say' command
  --open-output               Open output folder when complete
  --help                      Show this message

Examples:
  ./scripts/capture-headtracking-rotation-mac.sh
  ./scripts/capture-headtracking-rotation-mac.sh --duration 60 --extract-every 0.25
  ./scripts/capture-headtracking-rotation-mac.sh --duration 60 --cue-speech
  ./scripts/capture-headtracking-rotation-mac.sh --device 1 --out-dir TestEvidence/rotation_run_a
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --duration)
      DURATION_SEC="$2"
      shift 2
      ;;
    --fps)
      FPS="$2"
      shift 2
      ;;
    --extract-every)
      EXTRACT_EVERY_SEC="$2"
      shift 2
      ;;
    --countdown)
      COUNTDOWN_SEC="$2"
      shift 2
      ;;
    --device)
      VIDEO_DEVICE="$2"
      shift 2
      ;;
    --no-extract)
      NO_EXTRACT=1
      shift
      ;;
    --no-cues)
      NO_CUES=1
      shift
      ;;
    --cue-speech)
      CUE_SPEECH=1
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

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ERROR: ffmpeg is required but not found on PATH." >&2
  exit 1
fi

mkdir -p "$OUT_DIR"
VIDEO_PATH="${OUT_DIR}/rotation_capture.mp4"
RECORD_LOG="${OUT_DIR}/ffmpeg_record.log"
FRAMES_DIR="${OUT_DIR}/frames"
EXTRACT_LOG="${OUT_DIR}/ffmpeg_extract.log"
SUMMARY_MD="${OUT_DIR}/capture_summary.md"

detect_screen_device() {
  local device_dump
  device_dump="$(ffmpeg -f avfoundation -list_devices true -i "" 2>&1 || true)"
  local detected
  detected="$(
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
  )"
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
echo "cues: $([[ "$NO_CUES" -eq 1 ]] && echo disabled || echo enabled)"
echo "cue_speech: $([[ "$CUE_SPEECH" -eq 1 ]] && echo enabled || echo disabled)"
echo
echo "Set up both apps side-by-side now:"
echo "  - LocusQ Head-Tracking Companion"
echo "  - LocusQ (CALIBRATE top view)"
echo "Follow one clockwise pass: center -> 90 -> 180 -> 225 -> 270."
echo

if [[ "$NO_CUES" -eq 0 ]]; then
  echo "Cue schedule:"
  for idx in "${!CUE_TIMES_SEC[@]}"; do
    printf "  t=%ss  %s\n" "${CUE_TIMES_SEC[$idx]}" "${CUE_MESSAGES[$idx]}"
  done
  echo
fi

if [[ "$COUNTDOWN_SEC" -gt 0 ]]; then
  for ((s=COUNTDOWN_SEC; s>=1; s--)); do
    echo "Recording starts in ${s}s..."
    sleep 1
  done
fi

echo "Recording..."
cue_pid=""
if [[ "$NO_CUES" -eq 0 ]]; then
  (
    previous_time=0
    cue_count="${#CUE_TIMES_SEC[@]}"
    for ((idx=0; idx<cue_count; idx++)); do
      cue_time="${CUE_TIMES_SEC[$idx]}"
      cue_text="${CUE_MESSAGES[$idx]}"
      delay="$(awk -v now="$cue_time" -v prev="$previous_time" 'BEGIN { d = now - prev; if (d < 0) d = 0; printf "%.3f", d }')"
      sleep "$delay"
      printf "[cue t=%ss] %s\n" "$cue_time" "$cue_text"
      if [[ "$CUE_SPEECH" -eq 1 ]] && command -v say >/dev/null 2>&1; then
        say "$cue_text" >/dev/null 2>&1 || true
      fi
      previous_time="$cue_time"
    done
  ) &
  cue_pid="$!"
fi

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

echo "Recording complete: $VIDEO_PATH"

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

{
  echo "# Headtracking Rotation Capture"
  echo
  echo "- Timestamp (UTC): ${TIMESTAMP}"
  echo "- Video: ${VIDEO_PATH}"
  if [[ "$NO_EXTRACT" -eq 0 ]]; then
    echo "- Frames: ${FRAMES_DIR}"
    echo "- Extract interval: ${EXTRACT_EVERY_SEC}s"
  else
    echo "- Frames: skipped (--no-extract)"
  fi
  echo "- Record log: ${RECORD_LOG}"
  if [[ "$NO_EXTRACT" -eq 0 ]]; then
    echo "- Extract log: ${EXTRACT_LOG}"
  fi
} >"$SUMMARY_MD"

echo "Summary: $SUMMARY_MD"

if [[ "$OPEN_OUTPUT" -eq 1 ]]; then
  open "$OUT_DIR"
fi

echo "Done."
