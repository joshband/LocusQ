#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${LOCUSQ_BUILD_DIR:-$ROOT_DIR/build_local}"
BUILD_CONFIG="${LOCUSQ_BUILD_CONFIG:-Release}"
BUILD_JOBS="${LOCUSQ_BUILD_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 8)}"
WITH_STANDALONE_INSTALL="${LOCUSQ_INSTALL_STANDALONE:-0}"
REFRESH_AU_CACHE="${LOCUSQ_REFRESH_AU_CACHE:-1}"
REFRESH_REAPER_CACHE="${LOCUSQ_REFRESH_REAPER_CACHE:-1}"
REAPER_AUTO_QUIT="${LOCUSQ_REAPER_AUTO_QUIT:-1}"
REAPER_FORCE_KILL="${LOCUSQ_REAPER_FORCE_KILL:-0}"
REAPER_RELAUNCH="${LOCUSQ_REAPER_RELAUNCH:-0}"
CLEAR_QUARANTINE="${LOCUSQ_CLEAR_QUARANTINE:-1}"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
REAPER_WAS_RUNNING=0

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  cat <<'EOF'
Usage: scripts/build-and-install-mac.sh

Builds LocusQ on macOS and installs plugin bundles to user plugin folders:
  ~/Library/Audio/Plug-Ins/VST3/LocusQ.vst3
  ~/Library/Audio/Plug-Ins/Components/LocusQ.component

Environment overrides:
  LOCUSQ_BUILD_DIR            CMake build directory (default: build_local)
  LOCUSQ_BUILD_CONFIG         Build config (default: Release)
  LOCUSQ_BUILD_JOBS           Parallel jobs (default: hw.ncpu)
  LOCUSQ_INSTALL_STANDALONE   If 1, also copy LocusQ.app to ~/Applications
  LOCUSQ_REFRESH_AU_CACHE     If 1, refresh AU registrar cache (default: 1)
  LOCUSQ_REFRESH_REAPER_CACHE If 1, remove LocusQ entries from REAPER plugin caches (default: 1)
  LOCUSQ_REAPER_AUTO_QUIT     If 1, request REAPER quit before install (default: 1)
  LOCUSQ_REAPER_FORCE_KILL    If 1, force-kill REAPER if it ignores quit (default: 0)
  LOCUSQ_REAPER_RELAUNCH      If 1, relaunch REAPER when install completes (default: 0)
  LOCUSQ_CLEAR_QUARANTINE     If 1, clear quarantine xattr on installed bundles (default: 1)
EOF
  exit 0
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: scripts/build-and-install-mac.sh is macOS-only."
  exit 2
fi

wait_for_reaper_exit() {
  local timeout_seconds="${1:-15}"
  local deadline=$((SECONDS + timeout_seconds))
  while pgrep -ix reaper >/dev/null 2>&1 && [[ $SECONDS -lt $deadline ]]; do
    sleep 1
  done
}

maybe_stop_reaper_before_install() {
  if ! pgrep -ix reaper >/dev/null 2>&1; then
    return
  fi

  REAPER_WAS_RUNNING=1

  if [[ "$REAPER_AUTO_QUIT" == "1" ]]; then
    echo "reaper: running -> requesting graceful quit"
    osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
    wait_for_reaper_exit 20
  fi

  if pgrep -ix reaper >/dev/null 2>&1 && [[ "$REAPER_FORCE_KILL" == "1" ]]; then
    echo "reaper: still running -> forcing kill"
    pkill -ix reaper >/dev/null 2>&1 || true
    wait_for_reaper_exit 10
  fi

  if pgrep -ix reaper >/dev/null 2>&1; then
    echo "WARN: REAPER is still running; it may keep an old plugin binary loaded in memory."
    echo "      Set LOCUSQ_REAPER_FORCE_KILL=1 if you want the script to enforce restart."
  fi
}

refresh_au_cache() {
  if [[ "$REFRESH_AU_CACHE" != "1" ]]; then
    return
  fi

  if killall AudioComponentRegistrar >/dev/null 2>&1; then
    echo "au_cache: AudioComponentRegistrar restarted"
  else
    echo "au_cache: AudioComponentRegistrar was not running"
  fi
}

prune_reaper_cache_file() {
  local file="$1"
  local pattern='LocusQ|LcQd|Nfld'

  [[ -f "$file" ]] || return

  if ! rg -q "$pattern" "$file"; then
    return
  fi

  local backup="${file}.bak.${TIMESTAMP}"
  local tmp="${file}.tmp.$$"

  cp "$file" "$backup"
  rg -v "$pattern" "$file" >"$tmp" || true
  mv "$tmp" "$file"

  echo "reaper_cache_pruned: $file"
  echo "reaper_cache_backup: $backup"
}

