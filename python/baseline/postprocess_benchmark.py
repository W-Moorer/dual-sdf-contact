from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Iterable


def _read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def _to_float(row: dict[str, str], key: str) -> float:
    value = row.get(key, "")
    if value == "":
        return 0.0
    return float(value)


def read_summary_rows(benchmark_dir: Path) -> list[dict[str, str]]:
    summary_path = benchmark_dir / "summary.csv"
    if not summary_path.exists():
        raise FileNotFoundError(f"Missing summary.csv in {benchmark_dir}")
    rows = _read_csv(summary_path)
    for row in rows:
        row["benchmark_dir"] = str(benchmark_dir)
        row.setdefault("benchmark_name", benchmark_dir.name)
        row.setdefault("suite_name", "standalone")
        row.setdefault("run_name", row.get("benchmark_name", benchmark_dir.name))
    return rows


def discover_benchmark_dirs(paths: Iterable[Path]) -> list[Path]:
    benchmark_dirs: list[Path] = []
    for raw_path in paths:
        path = raw_path.resolve()
        if (path / "summary.csv").exists():
            benchmark_dirs.append(path)
            continue
        ex05_dir = path / "ex05_compare_backends"
        if ex05_dir.exists():
            benchmark_dirs.extend(sorted(child for child in ex05_dir.iterdir() if (child / "summary.csv").exists()))
            continue
        benchmark_dirs.extend(sorted(child for child in path.iterdir() if child.is_dir() and (child / "summary.csv").exists()))
    unique_dirs: list[Path] = []
    seen: set[Path] = set()
    for directory in benchmark_dirs:
        if directory not in seen:
            unique_dirs.append(directory)
            seen.add(directory)
    return unique_dirs


def aggregate_benchmark_dirs(benchmark_dirs: list[Path]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for benchmark_dir in benchmark_dirs:
        rows.extend(read_summary_rows(benchmark_dir))
    return rows


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_json(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"rows": rows}, indent=2) + "\n", encoding="utf-8")


