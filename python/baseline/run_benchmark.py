from __future__ import annotations

import argparse
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path

from .postprocess_benchmark import aggregate_benchmark_dirs, export_postprocess_artifacts
from .run_example import project_root, resolve_executable


SUITES = {
    "default": [
        "configs/benchmarks/primitive_smoke.json",
        "configs/benchmarks/gap_sweep.json",
        "configs/benchmarks/orientation_sweep.json",
        "configs/benchmarks/resolution_sweep.json",
        "configs/benchmarks/mesh_smoke.json",
    ],
    "paper_minimal": [
        "configs/benchmarks/primitive_smoke.json",
        "configs/benchmarks/gap_sweep.json",
        "configs/benchmarks/orientation_sweep.json",
        "configs/benchmarks/resolution_sweep.json",
        "configs/benchmarks/mesh_smoke.json",
        "configs/benchmarks/mesh_gap_sweep.json",
        "configs/benchmarks/mesh_resolution_sweep.json",
    ],
}


def timestamp_tag() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def load_benchmark_name(config_path: Path) -> str:
    with config_path.open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    return str(payload["benchmark_name"])


def run_single(
    executable: Path,
    benchmark_config: Path,
    output_dir: Path | None,
    sdf_backend: str | None,
    reference_backend: str | None,
    solver_backend: str | None,
    seed: int | None,
    suite_name: str | None,
    run_name: str | None,
) -> int:
    env = os.environ.copy()
    env.setdefault("PYTHONUTF8", "1")
    env.setdefault("BASELINE_OUTPUT_ROOT", str(project_root() / "outputs"))
    command = [str(executable), "--config", str(benchmark_config)]
    if output_dir is not None:
        command.extend(["--output-dir", str(output_dir)])
    if seed is not None:
        command.extend(["--seed", str(seed)])
    if suite_name:
        command.extend(["--suite-name", suite_name])
    if run_name:
        command.extend(["--run-name", run_name])
    if sdf_backend:
        command.extend(["--sdf-backend", sdf_backend])
    if reference_backend:
        command.extend(["--reference-backend", reference_backend])
    if solver_backend:
        command.extend(["--solver-backend", solver_backend])
    completed = subprocess.run(command, cwd=project_root(), env=env, check=False)
    return completed.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description="Run ex05 benchmark configs or suites.")
    parser.add_argument("--config", dest="benchmark_config", type=Path, default=None)
    parser.add_argument("--suite", choices=sorted(SUITES), default=None)
    parser.add_argument("--build-dir", type=Path, default=None)
    parser.add_argument("--build-config", default="Release")
    parser.add_argument("--output-dir", type=Path, default=None)
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--suite-name", default=None)
    parser.add_argument("--run-name", default=None)
    parser.add_argument("--sdf-backend", choices=["analytic", "openvdb", "nanovdb"], default=None)
    parser.add_argument("--reference-backend", choices=["analytic", "hppfcl", "fcl"], default=None)
    parser.add_argument("--solver-backend", choices=["simple", "siconos"], default=None)
    args = parser.parse_args()

    if bool(args.benchmark_config) == bool(args.suite):
        parser.error("Provide exactly one of --config or --suite.")

    executable = resolve_executable("ex05_compare_backends", args.build_dir, args.build_config)

    if args.benchmark_config:
        suite_name = args.suite_name or "standalone"
        run_name = args.run_name or load_benchmark_name(args.benchmark_config)
        return run_single(
            executable=executable,
            benchmark_config=args.benchmark_config,
            output_dir=args.output_dir,
            sdf_backend=args.sdf_backend,
            reference_backend=args.reference_backend,
            solver_backend=args.solver_backend,
            seed=args.seed,
            suite_name=suite_name,
            run_name=run_name,
        )

    suite_name = args.suite_name or args.suite
    run_name = args.run_name or f"{args.suite}_{timestamp_tag()}"
    suite_root = args.output_dir or (project_root() / "outputs" / "benchmark_suites" / run_name)
    benchmark_root = suite_root / "ex05_compare_backends"
    benchmark_dirs: list[Path] = []

    for relative_config in SUITES[args.suite]:
        config_path = project_root() / relative_config
        return_code = run_single(
            executable=executable,
            benchmark_config=config_path,
            output_dir=benchmark_root,
            sdf_backend=args.sdf_backend,
            reference_backend=args.reference_backend,
            solver_backend=args.solver_backend,
            seed=args.seed,
            suite_name=suite_name,
            run_name=run_name,
        )
        if return_code != 0:
            return return_code
        benchmark_dirs.append(benchmark_root / load_benchmark_name(config_path))

    rows = aggregate_benchmark_dirs(benchmark_dirs)
    export_postprocess_artifacts(rows, suite_root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