refresh_reaper_cache() {
  if [[ "$REFRESH_REAPER_CACHE" != "1" ]]; then
    return
  fi

  local reaper_dir="$HOME/Library/Application Support/REAPER"
  if [[ ! -d "$reaper_dir" ]]; then
    echo "reaper_cache: REAPER config dir not found, skipping"
    return
  fi

  shopt -s nullglob
  local files=(
    "$reaper_dir"/reaper-vstplugins*.ini
    "$reaper_dir"/reaper-auplugins*.ini
    "$reaper_dir"/reaper-recentfx.ini
    "$reaper_dir"/reaper-fxtags.ini
  )
  shopt -u nullglob

  if [[ ${#files[@]} -eq 0 ]]; then
    echo "reaper_cache: no cache files found, skipping"
    return
  fi

  for file in "${files[@]}"; do
    prune_reaper_cache_file "$file"
  done
}

clear_bundle_quarantine() {
  local bundle="$1"
  if [[ "$CLEAR_QUARANTINE" == "1" && -d "$bundle" ]]; then
    xattr -dr com.apple.quarantine "$bundle" >/dev/null 2>&1 || true
  fi
}

verify_binary_match() {
  local src="$1"
  local dst="$2"
  local label="$3"

  if [[ ! -f "$src" || ! -f "$dst" ]]; then
    echo "WARN: verification skipped for $label (missing file)"
    return
  fi

  local src_sha
  local dst_sha
  src_sha="$(shasum -a 256 "$src" | awk '{print $1}')"
  dst_sha="$(shasum -a 256 "$dst" | awk '{print $1}')"

  if [[ "$src_sha" != "$dst_sha" ]]; then
    echo "ERROR: installed $label binary does not match build artifact"
    echo "  src: $src_sha"
    echo "  dst: $dst_sha"
    exit 4
  fi
}

maybe_stop_reaper_before_install

echo "== LocusQ macOS build + install =="
echo "root_dir: $ROOT_DIR"
echo "build_dir: $BUILD_DIR"
echo "build_config: $BUILD_CONFIG"
echo "build_jobs: $BUILD_JOBS"
echo "install_standalone: $WITH_STANDALONE_INSTALL"
echo "refresh_au_cache: $REFRESH_AU_CACHE"
echo "refresh_reaper_cache: $REFRESH_REAPER_CACHE"
echo "reaper_auto_quit: $REAPER_AUTO_QUIT"
echo "reaper_force_kill: $REAPER_FORCE_KILL"
echo "reaper_relaunch: $REAPER_RELAUNCH"
echo

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
  -DBUILD_LOCUSQ_QA=ON

BUILD_TARGETS=(LocusQ_VST3 LocusQ_AU)
if [[ "$WITH_STANDALONE_INSTALL" == "1" ]]; then
  BUILD_TARGETS+=(LocusQ_Standalone)
fi

cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" \
  --target "${BUILD_TARGETS[@]}" \
  -j "$BUILD_JOBS"

VST3_SRC="$BUILD_DIR/LocusQ_artefacts/$BUILD_CONFIG/VST3/LocusQ.vst3"
AU_SRC="$BUILD_DIR/LocusQ_artefacts/$BUILD_CONFIG/AU/LocusQ.component"

if [[ ! -d "$VST3_SRC" ]]; then
  echo "ERROR: missing build artifact: $VST3_SRC"
  exit 3
fi

if [[ ! -d "$AU_SRC" ]]; then
  echo "ERROR: missing build artifact: $AU_SRC"
  exit 3
fi

VST3_DST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DST="$HOME/Library/Audio/Plug-Ins/Components"

mkdir -p "$VST3_DST" "$AU_DST"
rsync -a --delete "$VST3_SRC" "$VST3_DST/"
rsync -a --delete "$AU_SRC" "$AU_DST/"
clear_bundle_quarantine "$VST3_DST/LocusQ.vst3"
clear_bundle_quarantine "$AU_DST/LocusQ.component"

if [[ "$WITH_STANDALONE_INSTALL" == "1" ]]; then
  APP_SRC="$BUILD_DIR/LocusQ_artefacts/$BUILD_CONFIG/Standalone/LocusQ.app"
  APP_DST="$HOME/Applications"
  if [[ -d "$APP_SRC" ]]; then
    mkdir -p "$APP_DST"
    rsync -a --delete "$APP_SRC" "$APP_DST/"
    clear_bundle_quarantine "$APP_DST/LocusQ.app"
  else
    echo "WARN: standalone app not found, skipping: $APP_SRC"
  fi
fi

refresh_au_cache
refresh_reaper_cache

print_binary_details() {
  local path="$1"
  if [[ -f "$path" ]]; then
    echo "path=$path"
    stat -f "mtime=%Sm size=%z" -t "%Y-%m-%d %H:%M:%S" "$path"
    shasum -a 256 "$path" | awk '{print "sha256=" $1}'
  else
    echo "path=$path"
    echo "missing"
  fi
  echo
}

print_binary_details "$VST3_SRC/Contents/MacOS/LocusQ"
print_binary_details "$AU_SRC/Contents/MacOS/LocusQ"
print_binary_details "$VST3_DST/LocusQ.vst3/Contents/MacOS/LocusQ"
print_binary_details "$AU_DST/LocusQ.component/Contents/MacOS/LocusQ"

verify_binary_match "$VST3_SRC/Contents/MacOS/LocusQ" "$VST3_DST/LocusQ.vst3/Contents/MacOS/LocusQ" "VST3"
verify_binary_match "$AU_SRC/Contents/MacOS/LocusQ" "$AU_DST/LocusQ.component/Contents/MacOS/LocusQ" "AU"

if command -v auval >/dev/null 2>&1; then
  echo "auval_registry:"
  auval -a 2>/dev/null | rg "LocusQ|LcQd|Nfld" || true
  echo
fi

if [[ "$REAPER_WAS_RUNNING" == "1" && "$REAPER_RELAUNCH" == "1" ]]; then
  if ! pgrep -ix reaper >/dev/null 2>&1; then
    open -a REAPER >/dev/null 2>&1 || true
  fi
fi

echo "Done. Reaper cache entries for LocusQ were pruned; run a plugin rescan in REAPER if needed."
