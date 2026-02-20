#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"

DEFAULT_APP_PATHS=(
  "$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
  "$ROOT_DIR/build/LocusQ_artefacts/Standalone/LocusQ.app"
  "$ROOT_DIR/build_ship_universal/LocusQ_artefacts/Release/Standalone/LocusQ.app"
)

APP_PATH="${1:-}"
if [[ -z "$APP_PATH" ]]; then
  for candidate in "${DEFAULT_APP_PATHS[@]}"; do
    if [[ -d "$candidate" ]]; then
      APP_PATH="$candidate"
      break
    fi
  done
fi

if [[ -z "$APP_PATH" || ! -d "$APP_PATH" ]]; then
  echo "ERROR: LocusQ standalone app not found. Pass path explicitly:"
  echo "  scripts/standalone-ui-smoke-stage1-mac.sh /path/to/LocusQ.app"
  exit 2
fi

APP_NAME="$(basename "$APP_PATH" .app)"
OUT_DIR="$ROOT_DIR/TestEvidence/standalone_ui_smoke_stage1_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

SUMMARY_TSV="$OUT_DIR/summary.tsv"
LOG_TXT="$OUT_DIR/run.log"
echo -e "test_id\tresult\tdiff_score\tthreshold\tartifacts" > "$SUMMARY_TSV"

exec > >(tee -a "$LOG_TXT") 2>&1

echo "=== LocusQ Standalone UI Smoke (Incremental Stage 1, macOS) ==="
echo "app_path: $APP_PATH"
echo "app_name: $APP_NAME"
echo "out_dir:  $OUT_DIR"

close_all_app_instances() {
  local phase="$1"
  local pids
  pids="$(pgrep -x "$APP_NAME" || true)"
  if [[ -z "$pids" ]]; then
    return 0
  fi

  echo "[${phase}] Closing existing ${APP_NAME} instance(s): $(echo "$pids" | tr '\n' ' ')"
  osascript >/dev/null 2>&1 <<OSA || true
tell application "$APP_NAME" to quit
OSA

  for _ in {1..40}; do
    if ! pgrep -x "$APP_NAME" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.15
  done

  echo "[${phase}] Graceful quit timed out; sending TERM."
  pkill -TERM -x "$APP_NAME" >/dev/null 2>&1 || true
  for _ in {1..20}; do
    if ! pgrep -x "$APP_NAME" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.15
  done

  echo "[${phase}] TERM timed out; sending KILL."
  pkill -KILL -x "$APP_NAME" >/dev/null 2>&1 || true
}

cleanup() {
  local exit_code="$?"
  close_all_app_instances "cleanup"
  if [[ "$exit_code" -ne 0 ]]; then
    echo "Smoke script exited with code ${exit_code}"
  fi
}
trap cleanup EXIT INT TERM

osascript -e 'tell application "System Events" to UI elements enabled' >/dev/null \
  || { echo "ERROR: UI scripting is not enabled. Enable Accessibility for Terminal and Script Editor."; exit 3; }

if ! osascript <<'OSA' >/dev/null 2>&1
tell application "System Events"
  tell process "Finder"
    set _ to count windows
  end tell
end tell
OSA
then
  echo "ERROR: This shell is not permitted to send Accessibility events."
  echo "Grant access in: System Settings -> Privacy & Security -> Accessibility"
  echo "Enable your terminal app (Terminal/iTerm) and /usr/bin/osascript, then rerun."
  exit 3
fi

close_all_app_instances "preflight"
open "$APP_PATH"

for _ in {1..50}; do
  if pgrep -x "$APP_NAME" >/dev/null 2>&1; then
    break
  fi
  sleep 0.2
done

if ! pgrep -x "$APP_NAME" >/dev/null 2>&1; then
  echo "ERROR: Failed to start $APP_NAME."
  exit 4
fi

osascript >/dev/null <<OSA
tell application "$APP_NAME" to activate
delay 0.6
tell application "System Events"
  tell process "$APP_NAME"
    set frontmost to true
    if (count of windows) = 0 then error "No front window for $APP_NAME"
    set position of front window to {120, 80}
    set size of front window to {1280, 820}
  end tell
end tell
OSA

sleep 0.6

