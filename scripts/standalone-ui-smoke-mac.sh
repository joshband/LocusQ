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
  echo "  scripts/standalone-ui-smoke-mac.sh /path/to/LocusQ.app"
  exit 2
fi

APP_NAME="$(basename "$APP_PATH" .app)"
OUT_DIR="$ROOT_DIR/TestEvidence/standalone_ui_smoke_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

SUMMARY_TSV="$OUT_DIR/summary.tsv"
LOG_TXT="$OUT_DIR/run.log"
echo -e "test_id\tresult\tdiff_score\tthreshold\tartifacts" > "$SUMMARY_TSV"

exec > >(tee -a "$LOG_TXT") 2>&1

echo "=== LocusQ Standalone UI Smoke (macOS) ==="
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

get_window_geometry_once() {
  osascript 2>/dev/null <<OSA || true
tell application "System Events"
  tell process "$APP_NAME"
    if (count of windows) = 0 then error "No front window for $APP_NAME"
    set p to value of attribute "AXPosition" of front window
    set s to value of attribute "AXSize" of front window
    set xPos to (item 1 of p) as integer
    set yPos to (item 2 of p) as integer
    set w to (item 1 of s) as integer
    set h to (item 2 of s) as integer
    return (xPos as string) & "," & (yPos as string) & "," & (w as string) & "," & (h as string)
  end tell
end tell
OSA
}

WINDOW_GEOM=""
for _ in {1..30}; do
  candidate="$(get_window_geometry_once)"
  candidate="${candidate//[[:space:]]/}"
  if [[ "$candidate" =~ ^[0-9]+,[0-9]+,[0-9]+,[0-9]+$ ]]; then
    IFS=',' read -r cx cy cw ch <<< "$candidate"
    if (( cw > 200 && ch > 200 )); then
      WINDOW_GEOM="$candidate"
      break
    fi
  fi
  sleep 0.15
done

if [[ -z "$WINDOW_GEOM" ]]; then
  # Fallback to expected geometry if AX data is temporarily unavailable.
  WINDOW_GEOM="120,80,1280,820"
  echo "WARN: Window geometry query unavailable; using fallback ${WINDOW_GEOM}"
fi

IFS=',' read -r WIN_X WIN_Y WIN_W WIN_H <<< "$WINDOW_GEOM"
echo "window_geom: ${WIN_X},${WIN_Y},${WIN_W},${WIN_H}"

CONTENT_X_OFFSET="${CONTENT_X_OFFSET_OVERRIDE:-0}"
CONTENT_Y_OFFSET="${CONTENT_Y_OFFSET_OVERRIDE:-}"
AUTO_CONTENT_Y_OFFSET="${AUTO_CONTENT_Y_OFFSET:-0}"
CAPTURE_SCALE_X=1
CAPTURE_SCALE_Y=1
if [[ ! "$CONTENT_X_OFFSET" =~ ^-?[0-9]+$ ]]; then
  CONTENT_X_OFFSET=0
fi

detect_content_y_offset() {
  local image_path="$1"
  python3 - "$image_path" <<'PY'
import sys
from PIL import Image, ImageStat

path = sys.argv[1]
img = Image.open(path).convert("RGB")
w, h = img.size
limit = min(h, 280)

lum = []
for y in range(limit):
    row = img.crop((0, y, w, y + 1))
    r, g, b = ImageStat.Stat(row).mean
    lum.append(0.2126 * r + 0.7152 * g + 0.0722 * b)

# Detect bright warning strip band common in JUCE standalone.
band = None
y = 10
while y < limit:
    if lum[y] > 170:
        start = y
        while y < limit and lum[y] > 170:
            y += 1
        end = y - 1
        if end - start + 1 >= 8:
            band = (start, end)
            break
    else:
        y += 1

if band is not None:
    _, end = band
    for yy in range(end + 1, min(limit, end + 150)):
        if lum[yy] < 90:
            print(yy)
            sys.exit(0)

# Fallback: first sufficiently dark stable run after titlebar.
for yy in range(20, max(21, limit - 6)):
    if all(lum[yy + k] < 90 for k in range(6)):
        print(yy)
        sys.exit(0)

print(0)
PY
}

detect_capture_scale() {
  local image_path="$1"
  python3 - "$image_path" "$WIN_W" "$WIN_H" <<'PY'
import sys
from PIL import Image

image_path, win_w, win_h = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
img = Image.open(image_path)
img_w, img_h = img.size

def resolve_scale(pixel_size, point_size):
    if point_size <= 0:
        return 1.0
    raw = pixel_size / float(point_size)
    rounded = round(raw)
    if rounded >= 1 and abs(raw - rounded) <= 0.2:
        return float(rounded)
    return max(1.0, raw)

sx = resolve_scale(img_w, win_w)
sy = resolve_scale(img_h, win_h)
print(f"{sx:.6f} {sy:.6f}")
PY
}

