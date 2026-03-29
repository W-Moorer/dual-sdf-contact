# dual-sdf-contact

Phase-2 baseline platform for dual-SDF contact research.

Current policy:

- WSL2 Ubuntu 22.04 / 24.04 is the first-priority environment.
- Windows native remains a supported fallback baseline path.
- The main paper-track work is now the geometry layer:
  - real SDF backend
  - real reference backend
  - dual-SDF kinematics and benchmark outputs
- `Siconos` stays optional in this stage.

## Current Backend Status

Real backends integrated now:

- `OpenVDB`
  - real sphere / box level-set generation
  - real world-space `phi` / `grad` sampling inside the active narrow band
  - explicit narrow-band / world-AABB reporting
  - analytic extension outside the stored narrow band for the current primitive-backed examples
- `FCL`
  - real `distance()` / `collide()` on `sphere-sphere`, `sphere-box`, `box-box`
  - nearest points and collision normal where the API can provide them

Still optional / partial:

- `NanoVDB`
  - interface kept
  - current stage keeps it in optional/read-only skeleton status
  - fallback execution remains analytic
- `hpp-fcl`
  - interface kept
  - preferred future target when package availability is better
  - current stage still falls back to analytic execution
- `Siconos`
  - interface kept through `OptionalSiconosSolver`
  - not the primary focus of this stage
  - current solver priority remains `SimpleCcpSolver`

Windows fallback remains intact:

- `analytic` SDF provider
- `analytic` reference backend
- `SimpleCcpSolver`

## Configure Summary

Configure now prints a backend summary in this format:

```text
SDF backend available:
reference backend available:
solver backend available:
active default backend:
fallback enabled:
```

On the current validated WSL path, the summary is typically:

```text
SDF backend available: analytic, openvdb
reference backend available: analytic, fcl
solver backend available: simple
active default backend: sdf=openvdb, reference=fcl, solver=simple
fallback enabled: sdf=OFF, reference=OFF, solver=OFF
```

## Build And Run

### Recommended WSL Workflow

Bootstrap:

```bash
bash scripts/bootstrap_wsl.sh
```

Configure and build:

```bash
bash scripts/configure_wsl.sh Release
bash scripts/build_wsl.sh Release
```

Run all examples and smoke tests:

```bash
bash scripts/run_all_examples_wsl.sh Release
```

Manual WSL flow:

```bash
cmake --preset wsl-release
cmake --build --preset wsl-release
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m pytest tests/test_python_smoke.py -q
```

