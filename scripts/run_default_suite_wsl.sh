#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-Release}"
shift || true

case "${PROFILE,,}" in
  debug|wsl-debug)
    PRESET="wsl-debug"
    BUILD_CONFIG="Debug"
    ;;
  release|wsl-release)
    PRESET="wsl-release"
    BUILD_CONFIG="Release"
    ;;
  *)
    echo "[run_default_suite_wsl] unknown profile: ${PROFILE}"
    echo "[run_default_suite_wsl] use Debug, Release, wsl-debug, or wsl-release"
    exit 1
    ;;
esac

export PYTHONPATH="${ROOT_DIR}/python${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONUTF8=1
export BASELINE_OUTPUT_ROOT="${ROOT_DIR}/outputs"

echo "[run_default_suite_wsl] preset=${PRESET} suite=default"
cmake --build --preset "${PRESET}" "$@"
python3 -m baseline.run_benchmark --suite default --build-config "${BUILD_CONFIG}"
