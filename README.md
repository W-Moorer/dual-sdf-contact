# dual-sdf-contact

Phase-4 paper experiment system for dual-SDF contact research.

Current paper mainline:

- WSL2 Ubuntu 22.04 / 24.04 first
- real `OpenVDB` SDF
- real `FCL` reference backend
- `SimpleCcpSolver`
- `ex05_compare_backends` as the benchmark driver
- Python subprocess runner plus lightweight postprocess for tables and plot-ready CSV

Windows native remains the supported fallback baseline path.

## Current Backend Status

Real backends on the validated WSL path:

- `OpenVDB`
  - real sphere / box / mesh level-set generation
  - world-space `phi` / `grad` sampling inside the trusted narrow band
  - current mesh benchmark path supports OBJ and ASCII STL input
- `FCL`
  - real distance / collision queries for sphere / box / mesh benchmark geometries
  - active reference path for primitive and mesh paper experiments
- `SimpleCcpSolver`
  - current solver backend used for benchmark-side contact probe metrics

Optional / not mainline in this phase:

- `NanoVDB`
- `hpp-fcl`
- `Siconos`

Windows fallback remains:

- `analytic` SDF provider
- `analytic` reference backend
- `SimpleCcpSolver`

## Benchmark Driver

`ex05_compare_backends` is the paper benchmark driver.

It now supports:

- config-driven primitive and mesh benchmark cases
- batch case execution
- pose / gap / resolution / narrow-band sweeps
- per-sample and per-group metrics
- suite-aware labels
- reproducible outputs for tables and downstream plotting

Built-in benchmark configs:

- `configs/benchmarks/primitive_smoke.json`
- `configs/benchmarks/gap_sweep.json`
- `configs/benchmarks/orientation_sweep.json`
- `configs/benchmarks/resolution_sweep.json`
- `configs/benchmarks/mesh_smoke.json`
- `configs/benchmarks/mesh_gap_sweep.json`
- `configs/benchmarks/mesh_resolution_sweep.json`

Built-in suites:

- `default`
  - functional smoke plus benchmark sanity path
- `paper_minimal`
  - primitive correctness / gap / orientation / resolution
  - mesh smoke / gap / resolution

## Current Coverage

Primitive coverage:

- sphere-sphere
- sphere-box
- box-box

Mesh-backed coverage:

- mesh-sphere
- mesh-box
- lightweight repository assets in `data/meshes/`
  - `unit_cube.obj`
  - `unit_octahedron.obj`

Current paper-ready experiment families:

- geometry correctness
  - `primitive_smoke`
  - `mesh_smoke`
- gap and narrow-band sensitivity
  - `gap_sweep`
  - `mesh_gap_sweep`
- pose stability
  - `orientation_sweep`
- resolution / efficiency ablation
  - `resolution_sweep`
  - `mesh_resolution_sweep`

## Output Layout

Per benchmark:

```text
outputs/ex05_compare_backends/
  <benchmark_name>/
    config_resolved.json
    environment.json
    samples.csv
    summary.csv
    summary.json
    report.md
```

Per suite:

```text
outputs/benchmark_suites/
  <suite_name>_<timestamp>/
    ex05_compare_backends/
      <benchmark_name>/...
    aggregate_summary.csv
    aggregate_summary.json
    aggregate_table.md
    paper_table_accuracy.csv
    paper_table_accuracy.md
    paper_table_efficiency.csv
    paper_table_efficiency.md
    paper_table_ablation.csv
    paper_table_ablation.md
    plot_gap_vs_resolution.csv
    plot_runtime_vs_resolution.csv
    plot_symmetry_vs_bandwidth.csv
```

Key labels now carried through sample / summary / aggregate outputs:

- `benchmark_name`
- `suite_name`
- `run_name`
- `case_family`
- `sweep_family`
- `case_name`
- `shape_a`
- `shape_b`
- `mesh_a`
- `mesh_b`
- `sdf_backend`
- `reference_backend`
- `solver_backend`
- `voxel_size`
- `narrow_band_half_width`
- `sample_count`
- `seed`

## Metrics

Main geometry metrics:

- `signed_gap`
- `reference_signed_distance`
- `gap_error`
- `absolute_gap_error`
- `relative_gap_error`
- `normal_angle_error_deg`
- `symmetry_residual`
- `tangent_orthogonality_residual`
- `point_distance_consistency`

Runtime metrics:

- `reference_runtime_us`
- `dual_sdf_runtime_us`
- `runtime_total_us_mean`
- `runtime_total_us_median`
- `runtime_total_us_p95`
- `batch_total_runtime_us`

Stability metrics:

- `invalid_result_count`
- `degenerate_normal_count`
- `tangent_frame_fallback_count`
- `narrow_band_edge_hit_count`

