#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "[bootstrap_wsl] Updating apt indices"
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
  libeigen3-dev
)

OPTIONAL_PACKAGES=(
  libopenvdb-dev
  libtbb-dev
  libfcl-dev
  libhpp-fcl-dev
)

echo "[bootstrap_wsl] Installing core packages"
sudo apt-get install -y "${COMMON_PACKAGES[@]}"

for package in "${OPTIONAL_PACKAGES[@]}"; do
  if apt-cache show "$package" >/dev/null 2>&1; then
    echo "[bootstrap_wsl] Installing optional package: $package"
    sudo apt-get install -y "$package"
  else
    echo "[bootstrap_wsl] Optional package unavailable in current apt sources: $package"
  fi
done

python3 -m pip install --user --upgrade pip
python3 -m pip install --user pytest

echo "[bootstrap_wsl] Done. Source root: $ROOT_DIR"
#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "[bootstrap_wsl] Updating apt indices"
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
  libeigen3-dev
)

OPTIONAL_PACKAGES=(
  libopenvdb-dev
  libtbb-dev
  libfcl-dev
  libhpp-fcl-dev
)

echo "[bootstrap_wsl] Installing core packages"
sudo apt-get install -y "${COMMON_PACKAGES[@]}"

for package in "${OPTIONAL_PACKAGES[@]}"; do
  if apt-cache show "$package" >/dev/null 2>&1; then
    echo "[bootstrap_wsl] Installing optional package: $package"
    sudo apt-get install -y "$package"
  else
    echo "[bootstrap_wsl] Optional package unavailable in current apt sources: $package"
  fi
done

python3 -m pip install --user --upgrade pip
python3 -m pip install --user pytest

echo "[bootstrap_wsl] Done. Source root: $ROOT_DIR"
