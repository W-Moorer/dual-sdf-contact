#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN_SUITE_DIR="${1:-}"
APPENDIX_SUITE_DIR="${2:-}"
OUTPUT_DIR="${3:-}"
DOCS_OUTPUT="${4:-${ROOT_DIR}/docs/results_summary.md}"

export PYTHONPATH="${ROOT_DIR}/python${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONUTF8=1
export BASELINE_OUTPUT_ROOT="${ROOT_DIR}/outputs"

if [[ -z "${MAIN_SUITE_DIR}" ]]; then
  MAIN_SUITE_DIR="$(ls -1dt "${ROOT_DIR}/outputs/benchmark_suites"/paper_minimal_* 2>/dev/null | head -n1 || true)"
fi

if [[ -z "${APPENDIX_SUITE_DIR}" ]]; then
  APPENDIX_SUITE_DIR="$(ls -1dt "${ROOT_DIR}/outputs/benchmark_suites"/paper_extended_* 2>/dev/null | head -n1 || true)"
fi

if [[ -z "${MAIN_SUITE_DIR}" || -z "${APPENDIX_SUITE_DIR}" ]]; then
  echo "[export_paper_assets_wsl] missing paper_minimal or paper_extended suite output"
  exit 1
fi

if [[ -z "${OUTPUT_DIR}" ]]; then
  OUTPUT_DIR="${APPENDIX_SUITE_DIR}"
fi

echo "[export_paper_assets_wsl] main=${MAIN_SUITE_DIR} appendix=${APPENDIX_SUITE_DIR} output=${OUTPUT_DIR}"
python3 -m baseline.export_paper_assets \
  --main-suite-dir "${MAIN_SUITE_DIR}" \
  --appendix-suite-dir "${APPENDIX_SUITE_DIR}" \
  --output-dir "${OUTPUT_DIR}" \
  --docs-output "${DOCS_OUTPUT}"
