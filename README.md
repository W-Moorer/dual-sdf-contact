# dual-sdf-contact

Minimal but extensible baseline platform for 3D nonsmooth frictional contact dynamics experiments.

This repository now follows an explicit dual-track strategy:

- Windows native = baseline / fallback first
- WSL2 Ubuntu = real backend integration later

Current intent:

- Windows native is the recommended environment for skeleton development, fallback examples, interface validation, and solver prototyping.
- Future WSL2 Ubuntu 22.04 / 24.04 is the recommended environment for OpenVDB / NanoVDB / hpp-fcl / optional Siconos integration and paper-grade experiments.

This is a baseline platform, not the final paper system.

## Current Verified Status

Current host:

- Windows native
- no WSL available on this machine

Current default backend combination:

- SDF backend: `analytic`
- reference backend: `analytic`
- solver backend: `simple`

Those defaults are enforced on Windows by:

- `BASELINE_FORCE_FALLBACK_SDF=ON`
- `BASELINE_FORCE_FALLBACK_REFERENCE=ON`
- `BASELINE_FORCE_SIMPLE_SOLVER=ON`

## What Exists

Core modules:

- `SdfProvider`
- `OpenVdbSdfProvider`
- `NanoVdbSdfProvider`
- `CandidatePairManager`
- `DualSdfContactCalculator`
- `ContactProblemBuilder`
- `ContactSolver`
- `SimpleCcpSolver`
- `OptionalSiconosSolver`
- runtime backend factory / registry

Real-backend skeletons are already present in code:

- [openvdb_sdf_provider.h](E:/workspace/dual-sdf-contact/include/baseline/sdf/openvdb_sdf_provider.h)
- [nanovdb_sdf_provider.h](E:/workspace/dual-sdf-contact/include/baseline/sdf/nanovdb_sdf_provider.h)
- [reference_geometry.h](E:/workspace/dual-sdf-contact/include/baseline/contact/reference_geometry.h)
- [siconos_solver.h](E:/workspace/dual-sdf-contact/include/baseline/solver/siconos_solver.h)
- [backend_factory.h](E:/workspace/dual-sdf-contact/include/baseline/runtime/backend_factory.h)

Current semantics:

- `analytic` is the stable fallback execution path.
- `openvdb`, `nanovdb`, `hppfcl`, `fcl`, and `siconos` now have stable API skeletons and factory entries.
- If those backends are explicitly requested while unavailable, the executable returns a clear error.
- If those dependencies are detected in the future, the same factory and example entrypoints can select them without rewriting example bodies.
- Today, the non-fallback adapters still report honestly that they delegate to fallback execution until the real library calls are wired in.

## Architecture

```text
parameterized geometry
    |
    +--> ReferenceGeometry -----------------------------+
    |                                                   |
    +--> SdfProvider / backend factory                  |
                                                        v
                                             CandidatePairManager
                                                        |
                                                        v
                                          DualSdfContactCalculator
                                                        |
                                                        v
                                           ContactProblemBuilder
                                                        |
                                                        v
                                ContactSolver / backend factory layer
                                   |                     |
                                   |                     +--> OptionalSiconosSolver
                                   |
                                   +--> SimpleCcpSolver
                                                        |
                                                        v
                                           CSV / JSON / Markdown outputs
```

## Repository Layout

```text
.
|-- CMakeLists.txt
|-- CMakePresets.json
|-- cmake/
|-- docs/
|-- third_party/
|-- include/
|   `-- baseline/
|-- src/
|   |-- core/
|   |-- runtime/
|   |-- sdf/
|   |-- contact/
|   |-- solver/
|   `-- apps/
|-- python/
|   `-- baseline/
|-- tests/
|-- outputs/
`-- scripts/
```

## Backend Factory

Runtime backend selection lives in:

- [backend_factory.cpp](E:/workspace/dual-sdf-contact/src/runtime/backend_factory.cpp)

Supported names:

- SDF: `analytic`, `openvdb`, `nanovdb`
- reference: `analytic`, `hppfcl`, `fcl`
- solver: `simple`, `siconos`

Examples and the Python runner now go through this layer instead of hard-coding implementation class names.

## CMake Options

Backend-related options:

- `BASELINE_WITH_OPENVDB`
- `BASELINE_WITH_NANOVDB`
- `BASELINE_WITH_HPP_FCL`
- `BASELINE_WITH_FCL`
- `BASELINE_WITH_SICONOS`
- `BASELINE_FORCE_FALLBACK_SDF`
- `BASELINE_FORCE_FALLBACK_REFERENCE`
- `BASELINE_FORCE_SIMPLE_SOLVER`

Configure-time output prints:

- dependency detection summary
- selected default SDF / reference / solver backends
- force-fallback flags
- currently available real backends

On the current Windows host the summary is effectively:

