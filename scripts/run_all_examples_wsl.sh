#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-Release}"
shift || true

case "${PROFILE,,}" in
  debug)
    PRESET="wsl-debug"
    BUILD_DIR="${ROOT_DIR}/build/wsl-debug"
    CONFIG="Debug"
    ;;
  release)
    PRESET="wsl-release"
    BUILD_DIR="${ROOT_DIR}/build/wsl-release"
    CONFIG="Release"
    ;;
  wsl-debug)
    PRESET="wsl-debug"
    BUILD_DIR="${ROOT_DIR}/build/wsl-debug"
    CONFIG="Debug"
    ;;
  wsl-release)
    PRESET="wsl-release"
    BUILD_DIR="${ROOT_DIR}/build/wsl-release"
    CONFIG="Release"
    ;;
  *)
    echo "[run_all_examples_wsl] unknown profile: ${PROFILE}"
    echo "[run_all_examples_wsl] use Debug, Release, wsl-debug, or wsl-release"
    exit 1
    ;;
esac

export PYTHONPATH="${ROOT_DIR}/python${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONUTF8=1
export BASELINE_OUTPUT_ROOT="${ROOT_DIR}/outputs"

echo "[run_all_examples_wsl] preset=${PRESET}"
cmake --build --preset "${PRESET}" "$@"

examples=(
  ex01_nanovdb_hello
  ex02_hppfcl_distance
  ex03_dual_sdf_gap
  ex04_single_step_contact
  ex05_compare_backends
  ex06_regression_smoke
)

for example in "${examples[@]}"; do
  echo "[run_all_examples_wsl] ${example}"
  python3 -m baseline.run_example "${example}" --config "${CONFIG}"
done

echo "[run_all_examples_wsl] ctest"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "[run_all_examples_wsl] pytest"
python3 -m pytest "${ROOT_DIR}/tests/test_python_smoke.py" -q
