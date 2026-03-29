#!/usr/bin/env bash
set -euo pipefail

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
    echo "[build_wsl] unknown profile: ${PROFILE}"
    echo "[build_wsl] use Debug, Release, wsl-debug, or wsl-release"
    exit 1
    ;;
esac

echo "[build_wsl] preset=${PRESET}"
cmake --build --preset "${PRESET}" "$@"
