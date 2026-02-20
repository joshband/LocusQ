#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${LOCUSQ_BUILD_DIR:-$ROOT_DIR/build_local}"
BUILD_CONFIG="${LOCUSQ_BUILD_CONFIG:-Release}"
BUILD_JOBS="${LOCUSQ_BUILD_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 8)}"
WITH_STANDALONE_INSTALL="${LOCUSQ_INSTALL_STANDALONE:-0}"

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
EOF
  exit 0
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: scripts/build-and-install-mac.sh is macOS-only."
  exit 2
fi

echo "== LocusQ macOS build + install =="
echo "root_dir: $ROOT_DIR"
echo "build_dir: $BUILD_DIR"
echo "build_config: $BUILD_CONFIG"
echo "build_jobs: $BUILD_JOBS"
echo "install_standalone: $WITH_STANDALONE_INSTALL"
echo

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
  -DBUILD_LOCUSQ_QA=ON

cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" \
  --target LocusQ_VST3 LocusQ_AU \
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

if [[ "$WITH_STANDALONE_INSTALL" == "1" ]]; then
  APP_SRC="$BUILD_DIR/LocusQ_artefacts/$BUILD_CONFIG/Standalone/LocusQ.app"
  APP_DST="$HOME/Applications"
  if [[ -d "$APP_SRC" ]]; then
    mkdir -p "$APP_DST"
    rsync -a --delete "$APP_SRC" "$APP_DST/"
  else
    echo "WARN: standalone app not found, skipping: $APP_SRC"
  fi
fi

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

if command -v auval >/dev/null 2>&1; then
  echo "auval_registry:"
  auval -a 2>/dev/null | rg "LocusQ|LcQd|Nfld" || true
  echo
fi

echo "Done. In Reaper, run a full plugin rescan if an old binary is still cached."
