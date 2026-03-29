from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

MAIN_RESULT_BENCHMARKS = {
    "primitive_smoke",
    "resolution_sweep",
    "mesh_smoke",
    "mesh_resolution_sweep",
    "mesh_nonconvex_smoke",
    "mesh_orientation_sweep",
}


def _read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def _write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def _write_markdown(path: Path, title: str, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text(f"# {title}\n\n_No rows found._\n", encoding="utf-8")
        return
    columns = list(rows[0].keys())
    lines = [
        f"# {title}",
        "",
        "| " + " | ".join(columns) + " |",
        "| " + " | ".join(["---"] * len(columns)) + " |",
    ]
    for row in rows:
        lines.append("| " + " | ".join(row.get(column, "") for column in columns) + " |")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _select_and_rename(rows: list[dict[str, str]], mapping: list[tuple[str, str]]) -> list[dict[str, str]]:
    converted: list[dict[str, str]] = []
    for row in rows:
        converted.append({target: row.get(source, "") for source, target in mapping})
    return converted


def _load_table_rows(source_dir: Path, table_name: str) -> list[dict[str, str]]:
    path = source_dir / table_name
    if not path.exists():
        raise FileNotFoundError(f"Missing input table: {path}")
    return _read_csv(path)


def _filter_rows(rows: list[dict[str, str]], benchmark_names: set[str]) -> list[dict[str, str]]:
    return [row for row in rows if row.get("benchmark") in benchmark_names]


def export_camera_ready_tables(source_dir: Path, output_dir: Path) -> None:
    accuracy_rows = _load_table_rows(source_dir, "paper_table_accuracy.csv")
    efficiency_rows = _load_table_rows(source_dir, "paper_table_efficiency.csv")
    ablation_rows = _load_table_rows(source_dir, "paper_table_ablation.csv")

    camera_accuracy = _select_and_rename(
        accuracy_rows,
        [
            ("benchmark_name", "benchmark"),
            ("case_group", "case_group"),
            ("case_name", "case"),
            ("mesh_category", "mesh_category"),
            ("sdf_backend", "sdf_backend"),
            ("reference_backend", "reference_backend"),
            ("reference_quality", "reference_quality"),
            ("reference_diagnostic_label", "reference_diagnostic"),
            ("voxel_size", "voxel_size"),
            ("narrow_band_half_width", "band_half_width"),
            ("absolute_gap_error_mean", "abs_gap_error"),
            ("normal_angle_error_deg_mean", "normal_angle_deg"),
            ("symmetry_residual_mean", "symmetry_residual"),
        ],
    )
    camera_efficiency = _select_and_rename(
        efficiency_rows,
        [
            ("benchmark_name", "benchmark"),
            ("case_group", "case_group"),
            ("case_name", "case"),
            ("mesh_category", "mesh_category"),
            ("sdf_backend", "sdf_backend"),
            ("reference_backend", "reference_backend"),
            ("reference_quality", "reference_quality"),
            ("reference_diagnostic_label", "reference_diagnostic"),
            ("voxel_size", "voxel_size"),
            ("narrow_band_half_width", "band_half_width"),
            ("reference_runtime_us_mean", "reference_mean_us"),
            ("dual_sdf_runtime_us_mean", "dual_sdf_mean_us"),
            ("runtime_total_us_mean", "total_mean_us"),
            ("runtime_total_us_p95", "total_p95_us"),
        ],
    )
    camera_ablation = _select_and_rename(
        ablation_rows,
        [
            ("benchmark_name", "benchmark"),
            ("case_group", "case_group"),
            ("sweep_family", "sweep_family"),
            ("case_name", "case"),
            ("mesh_category", "mesh_category"),
            ("reference_quality", "reference_quality"),
            ("reference_diagnostic_label", "reference_diagnostic"),
            ("voxel_size", "voxel_size"),
            ("narrow_band_half_width", "band_half_width"),
            ("orientation_angle_deg", "orientation_deg"),
            ("absolute_gap_error_mean", "abs_gap_error"),
            ("reference_point_distance_consistency_mean", "reference_point_residual"),
            ("reference_normal_alignment_residual_mean", "reference_normal_alignment"),
            ("runtime_total_us_mean", "total_mean_us"),
            ("symmetry_residual_mean", "symmetry_residual"),
            ("invalid_result_count", "invalid_count"),
            ("reference_warning_count", "reference_warning_count"),
            ("reference_grazing_count", "reference_grazing_count"),
        ],
    )
    camera_main_results = _filter_rows(camera_accuracy, MAIN_RESULT_BENCHMARKS)
    camera_appendix = [
        row
        for row in camera_ablation
        if row.get("benchmark") not in MAIN_RESULT_BENCHMARKS or row.get("reference_quality") in {"low", "grazing-caution"}
    ]

    _write_csv(output_dir / "camera_ready_table_accuracy.csv", camera_accuracy)
    _write_markdown(output_dir / "camera_ready_table_accuracy.md", "Camera-Ready Accuracy Table", camera_accuracy)
    _write_csv(output_dir / "camera_ready_table_efficiency.csv", camera_efficiency)
    _write_markdown(output_dir / "camera_ready_table_efficiency.md", "Camera-Ready Efficiency Table", camera_efficiency)
    _write_csv(output_dir / "camera_ready_table_ablation.csv", camera_ablation)
    _write_markdown(output_dir / "camera_ready_table_ablation.md", "Camera-Ready Ablation Table", camera_ablation)
    _write_csv(output_dir / "camera_ready_table_main_results.csv", camera_main_results)
    _write_markdown(
        output_dir / "camera_ready_table_main_results.md",
        "Camera-Ready Main Results Table",
        camera_main_results,
    )
    _write_csv(output_dir / "camera_ready_table_appendix.csv", camera_appendix)
    _write_markdown(
        output_dir / "camera_ready_table_appendix.md",
        "Camera-Ready Appendix Table",
        camera_appendix,
    )

    figure_manifest = {
        "plots": {
            "gap_vs_resolution": "plot_gap_vs_resolution.csv",
            "runtime_vs_resolution": "plot_runtime_vs_resolution.csv",
            "symmetry_vs_bandwidth": "plot_symmetry_vs_bandwidth.csv",
            "normal_vs_orientation": "plot_normal_vs_orientation.csv",
            "runtime_vs_orientation": "plot_runtime_vs_orientation.csv",
        },
        "tables": {
            "main_results": "camera_ready_table_main_results.csv",
            "appendix": "camera_ready_table_appendix.csv",
            "accuracy": "camera_ready_table_accuracy.csv",
            "efficiency": "camera_ready_table_efficiency.csv",
            "ablation": "camera_ready_table_ablation.csv",
        },
        "source_dir": str(source_dir),
    }
    (output_dir / "figure_manifest.json").write_text(json.dumps(figure_manifest, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Export thin camera-ready tables from paper table outputs.")
    parser.add_argument("source_dir", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    export_camera_ready_tables(args.source_dir, args.output_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