def write_markdown_table(path: Path, title: str, rows: list[dict[str, str]], columns: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text(f"# {title}\n\n_No rows found._\n", encoding="utf-8")
        return
    lines = [
        f"# {title}",
        "",
        "| " + " | ".join(columns) + " |",
        "| " + " | ".join(["---"] * len(columns)) + " |",
    ]
    for row in rows:
        lines.append("| " + " | ".join(row.get(column, "") for column in columns) + " |")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_markdown(path: Path, rows: list[dict[str, str]]) -> None:
    write_markdown_table(
        path,
        "Benchmark Table",
        rows,
        [
            "suite_name",
            "benchmark_name",
            "case_name",
            "shape_pair",
            "sdf_backend",
            "reference_backend",
            "voxel_size",
            "narrow_band_half_width",
            "absolute_gap_error_mean",
            "normal_angle_error_deg_mean",
            "runtime_total_us_mean",
        ],
    )


def _select_columns(rows: list[dict[str, str]], columns: list[str]) -> list[dict[str, str]]:
    return [{column: row.get(column, "") for column in columns} for row in rows]


def _sort_rows(rows: list[dict[str, str]], numeric_keys: list[str], lexical_keys: list[str]) -> list[dict[str, str]]:
    return sorted(
        rows,
        key=lambda row: tuple(_to_float(row, key) for key in numeric_keys)
        + tuple(row.get(key, "") for key in lexical_keys),
    )


def build_accuracy_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    columns = [
        "suite_name",
        "benchmark_name",
        "case_family",
        "sweep_family",
        "case_name",
        "shape_a",
        "shape_b",
        "mesh_a",
        "mesh_b",
        "sdf_backend",
        "reference_backend",
        "voxel_size",
        "narrow_band_half_width",
        "absolute_gap_error_mean",
        "normal_angle_error_deg_mean",
        "symmetry_residual_mean",
        "point_distance_consistency_mean",
    ]
    return _select_columns(_sort_rows(rows, ["voxel_size", "narrow_band_half_width"], ["benchmark_name", "case_name"]), columns)


def build_efficiency_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    columns = [
        "suite_name",
        "benchmark_name",
        "case_name",
        "shape_pair",
        "sdf_backend",
        "reference_backend",
        "voxel_size",
        "narrow_band_half_width",
        "reference_runtime_us_mean",
        "dual_sdf_runtime_us_mean",
        "runtime_total_us_mean",
        "runtime_total_us_p95",
        "batch_total_runtime_us",
        "invalid_result_count",
        "narrow_band_edge_hit_count",
    ]
    return _select_columns(_sort_rows(rows, ["runtime_total_us_mean"], ["benchmark_name", "case_name"]), columns)


def build_ablation_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    filtered = [
        row
        for row in rows
        if row.get("sweep_family") in {"gap_sweep", "orientation_sweep", "resolution_sweep"}
        or "mesh" in row.get("benchmark_name", "")
    ]
    columns = [
        "suite_name",
        "benchmark_name",
        "case_family",
        "sweep_family",
        "case_name",
        "shape_pair",
        "mesh_a",
        "mesh_b",
        "voxel_size",
        "narrow_band_half_width",
        "absolute_gap_error_mean",
        "runtime_total_us_mean",
        "symmetry_residual_mean",
        "invalid_result_count",
        "narrow_band_edge_hit_count",
    ]
    return _select_columns(_sort_rows(filtered, ["voxel_size", "narrow_band_half_width"], ["benchmark_name", "case_name"]), columns)


def build_plot_gap_vs_resolution(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    filtered = [row for row in rows if row.get("benchmark_name") in {"resolution_sweep", "mesh_resolution_sweep"}]
    columns = [
        "suite_name",
        "benchmark_name",
        "case_name",
        "shape_pair",
        "mesh_a",
        "mesh_b",
        "sdf_backend",
        "reference_backend",
        "voxel_size",
        "narrow_band_half_width",
        "absolute_gap_error_mean",
        "gap_error_mean",
        "normal_angle_error_deg_mean",
    ]
    return _select_columns(_sort_rows(filtered, ["voxel_size"], ["benchmark_name", "case_name"]), columns)


def build_plot_runtime_vs_resolution(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    filtered = [row for row in rows if row.get("benchmark_name") in {"resolution_sweep", "mesh_resolution_sweep"}]
    columns = [
        "suite_name",
        "benchmark_name",
        "case_name",
        "shape_pair",
        "sdf_backend",
        "reference_backend",
        "voxel_size",
        "narrow_band_half_width",
        "reference_runtime_us_mean",
        "dual_sdf_runtime_us_mean",
        "runtime_total_us_mean",
        "runtime_total_us_p95",
    ]
    return _select_columns(_sort_rows(filtered, ["voxel_size"], ["benchmark_name", "case_name"]), columns)


def build_plot_symmetry_vs_bandwidth(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    filtered = [
        row
        for row in rows
        if row.get("benchmark_name") in {"gap_sweep", "mesh_gap_sweep", "resolution_sweep", "mesh_resolution_sweep"}
    ]
    columns = [
        "suite_name",
        "benchmark_name",
        "case_name",
        "shape_pair",
        "sdf_backend",
        "reference_backend",
        "voxel_size",
        "narrow_band_half_width",
        "symmetry_residual_mean",
        "invalid_result_count",
        "narrow_band_edge_hit_count",
        "runtime_total_us_mean",
    ]
    return _select_columns(_sort_rows(filtered, ["narrow_band_half_width", "voxel_size"], ["benchmark_name", "case_name"]), columns)


def export_postprocess_artifacts(rows: list[dict[str, str]], output_dir: Path) -> None:
    write_csv(output_dir / "aggregate_summary.csv", rows)
    write_json(output_dir / "aggregate_summary.json", rows)
    write_markdown(output_dir / "aggregate_table.md", rows)

    accuracy_rows = build_accuracy_rows(rows)
    efficiency_rows = build_efficiency_rows(rows)
    ablation_rows = build_ablation_rows(rows)
    plot_gap_rows = build_plot_gap_vs_resolution(rows)
    plot_runtime_rows = build_plot_runtime_vs_resolution(rows)
    plot_symmetry_rows = build_plot_symmetry_vs_bandwidth(rows)

    write_csv(output_dir / "paper_table_accuracy.csv", accuracy_rows)
    write_markdown_table(
        output_dir / "paper_table_accuracy.md",
        "Paper Table Accuracy",
        accuracy_rows,
        list(accuracy_rows[0].keys()) if accuracy_rows else ["benchmark_name"],
    )
    write_csv(output_dir / "paper_table_efficiency.csv", efficiency_rows)
    write_markdown_table(
        output_dir / "paper_table_efficiency.md",
        "Paper Table Efficiency",
        efficiency_rows,
        list(efficiency_rows[0].keys()) if efficiency_rows else ["benchmark_name"],
    )
    write_csv(output_dir / "paper_table_ablation.csv", ablation_rows)
    write_markdown_table(
        output_dir / "paper_table_ablation.md",
        "Paper Table Ablation",
        ablation_rows,
        list(ablation_rows[0].keys()) if ablation_rows else ["benchmark_name"],
    )
    write_csv(output_dir / "plot_gap_vs_resolution.csv", plot_gap_rows)
    write_csv(output_dir / "plot_runtime_vs_resolution.csv", plot_runtime_rows)
    write_csv(output_dir / "plot_symmetry_vs_bandwidth.csv", plot_symmetry_rows)


def main() -> int:
    parser = argparse.ArgumentParser(description="Aggregate benchmark output directories.")
    parser.add_argument("benchmark_dirs", nargs="+", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    benchmark_dirs = discover_benchmark_dirs(args.benchmark_dirs)
    rows = aggregate_benchmark_dirs(benchmark_dirs)
    export_postprocess_artifacts(rows, args.output_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