### Windows Fallback Workflow

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/build_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/run_all_examples_windows.ps1 -Config Release
```

Windows presets keep:

- `BASELINE_FORCE_FALLBACK_SDF=ON`
- `BASELINE_FORCE_FALLBACK_REFERENCE=ON`
- `BASELINE_FORCE_SIMPLE_SOLVER=ON`

## Backend Selection

CLI flags:

- `--sdf-backend analytic|openvdb|nanovdb`
- `--reference-backend analytic|hppfcl|fcl`
- `--solver-backend simple|siconos`

Examples and the Python runner both use the same runtime backend factory.

Environment variables are also supported:

- `BASELINE_SDF_BACKEND`
- `BASELINE_REFERENCE_BACKEND`
- `BASELINE_SOLVER_BACKEND`

Python subprocess entrypoint:

```bash
PYTHONPATH=python python3 -m baseline.run_example ex03_dual_sdf_gap --sdf-backend openvdb
```

## How To Tell You Are Using A Real Backend

For `OpenVDB`:

- configure summary shows `openvdb` in `SDF backend available`
- runtime selection resolves to `selected_sdf_backend=openvdb`
- example outputs report `provider_backend_name=openvdb-real`

For `FCL`:

- configure summary shows `fcl` in `reference backend available`
- runtime selection resolves to `selected_reference_backend=fcl`
- example outputs report `engine_name=fcl-real`

Files to inspect:

- `outputs/ex01_nanovdb_hello/summary.json`
- `outputs/ex02_hppfcl_distance/summary.json`
- `outputs/ex03_dual_sdf_gap/contact.json`
- `outputs/ex05_compare_backends/summary.json`

## Example Status

### `ex01_nanovdb_hello`

- WSL default path uses `OpenVDB` when detected
- writes sampled `phi / grad / in_narrow_band` to `outputs/ex01_nanovdb_hello/samples.csv`
- writes backend and grid-frame convention summary to `outputs/ex01_nanovdb_hello/summary.json`

Run:

```bash
./build/wsl-release/bin/Release/ex01_nanovdb_hello
python3 -m baseline.run_example ex01_nanovdb_hello --sdf-backend openvdb
```

### `ex02_hppfcl_distance`

- prefers `hpp-fcl` if ever detected
- currently validated real path is `FCL`
- reports distance, signed distance, collision, closest points, and normal

Run:

```bash
./build/wsl-release/bin/Release/ex02_hppfcl_distance
python3 -m baseline.run_example ex02_hppfcl_distance --reference-backend fcl
```

### `ex03_dual_sdf_gap`

- compares analytic dual-SDF against the selected SDF backend
- validated WSL real path is `OpenVDB`
- reports:
  - signed gap
  - normal
  - symmetry residual
  - tangent orthogonality
  - gradient alignment
  - valid flags

Run:

```bash
./build/wsl-release/bin/Release/ex03_dual_sdf_gap
python3 -m baseline.run_example ex03_dual_sdf_gap --sdf-backend openvdb
```

### `ex04_single_step_contact`

- still uses `SimpleCcpSolver` as the practical stage-2 solver
- backend names are standardized in the JSON output
- works with selected SDF backends, including `OpenVDB` on WSL

### `ex05_compare_backends`

- upgraded to a benchmark seed
- compares:
  - analytic reference vs selected reference backend
  - analytic SDF vs selected SDF backend
  - gap error
  - normal angle error
  - runtime per query
- writes:
  - `outputs/ex05_compare_backends/summary.csv`
  - `outputs/ex05_compare_backends/summary.json`
  - `outputs/ex05_compare_backends/report.md`

Run:

```bash
./build/wsl-release/bin/Release/ex05_compare_backends
python3 -m baseline.run_example ex05_compare_backends --sdf-backend openvdb --reference-backend fcl
```

### `ex06_regression_smoke`

- stays minimal
- keeps an end-to-end executable smoke path in place

## Dual-SDF Kinematics Output

`DualSdfContactCalculator` now emits a unified `ContactKinematicsResult` with:

- `signed_gap`
- `normal`
- `tangent1`
- `tangent2`
- `point_on_a`
- `point_on_b`
- `symmetry_residual`
- `valid_flags`

It explicitly handles:

- degenerate normals
- unstable tangential bases
- gradient mismatch
- narrow-band boundary queries

## Tests

Validated C++ tests:

- `test_sdf_sampling`
- `test_dual_sdf_gap_symmetry`
- `test_contact_frame_orthogonality`
- `test_simple_ccp_solver_smoke`
- `test_backend_factory_defaults`
- `test_unavailable_backend_handling`
- `test_example_reports_backend_name`
- `test_openvdb_provider_smoke`
- `test_reference_backend_switch`
- `test_dual_sdf_gap_consistency_against_analytic`
- `test_ex05_outputs_exist`

Python subprocess smoke:

- `tests/test_python_smoke.py`

Backend-specific C++ tests use CTest skip semantics when the relevant real dependency is not available.

## Presets

Provided presets:

- `wsl-debug`
- `wsl-release`
- `windows-debug`
- `windows-release`

The preset file is intentionally kept compatible with `cmake 3.22`, which is common on Ubuntu 22.04 WSL images.

## Notes On OpenVDB Usage

Current convention:

- queries are issued in world coordinates
- voxel size is in world units
- `narrow_band` is stored in voxel units
- the generated grids use an axis-aligned linear world/grid transform

For the current primitive-backed examples:

- inside the trusted OpenVDB narrow band, sampling is real OpenVDB
- outside that trusted band, the provider explicitly falls back to the known analytic primitive extension

This keeps the stage-2 benchmark path physically meaningful while still exercising a real sparse level-set backend where it matters most.

## Next Steps

Highest-value follow-ups:

1. real `NanoVDB` read-only query path from `OpenVDB` grids
2. real `hpp-fcl` adapter when package availability is practical
3. mesh-to-level-set and non-primitive contact benchmark cases

See [docs/migrate_to_wsl.md](docs/migrate_to_wsl.md) for the stage-2 WSL migration notes and near-term priorities.