Solver probe metrics:

- `normal_impulse`
- `tangential_impulse_magnitude`
- `solver_residual`
- `solver_iterations`
- `solver_success_rate`

## WSL Workflow

Bootstrap once:

```bash
bash scripts/bootstrap_wsl.sh
```

Configure and build:

```bash
bash scripts/configure_wsl.sh Release
bash scripts/build_wsl.sh Release
```

Functional smoke path:

```bash
bash scripts/run_all_examples_wsl.sh Release
```

Single primitive benchmark:

```bash
bash scripts/run_benchmark_wsl.sh Release configs/benchmarks/primitive_smoke.json
```

Single mesh benchmark:

```bash
bash scripts/run_benchmark_wsl.sh Release configs/benchmarks/mesh_smoke.json
```

Default suite:

```bash
bash scripts/run_default_suite_wsl.sh Release
```

Paper minimal suite:

```bash
bash scripts/run_paper_minimal_suite_wsl.sh Release
```

Export paper tables from the latest `paper_minimal` suite:

```bash
bash scripts/export_paper_tables_wsl.sh
```

Manual Python entrypoints:

```bash
PYTHONPATH=python python3 -m baseline.run_benchmark --config configs/benchmarks/mesh_smoke.json --build-config Release
PYTHONPATH=python python3 -m baseline.run_benchmark --suite paper_minimal --build-config Release
PYTHONPATH=python python3 -m baseline.postprocess_benchmark outputs/benchmark_suites/paper_minimal_<timestamp> --output-dir outputs/postprocess/paper_minimal
```

## Windows Fallback Workflow

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/build_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/run_all_examples_windows.ps1 -Config Release
```

Windows presets still force:

- `BASELINE_FORCE_FALLBACK_SDF=ON`
- `BASELINE_FORCE_FALLBACK_REFERENCE=ON`
- `BASELINE_FORCE_SIMPLE_SOLVER=ON`

## Runtime Backend Selection

Runtime flags:

- `--sdf-backend analytic|openvdb|nanovdb`
- `--reference-backend analytic|hppfcl|fcl`
- `--solver-backend simple|siconos`

Benchmark flags on `ex05_compare_backends`:

- `--config <path>`
- `--case-family primitive|mesh`
- `--suite-name <name>`
- `--run-name <name>`
- `--output-dir <path>`
- `--seed <int>`

Environment variables:

- `BASELINE_SDF_BACKEND`
- `BASELINE_REFERENCE_BACKEND`
- `BASELINE_SOLVER_BACKEND`

## How To Verify Real Backends

OpenVDB is active when:

- configure summary shows `openvdb` in `SDF backend available`
- benchmark rows include `sdf_backend=openvdb`
- `samples.csv` shows `sdf_provider_backend=openvdb-real`

FCL is active when:

- configure summary shows `fcl` in `reference backend available`
- benchmark rows include `reference_backend=fcl`
- `samples.csv` shows `reference_engine_name=fcl-real`

Good files to inspect:

- `outputs/ex05_compare_backends/mesh_smoke/config_resolved.json`
- `outputs/ex05_compare_backends/mesh_smoke/environment.json`
- `outputs/ex05_compare_backends/mesh_smoke/samples.csv`
- `outputs/benchmark_suites/<paper_minimal_run>/paper_table_accuracy.csv`

## Tests

C++ smoke coverage includes:

- existing geometry / solver tests
- `test_benchmark_config_parse`
- `test_mesh_config_parse`
- `test_mesh_case_smoke`
- `test_ex05_generates_resolved_config`
- `test_ex05_generates_summary_files`
- `test_benchmark_backend_labels_present`
- `test_resolution_sweep_smoke`

Python smoke coverage includes:

- `tests/test_python_smoke.py`
- `tests/test_benchmark_runner.py`
- `tests/test_postprocess_benchmark.py`

Validated WSL commands:

```bash
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m pytest tests/test_python_smoke.py tests/test_benchmark_runner.py tests/test_postprocess_benchmark.py -q
```

## Notes

Current convention:

- all queries are in world frame
- voxel size is in world units
- narrow-band half width is stored in voxel units
- OpenVDB remains the real benchmark SDF path
- analytic SDF / reference backends remain important for ablation and fallback comparisons

Current phase focus:

- primitive + mesh benchmark coverage
- paper minimal experiment matrix
- stable CSV / JSON / markdown output
- stable table and plot-ready export

Not the current phase focus:

- deep `Siconos` work
- `NanoVDB` mainline integration
- replacing `FCL` with `hpp-fcl`
- GUI or heavy visualization

## Further Reading

- [docs/migrate_to_wsl.md](docs/migrate_to_wsl.md)
- [docs/benchmark_plan.md](docs/benchmark_plan.md)
