#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${1:-}"
OUTPUT_DIR="${2:-}"

export PYTHONPATH="${ROOT_DIR}/python${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONUTF8=1
export BASELINE_OUTPUT_ROOT="${ROOT_DIR}/outputs"

if [[ -z "${SOURCE_DIR}" ]]; then
  SOURCE_DIR="$(ls -1dt "${ROOT_DIR}/outputs/benchmark_suites"/paper_extended_* 2>/dev/null | head -n1 || true)"
fi

if [[ -z "${SOURCE_DIR}" ]]; then
  echo "[export_camera_ready_tables_wsl] no paper_extended suite output found"
  exit 1
fi

if [[ -z "${OUTPUT_DIR}" ]]; then
  OUTPUT_DIR="${SOURCE_DIR}"
fi

echo "[export_camera_ready_tables_wsl] source=${SOURCE_DIR} output=${OUTPUT_DIR}"
python3 -m baseline.export_camera_ready_tables "${SOURCE_DIR}" --output-dir "${OUTPUT_DIR}"
