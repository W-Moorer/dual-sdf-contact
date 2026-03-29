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


def _run_benchmark(args: list[str], cwd: Path) -> None:
    subprocess.run(
        [sys.executable, "-m", "baseline.run_benchmark", *args],
        cwd=cwd,
        env=_module_env(),
        check=True,
    )


def _run_postprocess(args: list[str], cwd: Path) -> None:
    subprocess.run(
        [sys.executable, "-m", "baseline.postprocess_benchmark", *args],
        cwd=cwd,
        env=_module_env(),
        check=True,
    )


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_postprocess_generates_paper_tables(tmp_path: Path) -> None:
    benchmark_root = tmp_path / "benchmarks"
    post_root = tmp_path / "postprocess"
    _run_benchmark(
        ["--config", str(ROOT / "configs" / "benchmarks" / "resolution_sweep.json"), "--output-dir", str(benchmark_root)],
        ROOT,
    )
    _run_benchmark(
        ["--config", str(ROOT / "configs" / "benchmarks" / "mesh_resolution_sweep.json"), "--output-dir", str(benchmark_root)],
        ROOT,
    )
    _run_postprocess(
        [
            str(benchmark_root / "resolution_sweep"),
            str(benchmark_root / "mesh_resolution_sweep"),
            "--output-dir",
            str(post_root),
        ],
        ROOT,
    )

    assert (post_root / "paper_table_accuracy.csv").exists()
    assert (post_root / "paper_table_efficiency.csv").exists()
    assert (post_root / "paper_table_ablation.md").exists()


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_plot_ready_csv_generated(tmp_path: Path) -> None:
    suite_root = tmp_path / "paper_minimal"
    _run_benchmark(["--suite", "paper_minimal", "--output-dir", str(suite_root)], ROOT)

    plot_gap = suite_root / "plot_gap_vs_resolution.csv"
    plot_runtime = suite_root / "plot_runtime_vs_resolution.csv"
    plot_symmetry = suite_root / "plot_symmetry_vs_bandwidth.csv"

    assert plot_gap.exists()
    assert plot_runtime.exists()
    assert plot_symmetry.exists()
    assert "voxel_size" in plot_gap.read_text(encoding="utf-8")
    assert "runtime_total_us_mean" in plot_runtime.read_text(encoding="utf-8")


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_paper_minimal_suite_smoke(tmp_path: Path) -> None:
    suite_root = tmp_path / "paper_minimal_suite"
    _run_benchmark(["--suite", "paper_minimal", "--output-dir", str(suite_root)], ROOT)

    assert (suite_root / "aggregate_summary.csv").exists()
    assert (suite_root / "paper_table_accuracy.md").exists()
    assert (suite_root / "plot_symmetry_vs_bandwidth.csv").exists()