scale_to_pixels() {
  local points="$1"
  local scale="$2"
  python3 - "$points" "$scale" <<'PY'
import sys

points = float(sys.argv[1])
scale = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0
if not scale > 0:
    scale = 1.0
print(int(round(points * scale)))
PY
}

pixels_to_points() {
  local pixels="$1"
  local scale="$2"
  python3 - "$pixels" "$scale" <<'PY'
import sys

pixels = float(sys.argv[1])
scale = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0
if not scale > 0:
    scale = 1.0
print(int(round(pixels / scale)))
PY
}

capture_window() {
  local path="$1"
  if ! screencapture -x -R"${WIN_X},${WIN_Y},${WIN_W},${WIN_H}" "$path"; then
    echo "WARN: Rect capture failed; falling back to full-screen capture."
    screencapture -x "$path"
  fi
}

click_rel() {
  local rel_x="$1"
  local rel_y="$2"
  osascript >/dev/null <<OSA
tell application "System Events"
  tell process "$APP_NAME"
    set frontmost to true
    set p to position of front window
    set absX to (item 1 of p) + ${CONTENT_X_OFFSET} + ${rel_x}
    set absY to (item 2 of p) + ${CONTENT_Y_OFFSET:-0} + ${rel_y}
    click at {absX, absY}
  end tell
end tell
OSA
}

type_text_replace() {
  local text="$1"
  osascript >/dev/null <<OSA
tell application "System Events"
  tell process "$APP_NAME"
    keystroke "a" using {command down}
    key code 51
    keystroke "${text}"
    key code 36
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
box = (x, y, x + w, y + h)
b = before.crop(box)
a = after.crop(box)
diff = ImageChops.difference(b, a)
stat = ImageStat.Stat(diff)
score = sum(stat.mean) / len(stat.mean)
print(f"{score:.6f}")
PY
}

run_click_test() {
  local test_id="$1"
  local click_x="$2"
  local click_y="$3"
  local roi_x="$4"
  local roi_y="$5"
  local roi_w="$6"
  local roi_h="$7"
  local threshold="$8"

  local before="$OUT_DIR/${test_id}_before.png"
  local after="$OUT_DIR/${test_id}_after.png"

  capture_window "$before"
  click_rel "$click_x" "$click_y"
  sleep 0.35
  capture_window "$after"

  local score
  local roi_x_adj_pt=$((roi_x + CONTENT_X_OFFSET))
  local roi_y_adj_pt=$((roi_y + CONTENT_Y_OFFSET))
  local roi_x_adj_px
  local roi_y_adj_px
  local roi_w_px
  local roi_h_px
  roi_x_adj_px="$(scale_to_pixels "$roi_x_adj_pt" "$CAPTURE_SCALE_X")"
  roi_y_adj_px="$(scale_to_pixels "$roi_y_adj_pt" "$CAPTURE_SCALE_Y")"
  roi_w_px="$(scale_to_pixels "$roi_w" "$CAPTURE_SCALE_X")"
  roi_h_px="$(scale_to_pixels "$roi_h" "$CAPTURE_SCALE_Y")"
  score="$(diff_score "$before" "$after" "$roi_x_adj_px" "$roi_y_adj_px" "$roi_w_px" "$roi_h_px")"

  local result="FAIL"
  awk -v a="$score" -v b="$threshold" 'BEGIN{exit !(a > b)}' && result="PASS"
  echo -e "${test_id}\t${result}\t${score}\t${threshold}\t${before}|${after}" >> "$SUMMARY_TSV"
  echo "${test_id}: ${result} (score=${score}, threshold=${threshold})"
}

run_text_test() {
  local test_id="$1"
  local field_x="$2"
  local field_y="$3"
  local text="$4"
  local roi_x="$5"
  local roi_y="$6"
  local roi_w="$7"
  local roi_h="$8"
  local threshold="$9"

  local before="$OUT_DIR/${test_id}_before.png"
  local after="$OUT_DIR/${test_id}_after.png"

  capture_window "$before"
  click_rel "$field_x" "$field_y"
  sleep 0.2
  type_text_replace "$text"
  sleep 0.35
  capture_window "$after"

  local score
  local roi_x_adj_pt=$((roi_x + CONTENT_X_OFFSET))
  local roi_y_adj_pt=$((roi_y + CONTENT_Y_OFFSET))
  local roi_x_adj_px
  local roi_y_adj_px
  local roi_w_px
  local roi_h_px
  roi_x_adj_px="$(scale_to_pixels "$roi_x_adj_pt" "$CAPTURE_SCALE_X")"
  roi_y_adj_px="$(scale_to_pixels "$roi_y_adj_pt" "$CAPTURE_SCALE_Y")"
  roi_w_px="$(scale_to_pixels "$roi_w" "$CAPTURE_SCALE_X")"
  roi_h_px="$(scale_to_pixels "$roi_h" "$CAPTURE_SCALE_Y")"
  score="$(diff_score "$before" "$after" "$roi_x_adj_px" "$roi_y_adj_px" "$roi_w_px" "$roi_h_px")"

  local result="FAIL"
  awk -v a="$score" -v b="$threshold" 'BEGIN{exit !(a > b)}' && result="PASS"
  echo -e "${test_id}\t${result}\t${score}\t${threshold}\t${before}|${after}" >> "$SUMMARY_TSV"
  echo "${test_id}: ${result} (score=${score}, threshold=${threshold})"
}