WINDOW_GEOM="$(osascript <<OSA
tell application "System Events"
  tell process "$APP_NAME"
    if (count of windows) = 0 then error "No front window for $APP_NAME"
    set p to value of attribute "AXPosition" of front window
    set s to value of attribute "AXSize" of front window
    return (item 1 of p as string) & "," & (item 2 of p as string) & "," & (item 1 of s as string) & "," & (item 2 of s as string)
  end tell
end tell
OSA
)"
WINDOW_GEOM="${WINDOW_GEOM//[[:space:]]/}"
IFS=',' read -r WIN_X WIN_Y WIN_W WIN_H <<< "$WINDOW_GEOM"
echo "window_geom: ${WIN_X},${WIN_Y},${WIN_W},${WIN_H}"

capture_window() {
  local path="$1"
  screencapture -x -R"${WIN_X},${WIN_Y},${WIN_W},${WIN_H}" "$path"
}

click_abs() {
  local rel_x="$1"
  local rel_y="$2"
  local abs_x=$((WIN_X + rel_x))
  local abs_y=$((WIN_Y + rel_y))
  osascript >/dev/null <<OSA
tell application "System Events"
  tell process "$APP_NAME"
    set frontmost to true
    click at {${abs_x}, ${abs_y}}
  end tell
end tell
OSA
}

send_keys() {
  local script="$1"
  osascript >/dev/null <<OSA
tell application "System Events"
  tell process "$APP_NAME"
    set frontmost to true
    ${script}
  end tell
end tell
OSA
}

diff_score() {
  local before="$1"
  local after="$2"
  local x="$3"
  local y="$4"
  local w="$5"
  local h="$6"

  python3 - "$before" "$after" "$x" "$y" "$w" "$h" <<'PY'
import sys
from PIL import Image, ImageChops, ImageStat

before_path, after_path, x, y, w, h = sys.argv[1:]
x = int(x); y = int(y); w = int(w); h = int(h)

before = Image.open(before_path).convert("RGB")
after = Image.open(after_path).convert("RGB")

x = max(0, min(before.width - 1, x))
y = max(0, min(before.height - 1, y))
w = max(1, min(before.width - x, w))
h = max(1, min(before.height - y, h))

box = (x, y, x + w, y + h)
b = before.crop(box)
a = after.crop(box)
diff = ImageChops.difference(b, a)
stat = ImageStat.Stat(diff)
score = sum(stat.mean) / len(stat.mean)
print(f"{score:.6f}")
PY
}

BOOTSHOT="$OUT_DIR/_bootstrap_window.png"
capture_window "$BOOTSHOT"

SHOT_GEOM="$(python3 - "$BOOTSHOT" <<'PY'
import sys
from PIL import Image, ImageStat

img = Image.open(sys.argv[1]).convert("RGB")
w, h = img.size
limit = min(h, 560)

lum = []
for y in range(limit):
    row = img.crop((0, y, w, y + 1))
    r, g, b = ImageStat.Stat(row).mean
    lum.append(0.2126 * r + 0.7152 * g + 0.0722 * b)

content_y = 0
for yy in range(20, max(21, limit - 7)):
    if all(lum[yy + k] < 90 for k in range(6)):
        content_y = yy
        break

print(f"{w},{h},{content_y}")
PY
)"
IFS=',' read -r SHOT_W SHOT_H CONTENT_Y_PX <<< "$SHOT_GEOM"
echo "screenshot_geom: ${SHOT_W}x${SHOT_H}, content_y_px=${CONTENT_Y_PX}"

SCALE_X="$(python3 - <<PY
print(${SHOT_W} / ${WIN_W})
PY
)"
SCALE_Y="$(python3 - <<PY
print(${SHOT_H} / ${WIN_H})
PY
)"
echo "scale: x=${SCALE_X}, y=${SCALE_Y}"

# Stage-1 harness is centered and stable at fixed window size.
# Use ratio-based click points in window points to avoid Retina scaling issues.
TOGGLE_X_PT=$((WIN_W * 22 / 100))
TOGGLE_Y_PT=$((WIN_H * 36 / 100))
CHOICE_X_PT=$((WIN_W * 49 / 100))
CHOICE_Y_PT=$((WIN_H * 36 / 100))
SLIDER_X_PT=$((WIN_W * 69 / 100))
SLIDER_Y_PT=$((WIN_H * 36 / 100))

