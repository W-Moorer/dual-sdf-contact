#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-Release}"
shift || true

case "${PROFILE,,}" in
  debug)
    PRESET="wsl-debug"
    ;;
  release)
    PRESET="wsl-release"
    ;;
  wsl-debug|wsl-release)
    PRESET="${PROFILE,,}"
    ;;
  *)
    echo "[configure_wsl] unknown profile: ${PROFILE}"
    echo "[configure_wsl] use Debug, Release, wsl-debug, or wsl-release"
    exit 1
    ;;
esac

echo "[configure_wsl] preset=${PRESET}"
cmake --preset "${PRESET}" -S "${ROOT_DIR}" "$@"
