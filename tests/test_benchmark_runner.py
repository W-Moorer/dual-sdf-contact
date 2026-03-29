from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[1]


def _module_env() -> dict[str, str]:
    env = os.environ.copy()
    env["PYTHONPATH"] = str(ROOT / "python") + os.pathsep + env.get("PYTHONPATH", "")
    env.setdefault("PYTHONUTF8", "1")
    return env


def _has_any_build() -> bool:
    for directory in [
        ROOT / "build" / "windows-release" / "bin" / "Release",
        ROOT / "build" / "windows-debug" / "bin" / "Debug",
        ROOT / "build" / "wsl-release" / "bin" / "Release",
        ROOT / "build" / "wsl-release" / "bin",
        ROOT / "build" / "wsl-debug" / "bin" / "Debug",
        ROOT / "build" / "wsl-debug" / "bin",
    ]:
        if directory.exists():
            return True
    return False


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_python_benchmark_runner_smoke(tmp_path: Path) -> None:
    output_dir = tmp_path / "benchmark-single"
    subprocess.run(
        [
            sys.executable,
            "-m",
            "baseline.run_benchmark",
            "--config",
            str(ROOT / "configs" / "benchmarks" / "primitive_smoke.json"),
            "--output-dir",
            str(output_dir),
            "--build-config",
            "Release",
        ],
        cwd=ROOT,
        env=_module_env(),
        check=True,
    )
    benchmark_dir = output_dir / "primitive_smoke"
    assert (benchmark_dir / "config_resolved.json").exists()
    assert (benchmark_dir / "summary.csv").exists()
