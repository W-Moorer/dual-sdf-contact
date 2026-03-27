#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-wsl-release}"
shift || true

cmake --build --preset "$PRESET" "$@"
