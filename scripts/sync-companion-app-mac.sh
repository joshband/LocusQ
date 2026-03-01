#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPANION_DIR="$ROOT_DIR/companion"

APP_PATH="${LOCUSQ_COMPANION_APP_PATH:-}"
if [[ -z "$APP_PATH" ]]; then
  if [[ -d "/Applications/LocusQ Headtrack Companion.app" ]]; then
    APP_PATH="/Applications/LocusQ Headtrack Companion.app"
  elif [[ -d "$HOME/Applications/LocusQ Headtrack Companion.app" ]]; then
    APP_PATH="$HOME/Applications/LocusQ Headtrack Companion.app"
  else
    APP_PATH="/Applications/LocusQ Headtrack Companion.app"
  fi
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: scripts/sync-companion-app-mac.sh is macOS-only."
  exit 2
fi

if [[ ! -d "$COMPANION_DIR" ]]; then
  echo "ERROR: missing companion project: $COMPANION_DIR"
  exit 3
fi

if [[ ! -d "$APP_PATH" ]]; then
  echo "ERROR: companion app bundle not found: $APP_PATH"
  echo "Set LOCUSQ_COMPANION_APP_PATH to an existing app bundle path."
  exit 4
fi

echo "== Sync LocusQ Headtrack Companion =="
echo "companion_dir: $COMPANION_DIR"
echo "app_path: $APP_PATH"

echo "Building companion backend..."
(cd "$COMPANION_DIR" && swift build -c release)

COMPANION_BIN="$COMPANION_DIR/.build/release/locusq-headtrack-companion"
if [[ ! -x "$COMPANION_BIN" ]]; then
  COMPANION_BIN="$COMPANION_DIR/.build/arm64-apple-macosx/release/locusq-headtrack-companion"
fi

if [[ ! -x "$COMPANION_BIN" ]]; then
  echo "ERROR: companion backend binary not found after build."
  exit 5
fi

APP_MACOS_DIR="$APP_PATH/Contents/MacOS"
APP_RESOURCES_DIR="$APP_PATH/Contents/Resources"
APP_BACKEND_BIN="$APP_MACOS_DIR/locusq-headtrack-companion"
APP_MONITOR_BIN="$APP_MACOS_DIR/locusq-headtrack-monitor"

if [[ ! -x "$APP_MONITOR_BIN" ]]; then
  echo "WARN: monitor launcher not found at expected path: $APP_MONITOR_BIN"
fi

mkdir -p "$APP_MACOS_DIR" "$APP_RESOURCES_DIR"
cp -f "$COMPANION_BIN" "$APP_BACKEND_BIN"
chmod +x "$APP_BACKEND_BIN"

THREE_JS_SRC="$ROOT_DIR/Source/ui/public/js/three.min.js"
if [[ -f "$THREE_JS_SRC" ]]; then
  cp -f "$THREE_JS_SRC" "$APP_RESOURCES_DIR/three.min.js"
fi

ICON_SRC="$ROOT_DIR/Resources/LocusQ.icns"
if [[ -f "$ICON_SRC" ]]; then
  cp -f "$ICON_SRC" "$APP_RESOURCES_DIR/LocusQHeadtrackCompanion.icns"
fi

xattr -dr com.apple.quarantine "$APP_PATH" >/dev/null 2>&1 || true

echo "Synced backend + resources."
stat -f "mtime=%Sm size=%z path=%N" -t "%Y-%m-%d %H:%M:%S" "$APP_BACKEND_BIN"
shasum -a 256 "$APP_BACKEND_BIN" | awk '{print "sha256=" $1}'
if [[ -f "$APP_RESOURCES_DIR/LocusQHeadtrackCompanion.icns" ]]; then
  stat -f "mtime=%Sm size=%z path=%N" -t "%Y-%m-%d %H:%M:%S" "$APP_RESOURCES_DIR/LocusQHeadtrackCompanion.icns"
fi

echo "Done."
