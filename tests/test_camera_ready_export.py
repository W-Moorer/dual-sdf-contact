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


def _run_module(module: str, args: list[str]) -> None:
    subprocess.run([sys.executable, "-m", module, *args], cwd=ROOT, env=_module_env(), check=True)


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_camera_ready_tables_generated(tmp_path: Path) -> None:
    suite_root = tmp_path / "paper-extended"
    export_root = tmp_path / "camera-ready"
    _run_module("baseline.run_benchmark", ["--suite", "paper_extended", "--output-dir", str(suite_root)])
    _run_module("baseline.export_camera_ready_tables", [str(suite_root), "--output-dir", str(export_root)])

    assert (export_root / "camera_ready_table_accuracy.csv").exists()
    assert (export_root / "camera_ready_table_efficiency.md").exists()
    assert (export_root / "camera_ready_table_ablation.csv").exists()
    assert (export_root / "camera_ready_table_main_results.csv").exists()
    assert (export_root / "camera_ready_table_appendix.md").exists()
    assert (export_root / "figure_manifest.json").exists()


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_paper_extended_suite_smoke(tmp_path: Path) -> None:
    suite_root = tmp_path / "paper-extended-suite"
    _run_module("baseline.run_benchmark", ["--suite", "paper_extended", "--output-dir", str(suite_root)])

    assert (suite_root / "aggregate_summary.csv").exists()
    assert (suite_root / "plot_normal_vs_orientation.csv").exists()
    assert (suite_root / "plot_runtime_vs_orientation.csv").exists()
