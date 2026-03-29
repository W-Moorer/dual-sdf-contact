from __future__ import annotations

import json
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


@pytest.fixture(scope="module")
def paper_assets_workspace(tmp_path_factory: pytest.TempPathFactory) -> dict[str, Path]:
    base = tmp_path_factory.mktemp("paper-assets")
    minimal = base / "paper-minimal"
    extended = base / "paper-extended"
    assets = base / "paper-assets"
    summary = base / "results_summary.md"

    _run_module("baseline.run_benchmark", ["--suite", "paper_minimal", "--output-dir", str(minimal)])
    _run_module("baseline.run_benchmark", ["--suite", "paper_extended", "--output-dir", str(extended)])
    _run_module(
        "baseline.export_paper_assets",
        [
            "--main-suite-dir",
            str(minimal),
            "--appendix-suite-dir",
            str(extended),
            "--output-dir",
            str(assets),
            "--docs-output",
            str(summary),
        ],
    )
    return {
        "minimal": minimal,
        "extended": extended,
        "assets": assets,
        "summary": summary,
    }


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_paper_minimal_suite_outputs_assets(paper_assets_workspace: dict[str, Path]) -> None:
    minimal = paper_assets_workspace["minimal"]
    assert (minimal / "aggregate_summary.csv").exists()
    assert (minimal / "camera_ready_table_main_results.csv").exists()
    assert (minimal / "freeze_manifest.json").exists()
    assert (minimal / "ex05_compare_backends" / "mesh_nonconvex_smoke" / "freeze_manifest.json").exists()
    assert (minimal / "ex05_compare_backends" / "mesh_orientation_sweep" / "summary.csv").exists()


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_paper_extended_suite_outputs_assets(paper_assets_workspace: dict[str, Path]) -> None:
    extended = paper_assets_workspace["extended"]
    assert (extended / "aggregate_summary.csv").exists()
    assert (extended / "camera_ready_table_appendix.csv").exists()
    assert (extended / "figure_manifest.json").exists()
    assert (extended / "freeze_manifest.json").exists()
    assert (extended / "ex05_compare_backends" / "mesh_nonconvex_smoke_2" / "summary.csv").exists()
    assert (extended / "ex05_compare_backends" / "mesh_orientation_sweep_2" / "summary.csv").exists()


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_results_summary_generated(paper_assets_workspace: dict[str, Path]) -> None:
    summary = paper_assets_workspace["summary"]
    assets = paper_assets_workspace["assets"]
    assert summary.exists()
    assert (assets / "paper_assets_manifest.json").exists()
    assert (assets / "appendix_assets_manifest.json").exists()
    content = summary.read_text(encoding="utf-8")
    assert "Main Text Recommendation" in content
    assert "Appendix Recommendation" in content


@pytest.mark.skipif(not _has_any_build(), reason="No built benchmark executable was found.")
def test_main_vs_appendix_mapping_present(paper_assets_workspace: dict[str, Path]) -> None:
    manifest_path = paper_assets_workspace["assets"] / "paper_assets_manifest.json"
    payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    assert "primitive_smoke" in payload["main_text"]["benchmarks"]
    assert "mesh_orientation_sweep" in payload["main_text"]["benchmarks"]
    assert "mesh_orientation_sweep_2" in payload["appendix"]["benchmarks"]
    assert payload["main_text"]["tables_for_paper"]["table_1"].endswith("camera_ready_table_main_results.csv")
