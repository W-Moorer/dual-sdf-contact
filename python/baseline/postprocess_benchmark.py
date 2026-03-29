from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


def read_summary_rows(benchmark_dir: Path) -> list[dict[str, str]]:
    summary_path = benchmark_dir / "summary.csv"
    if not summary_path.exists():
        raise FileNotFoundError(f"Missing summary.csv in {benchmark_dir}")
    with summary_path.open("r", encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    for row in rows:
        row["benchmark_dir"] = str(benchmark_dir)
        row["benchmark_name"] = benchmark_dir.name
    return rows


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


def write_markdown(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("# Benchmark Table\n\n_No rows found._\n", encoding="utf-8")
        return
    columns = [
        "benchmark_name",
        "case_name",
        "sdf_backend",
        "reference_backend",
        "solver_backend",
        "voxel_size",
        "narrow_band_half_width",
        "absolute_gap_error_mean",
        "normal_angle_error_deg_mean",
        "runtime_total_us_mean",
    ]
    lines = [
        "# Benchmark Table",
        "",
        "| " + " | ".join(columns) + " |",
        "| " + " | ".join(["---"] * len(columns)) + " |",
    ]
    for row in rows:
        lines.append("| " + " | ".join(row.get(column, "") for column in columns) + " |")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_json(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"rows": rows}, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Aggregate benchmark output directories.")
    parser.add_argument("benchmark_dirs", nargs="+", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    rows = aggregate_benchmark_dirs(args.benchmark_dirs)
    write_csv(args.output_dir / "aggregate_summary.csv", rows)
    write_json(args.output_dir / "aggregate_summary.json", rows)
    write_markdown(args.output_dir / "aggregate_table.md", rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
