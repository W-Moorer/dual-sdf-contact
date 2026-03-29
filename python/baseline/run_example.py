from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path


EXAMPLES = {
    "ex01_nanovdb_hello": "ex01_nanovdb_hello",
    "ex02_hppfcl_distance": "ex02_hppfcl_distance",
    "ex03_dual_sdf_gap": "ex03_dual_sdf_gap",
    "ex04_single_step_contact": "ex04_single_step_contact",
    "ex05_compare_backends": "ex05_compare_backends",
    "ex06_regression_smoke": "ex06_regression_smoke",
}


def project_root() -> Path:
    return Path(__file__).resolve().parents[2]


def candidate_binary_dirs(root: Path, config: str) -> list[Path]:
    config = config or "Release"
    candidates = [
        root / "build" / "windows-release" / "bin" / config,
        root / "build" / "windows-release" / "bin",
        root / "build" / "windows-debug" / "bin" / config,
        root / "build" / "windows-debug" / "bin",
        root / "build" / "wsl-release" / "bin" / config,
        root / "build" / "wsl-release" / "bin",
        root / "build" / "wsl-debug" / "bin" / config,
        root / "build" / "wsl-debug" / "bin",
        root / "build" / "bin" / config,
        root / "build" / "bin",
    ]
    return [path for path in candidates if path.exists()]


def resolve_executable(example: str, build_dir: Path | None, config: str) -> Path:
    executable = EXAMPLES[example] + (".exe" if platform.system() == "Windows" else "")
    search_dirs = []
    if build_dir:
        search_dirs.extend(
            [
                build_dir,
                build_dir / config,
                build_dir / "bin",
                build_dir / "bin" / config,
            ]
        )
    else:
        search_dirs.extend(candidate_binary_dirs(project_root(), config))
    for directory in search_dirs:
        candidate = directory / executable
        if candidate.exists():
            return candidate
    raise FileNotFoundError(
        f"Could not find executable for {example}. Checked: "
        + ", ".join(str(directory / executable) for directory in search_dirs)
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a built C++ example executable.")
    parser.add_argument("example", nargs="?", choices=sorted(EXAMPLES))
    parser.add_argument("--build-dir", type=Path, default=None, help="Directory containing built executables.")
    parser.add_argument("--config", default="Release", help="Build config for multi-config generators.")
    parser.add_argument("--sdf-backend", choices=["analytic", "openvdb", "nanovdb"], default=None)
    parser.add_argument("--reference-backend", choices=["analytic", "hppfcl", "fcl"], default=None)
    parser.add_argument("--solver-backend", choices=["simple", "siconos"], default=None)
    parser.add_argument("--list", action="store_true", help="List known examples and exit.")
    args = parser.parse_args()

    if args.list:
        print("\n".join(sorted(EXAMPLES)))
        return 0
    if args.example is None:
        parser.error("example is required unless --list is used")

    executable = resolve_executable(args.example, args.build_dir, args.config)
    env = os.environ.copy()
    env.setdefault("PYTHONUTF8", "1")
    env.setdefault("BASELINE_OUTPUT_ROOT", str(project_root() / "outputs"))
    command = [str(executable)]
    if args.sdf_backend:
        command.extend(["--sdf-backend", args.sdf_backend])
    if args.reference_backend:
        command.extend(["--reference-backend", args.reference_backend])
    if args.solver_backend:
        command.extend(["--solver-backend", args.solver_backend])
    completed = subprocess.run(command, cwd=project_root(), env=env, check=False)
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
