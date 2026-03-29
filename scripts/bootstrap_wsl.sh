#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ ! -f /etc/os-release ]]; then
  echo "[bootstrap_wsl] /etc/os-release not found; expected Ubuntu 22.04/24.04 under WSL2."
  exit 1
fi

. /etc/os-release
echo "[bootstrap_wsl] distro=${PRETTY_NAME}"
echo "[bootstrap_wsl] root=${ROOT_DIR}"

if ! grep -qi microsoft /proc/version 2>/dev/null; then
  echo "[bootstrap_wsl] warning: this does not look like WSL2, continuing anyway."
fi

sudo apt-get update

COMMON_PACKAGES=(
  build-essential
  cmake
  ninja-build
  git
  pkg-config
  python3
  python3-pip
  python3-venv
  python3-pytest
  libeigen3-dev
  libfcl-dev
)

OPTIONAL_PACKAGES=(
  libsiconos-numerics-dev
)

echo "[bootstrap_wsl] installing common packages"
sudo apt-get install -y "${COMMON_PACKAGES[@]}"

for package in "${OPTIONAL_PACKAGES[@]}"; do
  if apt-cache show "$package" >/dev/null 2>&1; then
    echo "[bootstrap_wsl] attempting optional package: $package"
    sudo apt-get install -y "$package" || echo "[bootstrap_wsl] optional package install failed: $package"
  fi
done

OPENVDB_LOCAL_ROOT="${ROOT_DIR}/third_party/_deps/openvdb"
mkdir -p "${ROOT_DIR}/third_party/_apt/openvdb_deb" "${OPENVDB_LOCAL_ROOT}"

if apt-cache show libopenvdb-dev >/dev/null 2>&1; then
  echo "[bootstrap_wsl] attempting system OpenVDB install"
  if sudo apt-get install -y libopenvdb-dev; then
    echo "[bootstrap_wsl] system libopenvdb-dev installed"
  else
    echo "[bootstrap_wsl] libopenvdb-dev install failed; extracting headers locally instead"
    (
      cd "${ROOT_DIR}/third_party/_apt/openvdb_deb"
      apt-get download libopenvdb-dev
      rm -rf "${OPENVDB_LOCAL_ROOT}"
      mkdir -p "${OPENVDB_LOCAL_ROOT}"
      dpkg-deb -x libopenvdb-dev_*.deb "${OPENVDB_LOCAL_ROOT}"
    )
  fi
else
  echo "[bootstrap_wsl] libopenvdb-dev unavailable in current apt sources"
fi

if apt-cache show libhpp-fcl-dev >/dev/null 2>&1; then
  echo "[bootstrap_wsl] attempting optional hpp-fcl install"
  sudo apt-get install -y libhpp-fcl-dev || echo "[bootstrap_wsl] hpp-fcl install failed; FCL will remain the primary reference backend"
else
  echo "[bootstrap_wsl] libhpp-fcl-dev unavailable; FCL stays primary"
fi

echo "[bootstrap_wsl] toolchain"
command -v gcc >/dev/null 2>&1 && gcc --version | head -n 1
command -v clang >/dev/null 2>&1 && clang --version | head -n 1 || true
command -v cmake >/dev/null 2>&1 && cmake --version | head -n 1
command -v ninja >/dev/null 2>&1 && ninja --version
command -v python3 >/dev/null 2>&1 && python3 --version
command -v pytest >/dev/null 2>&1 && pytest --version || true

echo "[bootstrap_wsl] complete"
