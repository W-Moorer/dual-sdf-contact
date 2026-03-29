# dual-sdf-contact

Phase-3 paper-track benchmark platform for dual-SDF contact research.

Current priorities:

- WSL2 Ubuntu 22.04 / 24.04 is the first-priority environment.
- Windows native remains a supported fallback baseline path.
- The paper mainline is now:
  - real `OpenVDB` SDF
  - real `FCL` reference backend
  - dual-SDF kinematics
  - `ex05_compare_backends` as the benchmark driver
- `Siconos`, `hpp-fcl`, and `NanoVDB` remain optional.

## Current Backend Status

Real backends on the validated WSL path:

- `OpenVDB`
  - real sphere / box level-set generation
  - rotated box level-set generation through mesh-to-level-set for the current primitive benchmark path
  - real world-space `phi` / `grad` sampling inside the trusted narrow band
- `FCL`
  - real distance / collision queries for sphere-sphere, sphere-box, and box-box
  - rotated box transforms are wired through the current primitive path

Optional / partial:

- `NanoVDB`
  - interface kept
  - not on the main benchmark path
- `hpp-fcl`
  - interface kept
  - not required while `FCL` already supports the paper mainline
- `Siconos`
  - interface kept through `OptionalSiconosSolver`
  - not the focus of this phase

Windows fallback remains intact:

- `analytic` SDF provider
- `analytic` reference backend
- `SimpleCcpSolver`

## Stage-3 Benchmark Driver

`ex05_compare_backends` is now the paper benchmark driver rather than a one-off example.

It supports:

- config-driven benchmark execution from `configs/benchmarks/*.json`
- primitive smoke cases
- gap sweep
- orientation sweep
- resolution sweep
- narrow-band width ablation
- batch sample output plus aggregate statistics
- backend-aware output directories and environment snapshots

Current built-in configs:

- `configs/benchmarks/primitive_smoke.json`
- `configs/benchmarks/gap_sweep.json`
- `configs/benchmarks/orientation_sweep.json`
- `configs/benchmarks/resolution_sweep.json`

Current default suite:

- `primitive_smoke`
- `gap_sweep`
- `orientation_sweep`
- `resolution_sweep`

## Benchmark Output Layout

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

Suite output:

```text
outputs/benchmark_suites/
  <suite_name>_<timestamp>/
    ex05_compare_backends/
      <benchmark_name>/...
    aggregate_summary.csv
    aggregate_summary.json
    aggregate_table.md
```

Key output meanings:

- `config_resolved.json`
  - final benchmark config after CLI overrides
  - resolved executable backends
- `environment.json`
  - platform track
  - compiler id / version
  - CMake generator
  - dependency availability
  - active backends
- `samples.csv`
  - sample-level metrics
- `summary.csv`
  - aggregate metrics per case / backend / voxel / narrow-band group
- `summary.json`
  - machine-readable aggregate plus embedded sample rows
- `report.md`
  - quick human scan for current run

## Metrics Emitted By `ex05`

Sample-level metrics include:

- `signed_gap`
- `reference_signed_distance`
- `gap_error`
- `absolute_gap_error`
- `relative_gap_error`
- `normal_angle_error_deg`
- `symmetry_residual`
- `tangent_orthogonality_residual`
- `point_distance_consistency`
- `reference_runtime_us`
- `dual_sdf_runtime_us`
- `solver_runtime_us`
- `runtime_total_us`
- `normal_impulse`
- `tangential_impulse_magnitude`
- `solver_residual`
- `solver_iterations`
- `solver_success_flag`
- backend labels and resolved provider / engine names

Aggregate summaries include:

- mean
- std
- min
- max
- median
- p95
- invalid result count
- degenerate normal count
- tangent frame fallback count
- narrow-band edge hit count
- total batch runtime

## Build And Run

### Recommended WSL Workflow

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

Paper benchmark smoke path:

```bash
bash scripts/run_benchmark_wsl.sh Release configs/benchmarks/primitive_smoke.json
```

Default benchmark suite:

```bash
bash scripts/run_default_suite_wsl.sh Release
```

Manual benchmark commands:

```bash
PYTHONPATH=python python3 -m baseline.run_benchmark --config configs/benchmarks/primitive_smoke.json --build-config Release
PYTHONPATH=python python3 -m baseline.run_benchmark --suite default --build-config Release
PYTHONPATH=python python3 -m baseline.postprocess_benchmark outputs/ex05_compare_backends/primitive_smoke --output-dir outputs/postprocess/primitive_smoke
```

### Windows Fallback Workflow

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/build_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/run_all_examples_windows.ps1 -Config Release
```

Windows presets still force:

- `BASELINE_FORCE_FALLBACK_SDF=ON`
- `BASELINE_FORCE_FALLBACK_REFERENCE=ON`
- `BASELINE_FORCE_SIMPLE_SOLVER=ON`

## Example Entrypoints

Examples remain available for focused checks:

- `ex01_nanovdb_hello`
  - OpenVDB SDF sampling smoke
- `ex02_hppfcl_distance`
  - reference backend distance / collision smoke
- `ex03_dual_sdf_gap`
  - analytic vs selected SDF kinematics
- `ex04_single_step_contact`
  - single-step solver probe
- `ex05_compare_backends`
  - benchmark driver
- `ex06_regression_smoke`
  - minimal end-to-end regression smoke

Run through the Python subprocess runner:

```bash
PYTHONPATH=python python3 -m baseline.run_example ex03_dual_sdf_gap --sdf-backend openvdb
PYTHONPATH=python python3 -m baseline.run_example ex05_compare_backends
```

## Backend Selection

Runtime backend flags:

- `--sdf-backend analytic|openvdb|nanovdb`
- `--reference-backend analytic|hppfcl|fcl`
- `--solver-backend simple|siconos`

Benchmark-specific flags on `ex05_compare_backends`:

- `--config <path>`
- `--case-family ...`
- `--output-dir <path>`
- `--seed <int>`

Environment variables are also supported:

- `BASELINE_SDF_BACKEND`
- `BASELINE_REFERENCE_BACKEND`
- `BASELINE_SOLVER_BACKEND`

## How To Tell You Are Using Real Backends

For `OpenVDB`:

- configure summary shows `openvdb` in `SDF backend available`
- benchmark rows include `sdf_backend=openvdb`
- `samples.csv` shows `sdf_provider_backend=openvdb-real`

For `FCL`:

- configure summary shows `fcl` in `reference backend available`
- benchmark rows include `reference_backend=fcl`
- `samples.csv` shows `reference_engine_name=fcl-real`

Files to inspect:

- `outputs/ex05_compare_backends/primitive_smoke/config_resolved.json`
- `outputs/ex05_compare_backends/primitive_smoke/environment.json`
- `outputs/ex05_compare_backends/primitive_smoke/samples.csv`
- `outputs/ex05_compare_backends/primitive_smoke/summary.csv`

## Tests

Validated C++ tests now include:

- existing geometry / solver smoke tests
- `test_benchmark_config_parse`
- `test_ex05_generates_resolved_config`
- `test_ex05_generates_summary_files`
- `test_benchmark_backend_labels_present`
- `test_resolution_sweep_smoke`

Python tests:

- `tests/test_python_smoke.py`
- `tests/test_benchmark_runner.py`

Validated commands on the current WSL path:

```bash
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m pytest tests/test_python_smoke.py tests/test_benchmark_runner.py -q
```

## Notes

Current world/grid convention:

- queries are in world frame
- voxel size is in world units
- narrow-band width is stored in voxel units
- the active primitive benchmark path uses analytic extension outside the trusted OpenVDB band

Mesh-backed cases are intentionally only scaffolded at the config/model layer in this phase.
They are not allowed to block primitive benchmark execution.

## Further Reading

- [docs/migrate_to_wsl.md](docs/migrate_to_wsl.md)
- [docs/benchmark_plan.md](docs/benchmark_plan.md)