```text
SDF backend: analytic
reference backend: analytic
solver backend: simple
real backends available: none
```

## Dependency Strategy

### Windows Native

Recommended for:

- fallback baseline development
- interface stabilization
- examples and tests
- solver prototyping

Preferred toolchain:

- Visual Studio 2022
- MSVC
- CMake
- optional Ninja
- optional vcpkg

The project does not require OpenVDB / NanoVDB / hpp-fcl / FCL / Siconos to exist on Windows.

### Future WSL2 Ubuntu 22.04 / 24.04

Recommended later for:

- OpenVDB / NanoVDB integration
- hpp-fcl / FCL integration
- optional Siconos integration
- formal experiment runs

Recommended install order later:

1. `Eigen`
2. `OpenVDB / NanoVDB`
3. `hpp-fcl / FCL`
4. optional `Siconos`

Important WSL note:

- keep build directories inside the Linux filesystem instead of `/mnt/c` when compile speed matters.

## Build And Run

### Windows Native

Recommended script flow:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/build_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/run_all_examples_windows.ps1 -Config Release
```

Optional vcpkg bootstrap:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap_windows.ps1
```

Manual Windows flow:

```powershell
cmake -S . -B build/windows-release -G "Visual Studio 17 2022" -A x64
cmake --build build/windows-release --config Release --parallel
ctest --test-dir build/windows-release -C Release --output-on-failure
$env:PYTHONPATH = "$PWD\python"
python -m baseline.run_example ex01_nanovdb_hello
```

### Future WSL Flow

These scripts are included now for future WSL devices.
They were not executed on the current Windows-only machine.

```bash
bash scripts/bootstrap_wsl.sh
bash scripts/configure_wsl.sh wsl-release
bash scripts/build_wsl.sh wsl-release
bash scripts/run_all_examples_wsl.sh
```

Manual WSL flow later:

```bash
cmake --preset wsl-release
cmake --build --preset wsl-release
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m baseline.run_example ex01_nanovdb_hello
```

## CMake Presets

Provided presets:

- `windows-debug`
- `windows-release`
- `wsl-debug`
- `wsl-release`

Policy encoded in presets:

- Windows presets force the fallback baseline path.
- WSL presets disable the force-fallback knobs so future real backend work can be enabled through configuration instead of example rewrites.

## Python Entry Point

Examples are launched through built C++ executables:

```bash
python -m baseline.run_example ex01_nanovdb_hello
python -m baseline.run_example ex02_hppfcl_distance
python -m baseline.run_example ex03_dual_sdf_gap
python -m baseline.run_example ex04_single_step_contact
python -m baseline.run_example ex05_compare_backends
python -m baseline.run_example ex06_regression_smoke
```

Backend flags are supported:

```bash
python -m baseline.run_example ex03_dual_sdf_gap --sdf-backend analytic
python -m baseline.run_example ex02_hppfcl_distance --reference-backend analytic
python -m baseline.run_example ex04_single_step_contact --solver-backend simple
```

Flags:

- `--sdf-backend analytic|openvdb|nanovdb`
- `--reference-backend analytic|hppfcl|fcl`
- `--solver-backend simple|siconos`

Default behavior on the current Windows host remains the stable fallback combination.

## Examples

- `ex01_nanovdb_hello`: reports selected SDF backend and whether fallback execution is running.
- `ex02_hppfcl_distance`: reports selected reference backend and fallback status.
- `ex03_dual_sdf_gap`: reports backend names and a symmetry residual.
- `ex04_single_step_contact`: reports selected SDF and solver backends while keeping `SimpleCcpSolver` visible as baseline.
- `ex05_compare_backends`: compares analytic reference, selected reference, dual-SDF result, simple solver, and selected solver backend.
- `ex06_regression_smoke`: small end-to-end smoke run through the same backend factory layer.

## Tests

C++ tests:

- `test_sdf_sampling`
- `test_dual_sdf_gap_symmetry`
- `test_contact_frame_orthogonality`
- `test_simple_ccp_solver_smoke`
- `test_backend_factory_defaults`
- `test_unavailable_backend_handling`
- `test_example_reports_backend_name`
- `test_siconos_backend_smoke` when Siconos is detected

Python smoke:

- `tests/test_python_smoke.py`

## Outputs

Outputs are written under:

- `outputs/ex01_nanovdb_hello/`
- `outputs/ex02_hppfcl_distance/`
- `outputs/ex03_dual_sdf_gap/`
- `outputs/ex04_single_step_contact/`
- `outputs/ex05_compare_backends/`
- `outputs/ex06_regression_smoke/`

`BASELINE_OUTPUT_ROOT` can override the output root.

## Migration

See [migrate_to_wsl.md](E:/workspace/dual-sdf-contact/docs/migrate_to_wsl.md) for the concrete migration checklist for the future WSL2 Ubuntu device.