run_presence_test() {
  local test_id="$1"
  local roi_x="$2"
  local roi_y="$3"
  local roi_w="$4"
  local roi_h="$5"
  local min_mean="$6"
  local min_std="$7"

  local shot="$OUT_DIR/${test_id}.png"
  capture_window "$shot"

  local roi_x_adj_pt=$((roi_x + CONTENT_X_OFFSET))
  local roi_y_adj_pt=$((roi_y + CONTENT_Y_OFFSET))
  local roi_x_adj_px
  local roi_y_adj_px
  local roi_w_px
  local roi_h_px
  roi_x_adj_px="$(scale_to_pixels "$roi_x_adj_pt" "$CAPTURE_SCALE_X")"
  roi_y_adj_px="$(scale_to_pixels "$roi_y_adj_pt" "$CAPTURE_SCALE_Y")"
  roi_w_px="$(scale_to_pixels "$roi_w" "$CAPTURE_SCALE_X")"
  roi_h_px="$(scale_to_pixels "$roi_h" "$CAPTURE_SCALE_Y")"

  local stats
  stats="$(python3 - "$shot" "$roi_x_adj_px" "$roi_y_adj_px" "$roi_w_px" "$roi_h_px" <<'PY'
import sys
from PIL import Image, ImageStat

path, x, y, w, h = sys.argv[1:]
x = int(x); y = int(y); w = int(w); h = int(h)
img = Image.open(path).convert("L")
roi = img.crop((x, y, x + w, y + h))
stat = ImageStat.Stat(roi)
print(f"{stat.mean[0]:.6f} {stat.stddev[0]:.6f}")
PY
)"
  local mean std
  read -r mean std <<< "$stats"

  local result="FAIL"
  awk -v m="$mean" -v s="$std" -v mm="$min_mean" -v ms="$min_std" 'BEGIN{exit !((m >= mm) && (s >= ms))}' \
    && result="PASS"

  echo -e "${test_id}\t${result}\t${std}\t${min_std}\t${shot}" >> "$SUMMARY_TSV"
  echo "${test_id}: ${result} (mean=${mean}, std=${std}, min_mean=${min_mean}, min_std=${min_std})"
}

# Coordinates are relative to plugin content top-left (not title/warning strip).
# Script auto-detects content Y offset for JUCE standalone windows.
BOOTSHOT="$OUT_DIR/_bootstrap_window.png"
capture_window "$BOOTSHOT"

read -r CAPTURE_SCALE_X CAPTURE_SCALE_Y <<< "$(detect_capture_scale "$BOOTSHOT")"
if [[ ! "$CAPTURE_SCALE_X" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
  CAPTURE_SCALE_X=1
fi
if [[ ! "$CAPTURE_SCALE_Y" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
  CAPTURE_SCALE_Y=1
fi

if [[ -z "$CONTENT_Y_OFFSET" && "$AUTO_CONTENT_Y_OFFSET" == "1" ]]; then
  CONTENT_Y_OFFSET_PX="$(detect_content_y_offset "$BOOTSHOT")"
  if [[ ! "$CONTENT_Y_OFFSET_PX" =~ ^-?[0-9]+$ ]]; then
    CONTENT_Y_OFFSET_PX=0
  fi
  CONTENT_Y_OFFSET="$(pixels_to_points "$CONTENT_Y_OFFSET_PX" "$CAPTURE_SCALE_Y")"
fi
if [[ ! "$CONTENT_Y_OFFSET" =~ ^-?[0-9]+$ ]]; then
  CONTENT_Y_OFFSET=0
fi
echo "capture_scale: x=${CAPTURE_SCALE_X}, y=${CAPTURE_SCALE_Y}"
echo "content_offset: x=${CONTENT_X_OFFSET}, y=${CONTENT_Y_OFFSET} (points)"

# Stabilize initial state: enter emitter mode before smoke checks.
click_rel 235 91
sleep 0.30

# Smoke-level checks for visual state changes, not full semantic correctness.
# Coordinates are tuned for production-ui layout at 1202x858 points and are
# scaled to capture pixels via CAPTURE_SCALE_* above.
run_click_test "UI-01-tab-renderer"   314 91   170 56 220 50 0.80
run_click_test "UI-01-tab-emitter"    235 91   170 56 220 50 0.80
run_click_test "UI-02-quality-badge" 1159 67  1098 46  95 40 0.60
run_presence_test "UI-03-timeline-header-visible" 620 700 140 30 16.0 10.0
run_presence_test "UI-04-timeline-lanes-visible"   20 730 720 127 16.0 8.0
run_text_test  "UI-05-emit-label"    1120 127 "AutoUITest" 1033 108 170 40 0.35

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
