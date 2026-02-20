#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVER_URL_DEFAULT="${APPIUM_SERVER_URL:-http://127.0.0.1:4723}"
VENV_DIR_DEFAULT="${APPIUM_UI_VENV_DIR:-$ROOT_DIR/.venv-ui}"
VENV_PYTHON_DEFAULT="${VENV_DIR_DEFAULT}/bin/python3"
REQS_FILE="$ROOT_DIR/qa/ui/requirements-appium-mac2.txt"
STEP_TIMEOUT_DEFAULT="${APPIUM_UI_STEP_TIMEOUT_SECONDS:-10}"
MAX_RUN_DEFAULT="${APPIUM_UI_MAX_RUN_SECONDS:-180}"

DEFAULT_APP_PATHS=(
  "$ROOT_DIR/build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app"
  "$ROOT_DIR/build/LocusQ_artefacts/Standalone/LocusQ.app"
  "$ROOT_DIR/build_ship_universal/LocusQ_artefacts/Release/Standalone/LocusQ.app"
  "/Applications/LocusQ.app"
)

APP_PATH="${1:-}"
if [[ -n "$APP_PATH" && "$APP_PATH" == *.app ]]; then
  shift
else
  APP_PATH=""
fi

if [[ -z "$APP_PATH" ]]; then
  for candidate in "${DEFAULT_APP_PATHS[@]}"; do
    if [[ -d "$candidate" ]]; then
      APP_PATH="$candidate"
      break
    fi
  done
fi

if [[ -z "$APP_PATH" || ! -d "$APP_PATH" ]]; then
  echo "ERROR: LocusQ standalone app not found."
  echo "Pass path explicitly:"
  echo "  ./scripts/appium-mac2-ui-regression.sh /path/to/LocusQ.app"
  exit 2
fi

if ! command -v appium >/dev/null 2>&1; then
  echo "ERROR: 'appium' is not installed."
  echo "Install and retry:"
  echo "  npm install -g appium"
  exit 3
fi

set +e
APP_DRIVER_LIST_OUTPUT="$(appium driver list --installed 2>&1)"
APP_DRIVER_LIST_STATUS=$?
set -e

if [[ $APP_DRIVER_LIST_STATUS -ne 0 ]]; then
  echo "ERROR: Failed to query installed Appium drivers."
  echo "$APP_DRIVER_LIST_OUTPUT"
  if echo "$APP_DRIVER_LIST_OUTPUT" | grep -Eiq 'must be writeable'; then
    echo "Fix Appium home permissions and retry:"
    echo "  mkdir -p \"$HOME/.appium\""
    echo "  chmod -R u+rwX \"$HOME/.appium\""
  fi
  exit 4
fi

if ! echo "$APP_DRIVER_LIST_OUTPUT" | grep -Eiq 'mac2'; then
  echo "ERROR: Appium mac2 driver is not installed."
  echo "Install and retry:"
  echo "  appium driver install mac2"
  exit 4
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "ERROR: 'python3' is not available."
  exit 5
fi

has_python_deps() {
  local py_bin="$1"
  "$py_bin" - <<'PY' >/dev/null 2>&1
import appium
import PIL
import selenium
PY
}

PYTHON_BIN="${APPIUM_UI_PYTHON:-python3}"
if [[ "$PYTHON_BIN" == "python3" && -x "$VENV_PYTHON_DEFAULT" ]]; then
  # Prefer local venv when present to avoid PEP 668 system-package constraints.
  PYTHON_BIN="$VENV_PYTHON_DEFAULT"
fi

if ! has_python_deps "$PYTHON_BIN"; then
  if [[ "${APPIUM_UI_BOOTSTRAP_VENV:-0}" == "1" ]]; then
    echo "Bootstrapping local Appium venv at: $VENV_DIR_DEFAULT"
    python3 -m venv "$VENV_DIR_DEFAULT"
    "$VENV_PYTHON_DEFAULT" -m pip install --upgrade pip
    "$VENV_PYTHON_DEFAULT" -m pip install -r "$REQS_FILE"
    PYTHON_BIN="$VENV_PYTHON_DEFAULT"
  fi
fi

if ! has_python_deps "$PYTHON_BIN"; then
  echo "ERROR: Missing Python dependencies for Appium regression."
  echo "Detected python: $PYTHON_BIN"
  echo "Homebrew Python often blocks system installs (PEP 668)."
  echo "Use a local venv:"
  echo "  python3 -m venv \"$VENV_DIR_DEFAULT\""
  echo "  \"$VENV_PYTHON_DEFAULT\" -m pip install -r \"$REQS_FILE\""
  echo "Then rerun using one of:"
  echo "  APPIUM_UI_PYTHON=\"$VENV_PYTHON_DEFAULT\" ./scripts/appium-mac2-ui-regression.sh"
  echo "  APPIUM_UI_BOOTSTRAP_VENV=1 ./scripts/appium-mac2-ui-regression.sh"
  exit 6
fi

if command -v curl >/dev/null 2>&1; then
  if ! curl -sf "${SERVER_URL_DEFAULT}/status" >/dev/null 2>&1; then
    echo "ERROR: Appium server is not reachable at ${SERVER_URL_DEFAULT}."
    echo "Start it and retry:"
    echo "  appium"
    exit 7
  fi
fi

BUNDLE_ID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP_PATH/Contents/Info.plist" 2>/dev/null || true)"
if [[ -z "$BUNDLE_ID" ]]; then
  BUNDLE_ID="com.apc.LocusQ"
fi

echo "=== LocusQ Appium/mac2 Regression ==="
echo "app_path:   $APP_PATH"
echo "bundle_id:  $BUNDLE_ID"
echo "server_url: $SERVER_URL_DEFAULT"
echo "python_bin: $PYTHON_BIN"

"$PYTHON_BIN" "$ROOT_DIR/qa/ui/appium_mac2_regression.py" \
  --app-path "$APP_PATH" \
  --bundle-id "$BUNDLE_ID" \
  --server-url "$SERVER_URL_DEFAULT" \
  --step-timeout-seconds "$STEP_TIMEOUT_DEFAULT" \
  --max-run-seconds "$MAX_RUN_DEFAULT" \
  "$@"