# Control-local ROIs in window points (converted to screenshot pixels per capture).
TOGGLE_ROI_X_PT=$((WIN_W * 18 / 100))
TOGGLE_ROI_Y_PT=$((WIN_H * 31 / 100))
TOGGLE_ROI_W_PT=$((WIN_W * 12 / 100))
TOGGLE_ROI_H_PT=$((WIN_H * 10 / 100))

CHOICE_ROI_X_PT=$((WIN_W * 42 / 100))
CHOICE_ROI_Y_PT=$((WIN_H * 31 / 100))
CHOICE_ROI_W_PT=$((WIN_W * 16 / 100))
CHOICE_ROI_H_PT=$((WIN_H * 11 / 100))

SLIDER_ROI_X_PT=$((WIN_W * 63 / 100))
SLIDER_ROI_Y_PT=$((WIN_H * 31 / 100))
SLIDER_ROI_W_PT=$((WIN_W * 14 / 100))
SLIDER_ROI_H_PT=$((WIN_H * 11 / 100))

run_stage1_test() {
  local test_id="$1"
  local action="$2"
  local threshold="$3"
  local roi_x_pt="$4"
  local roi_y_pt="$5"
  local roi_w_pt="$6"
  local roi_h_pt="$7"
  local before="$OUT_DIR/${test_id}_before.png"
  local after="$OUT_DIR/${test_id}_after.png"

  capture_window "$before"
  eval "$action"
  sleep 0.35
  capture_window "$after"

  local roi_x_px roi_y_px roi_w_px roi_h_px
  roi_x_px="$(python3 - <<PY
print(int(round(${roi_x_pt} * ${SCALE_X})))
PY
)"
  roi_y_px="$(python3 - <<PY
print(int(round(${roi_y_pt} * ${SCALE_Y})))
PY
)"
  roi_w_px="$(python3 - <<PY
print(int(round(${roi_w_pt} * ${SCALE_X})))
PY
)"
  roi_h_px="$(python3 - <<PY
print(int(round(${roi_h_pt} * ${SCALE_Y})))
PY
)"

  local score
  score="$(diff_score "$before" "$after" "$roi_x_px" "$roi_y_px" "$roi_w_px" "$roi_h_px")"

  local result="FAIL"
  awk -v a="$score" -v b="$threshold" 'BEGIN{exit !(a > b)}' && result="PASS"
  echo -e "${test_id}\t${result}\t${score}\t${threshold}\t${before}|${after}" >> "$SUMMARY_TSV"
  echo "${test_id}: ${result} (score=${score}, threshold=${threshold})"
}

run_stage1_test "S1-01-toggle" "click_abs ${TOGGLE_X_PT} ${TOGGLE_Y_PT}" "0.06" \
  "${TOGGLE_ROI_X_PT}" "${TOGGLE_ROI_Y_PT}" "${TOGGLE_ROI_W_PT}" "${TOGGLE_ROI_H_PT}"
run_stage1_test "S1-02-choice" "click_abs ${CHOICE_X_PT} ${CHOICE_Y_PT}; send_keys \"key code 125\"; send_keys \"key code 36\"" "0.06" \
  "${CHOICE_ROI_X_PT}" "${CHOICE_ROI_Y_PT}" "${CHOICE_ROI_W_PT}" "${CHOICE_ROI_H_PT}"
run_stage1_test "S1-03-slider" "click_abs ${SLIDER_X_PT} ${SLIDER_Y_PT}; send_keys \"key code 124\"; send_keys \"key code 124\"; send_keys \"key code 124\"" "0.06" \
  "${SLIDER_ROI_X_PT}" "${SLIDER_ROI_Y_PT}" "${SLIDER_ROI_W_PT}" "${SLIDER_ROI_H_PT}"

close_all_app_instances "post-run"

echo
echo "Summary: $SUMMARY_TSV"
cat "$SUMMARY_TSV"

PASS_COUNT="$(awk 'NR>1 && $2=="PASS"{c++} END{print c+0}' "$SUMMARY_TSV")"
TOTAL_COUNT="$(awk 'NR>1{c++} END{print c+0}' "$SUMMARY_TSV")"

if [[ "$PASS_COUNT" -eq "$TOTAL_COUNT" ]]; then
  echo "RESULT: PASS (${PASS_COUNT}/${TOTAL_COUNT})"
  exit 0
fi

echo "RESULT: FAIL (${PASS_COUNT}/${TOTAL_COUNT})"
exit 1
