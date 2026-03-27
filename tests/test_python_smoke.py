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
        ROOT / "build" / "wsl-release" / "bin",
        ROOT / "build" / "wsl-debug" / "bin",
    ]:
        if directory.exists():
            return True
    return False


@pytest.mark.skipif(not _has_any_build(), reason="No built example executables were found.")
@pytest.mark.parametrize("example", ["ex01_nanovdb_hello", "ex04_single_step_contact"])
def test_python_runner_smoke(example: str) -> None:
    subprocess.run(
        [sys.executable, "-m", "baseline.run_example", example],
        cwd=ROOT,
        env=_module_env(),
        check=True,
    )
