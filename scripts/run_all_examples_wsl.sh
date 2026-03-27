#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build/wsl-release}"

echo "[run_all_examples_wsl] Intended for a future WSL2 Ubuntu host. It is not validated on the current Windows-only machine."

export PYTHONPATH="$ROOT_DIR/python${PYTHONPATH:+:$PYTHONPATH}"
export PYTHONUTF8=1

examples=(
  ex01_nanovdb_hello
  ex02_hppfcl_distance
  ex03_dual_sdf_gap
  ex04_single_step_contact
  ex05_compare_backends
  ex06_regression_smoke
)

for example in "${examples[@]}"; do
  python3 -m baseline.run_example "$example" --build-dir "$BUILD_DIR/bin"
done

ctest --test-dir "$BUILD_DIR" --output-on-failure
python3 -m pytest "$ROOT_DIR/tests/test_python_smoke.py"
