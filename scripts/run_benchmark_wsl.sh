#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-Release}"
BENCHMARK_CONFIG="${2:-${ROOT_DIR}/configs/benchmarks/primitive_smoke.json}"
shift 2 || true

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
    echo "[run_benchmark_wsl] unknown profile: ${PROFILE}"
    echo "[run_benchmark_wsl] use Debug, Release, wsl-debug, or wsl-release"
    exit 1
    ;;
esac

export PYTHONPATH="${ROOT_DIR}/python${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONUTF8=1
export BASELINE_OUTPUT_ROOT="${ROOT_DIR}/outputs"

echo "[run_benchmark_wsl] preset=${PRESET} benchmark_config=${BENCHMARK_CONFIG}"
cmake --build --preset "${PRESET}" "$@"
python3 -m baseline.run_benchmark --config "${BENCHMARK_CONFIG}" --build-config "${BUILD_CONFIG}"
