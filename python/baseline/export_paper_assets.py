from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from .experiment_freeze import build_freeze_manifest
from .run_example import project_root


MAIN_TEXT_BENCHMARKS = [
    "primitive_smoke",
    "resolution_sweep",
    "mesh_smoke",
    "mesh_resolution_sweep",
    "mesh_nonconvex_smoke",
    "mesh_orientation_sweep",
]

APPENDIX_BENCHMARKS = [
    "gap_sweep",
    "orientation_sweep",
    "mesh_gap_sweep",
    "mesh_nonconvex_smoke_2",
    "mesh_gap_sweep_2",
    "mesh_orientation_sweep_2",
]


def _timestamp_utc() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _write_markdown(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _suite_label(path: Path) -> str:
    return path.name


def _manifest_section(suite_dir: Path, suite_name: str, benchmarks: list[str], *, appendix: bool) -> dict[str, Any]:
    ex05_root = suite_dir / "ex05_compare_backends"
    table_prefix = "appendix" if appendix else "main"
    return {
        "suite_name": suite_name,
        "suite_dir": str(suite_dir),
        "benchmarks": benchmarks,
        "tables": {
            "accuracy": str(suite_dir / "camera_ready_table_accuracy.csv"),
            "efficiency": str(suite_dir / "camera_ready_table_efficiency.csv"),
            "ablation": str(suite_dir / "camera_ready_table_ablation.csv"),
            "focused": str(suite_dir / f"camera_ready_table_{'appendix' if appendix else 'main_results'}.csv"),
        },
        "plots": {
            "gap_vs_resolution": str(suite_dir / "plot_gap_vs_resolution.csv"),
            "runtime_vs_resolution": str(suite_dir / "plot_runtime_vs_resolution.csv"),
            "symmetry_vs_bandwidth": str(suite_dir / "plot_symmetry_vs_bandwidth.csv"),
            "normal_vs_orientation": str(suite_dir / "plot_normal_vs_orientation.csv"),
            "runtime_vs_orientation": str(suite_dir / "plot_runtime_vs_orientation.csv"),
        },
        "benchmark_dirs": {name: str(ex05_root / name) for name in benchmarks},
        "notes": [
            "Use camera_ready_table_main_results.csv for the main text summary table." if not appendix
            else "Use camera_ready_table_appendix.csv for supplementary and warning-heavy rows.",
            "Rows with reference_quality in {low, grazing-caution} are best discussed in the appendix.",
        ],
    }


def export_paper_assets(
    *,
    main_suite_dir: Path,
    appendix_suite_dir: Path,
    output_dir: Path,
    docs_output: Path,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    main_suite_name = _suite_label(main_suite_dir)
    appendix_suite_name = _suite_label(appendix_suite_dir)
    main_section = _manifest_section(main_suite_dir, main_suite_name, MAIN_TEXT_BENCHMARKS, appendix=False)
    appendix_section = _manifest_section(appendix_suite_dir, appendix_suite_name, APPENDIX_BENCHMARKS, appendix=True)

    paper_assets_manifest = {
        "generated_at_utc": _timestamp_utc(),
        "freeze": build_freeze_manifest(
            suite_name="paper_minimal",
            run_name=main_suite_name,
            benchmark_dirs=[main_suite_dir / "ex05_compare_backends" / name for name in MAIN_TEXT_BENCHMARKS],
        ),
        "main_text": {
            **main_section,
            "figures": {
                "figure_1": main_section["plots"]["gap_vs_resolution"],
                "figure_2": main_section["plots"]["runtime_vs_resolution"],
                "figure_3": main_section["plots"]["normal_vs_orientation"],
                "figure_4": main_section["plots"]["symmetry_vs_bandwidth"],
            },
            "tables_for_paper": {
                "table_1": main_section["tables"]["focused"],
                "table_2": main_section["tables"]["efficiency"],
                "table_3": main_section["tables"]["ablation"],
            },
        },
        "appendix": {
            **appendix_section,
            "figures": {
                "appendix_figure_a": appendix_section["plots"]["runtime_vs_orientation"],
                "appendix_figure_b": appendix_section["plots"]["symmetry_vs_bandwidth"],
            },
            "tables_for_appendix": {
                "appendix_table_a": appendix_section["tables"]["focused"],
                "appendix_table_b": appendix_section["tables"]["accuracy"],
                "appendix_table_c": appendix_section["tables"]["ablation"],
            },
        },
    }
    appendix_assets_manifest = {
        "generated_at_utc": _timestamp_utc(),
        "appendix": paper_assets_manifest["appendix"],
    }

    _write_json(output_dir / "paper_assets_manifest.json", paper_assets_manifest)
    _write_json(output_dir / "appendix_assets_manifest.json", appendix_assets_manifest)

    summary = f"""# Results Summary

## Main Text Recommendation

- Recommended suite: `{main_suite_name}`
- Recommended benchmarks: {", ".join(f"`{name}`" for name in MAIN_TEXT_BENCHMARKS)}
- Main table draft: `{(output_dir / "paper_assets_manifest.json").name}` with `table_1/table_2/table_3`
- Direct table files:
  - `{main_suite_dir / "camera_ready_table_main_results.csv"}`
  - `{main_suite_dir / "camera_ready_table_efficiency.csv"}`
  - `{main_suite_dir / "camera_ready_table_ablation.csv"}`
- Direct plot-ready CSV:
  - `{main_suite_dir / "plot_gap_vs_resolution.csv"}`
  - `{main_suite_dir / "plot_runtime_vs_resolution.csv"}`
  - `{main_suite_dir / "plot_normal_vs_orientation.csv"}`
  - `{main_suite_dir / "plot_symmetry_vs_bandwidth.csv"}`

## Appendix Recommendation

- Recommended suite: `{appendix_suite_name}`
- Recommended benchmarks: {", ".join(f"`{name}`" for name in APPENDIX_BENCHMARKS)}
- Supplementary table files:
  - `{appendix_suite_dir / "camera_ready_table_appendix.csv"}`
  - `{appendix_suite_dir / "camera_ready_table_accuracy.csv"}`
  - `{appendix_suite_dir / "camera_ready_table_ablation.csv"}`
- Supplementary plot-ready CSV:
  - `{appendix_suite_dir / "plot_runtime_vs_orientation.csv"}`
  - `{appendix_suite_dir / "plot_symmetry_vs_bandwidth.csv"}`
- Grazing or low-confidence rows should be discussed using:
  - `reference_quality`
  - `reference_diagnostic_label`
  - `reference_warning_count`
  - `reference_grazing_count`
  - `reference_normal_alignment_residual_mean`

## Asset Files

- Paper assets manifest: `{output_dir / "paper_assets_manifest.json"}`
- Appendix assets manifest: `{output_dir / "appendix_assets_manifest.json"}`
- Freeze manifest: `{appendix_suite_dir / "freeze_manifest.json"}`
"""
    _write_markdown(docs_output, summary)


def main() -> int:
    parser = argparse.ArgumentParser(description="Export a thin paper asset bundle from suite outputs.")
    parser.add_argument("--main-suite-dir", type=Path, required=True)
    parser.add_argument("--appendix-suite-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--docs-output", type=Path, default=project_root() / "docs" / "results_summary.md")
    args = parser.parse_args()

    export_paper_assets(
        main_suite_dir=args.main_suite_dir,
        appendix_suite_dir=args.appendix_suite_dir,
        output_dir=args.output_dir,
        docs_output=args.docs_output,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
