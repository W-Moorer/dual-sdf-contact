#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${1:-wsl-release}"

cmake --preset "$PRESET" -S "$ROOT_DIR"
