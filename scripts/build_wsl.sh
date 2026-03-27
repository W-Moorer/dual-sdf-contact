#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-wsl-release}"

cmake --build --preset "$PRESET"
