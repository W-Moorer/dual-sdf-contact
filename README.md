# dual-sdf-contact

Minimal but extensible baseline platform for 3D multi-body nonsmooth frictional contact dynamics experiments, aimed at papers built around bidirectional narrow-band SDFs plus CCP-style contact solves.

This repository is intentionally a baseline platform, not a full paper system.
The first version prioritizes:

- C++ core code that compiles and runs from a blank repository.
- Python entrypoints that simply call built C++ executables.
- Clear module boundaries so OpenVDB / NanoVDB / hpp-fcl / Siconos can be swapped in later.
- Stable examples and regression outputs under `outputs/`.
- WSL-first workflow, with Windows-native support as a practical fallback.

## What Is Implemented

Current C++ modules:

- `SdfProvider` interface.
- `OpenVdbSdfProvider` adapter skeleton.
- `NanoVdbSdfProvider` adapter skeleton.
- `CandidatePairManager`.
- `DualSdfContactCalculator`.
- `ContactProblemBuilder`.
- `ContactSolver` interface.
- `SimpleCcpSolver`.
- `OptionalSiconosSolver` adapter hook.

Current runtime behavior:

- If OpenVDB / NanoVDB are not detected, the SDF providers still run through an analytic narrow-band fallback that mimics a minimal level-set query path.
- If hpp-fcl / FCL are not detected, reference distance queries fall back to deterministic analytic geometry.
- If Siconos is not detected, the optional backend is reported as unavailable.
- If Siconos is detected in the future, the adapter hook is already present, but the current baseline still routes through the local solver and emits a note saying so. The real fc3d integration is a next-step replacement point.

## Architecture

```text
parameterized geometry
    |
    +--> ReferenceGeometry -----------------------------+
    |                                                   |
    +--> SdfProvider (OpenVDB / NanoVDB / fallback)     |
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
                              ContactSolver interface / solver backends
                                   |                     |
                                   |                     +--> OptionalSiconosSolver
                                   |
                                   +--> SimpleCcpSolver
                                                        |
                                                        v
                                           CSV / JSON / Markdown outputs
```

The important separations are:

- candidate-pair management vs precise contact geometry,
- contact geometry construction vs contact solve,
- SDF-based contact queries vs reference geometry queries,
- optional heavy dependencies vs the always-runnable fallback path.

## Repository Layout

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/
├── third_party/
├── include/
│   └── baseline/
├── src/
│   ├── core/
│   ├── sdf/
│   ├── contact/
│   ├── solver/
│   └── apps/
├── python/
│   └── baseline/
├── examples/
├── tests/
├── data/
├── outputs/
└── scripts/
```

## Dependency Strategy

The project is designed so the baseline still compiles with only a C++17 compiler, CMake, and Python.
Heavy libraries are optional and isolated behind small interfaces.

### WSL / Ubuntu 22.04 or 24.04

Preferred order:

1. `apt` for `build-essential`, `cmake`, `ninja-build`, `python3`, `pytest`, `libeigen3-dev`.
2. `apt` for optional `libopenvdb-dev`, `libfcl-dev`, `libhpp-fcl-dev`, `libtbb-dev` when available.
3. Keep Siconos optional and off by default.

Notes:

- `scripts/bootstrap_wsl.sh` installs the common packages and opportunistically installs optional ones if they exist in the active apt sources.
- For WSL builds, keep the build tree inside the Linux filesystem when possible.
- Avoid high-frequency builds under `/mnt/c/...`; it is noticeably slower than building under `~/...` or another native WSL path.

### Windows 11 Native

Preferred order:

1. MSVC + CMake.
2. Ninja + vcpkg if available.
3. If Ninja / vcpkg are not ready yet, the project still configures with the Visual Studio generator and zero heavy dependencies.

Notes:

- `scripts/bootstrap_windows.ps1` clones `vcpkg` into `third_party/vcpkg` and installs `eigen3` by default.
- Optional Windows packages are currently requested through classic `vcpkg install ...` in the script, not manifest mode. This is intentional: the heavy dependency mix is still host-dependent, and the baseline should not hard-fail on one missing optional port.
- The default Windows configure script uses `Visual Studio 17 2022` because it is the most robust fresh-host choice. If you already have Ninja and a developer shell, pass `-Generator Ninja`.

### Optional Dependency Detection

Configure-time detection is reported by CMake for:

- `Eigen3`
- `OpenVDB`
- `NanoVDB`
- `hpp-fcl`
- `FCL`
- `Siconos`

The baseline does not block on any of them.

## Build And Run

### Recommended WSL Flow

```bash
bash scripts/bootstrap_wsl.sh
bash scripts/configure_wsl.sh wsl-release
bash scripts/build_wsl.sh wsl-release
bash scripts/run_all_examples_wsl.sh
```

Manual WSL flow:

```bash
cmake --preset wsl-release
cmake --build --preset wsl-release
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m baseline.run_example ex01_nanovdb_hello
```

### Windows Native Flow

PowerShell scripts:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/build_windows.ps1 -Config Release
powershell -ExecutionPolicy Bypass -File scripts/run_all_examples_windows.ps1 -Config Release
```

If you also want `vcpkg` bootstrapped first:

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

### CMake Presets

Provided configure presets:

- `wsl-debug`
- `wsl-release`
- `windows-debug`
- `windows-release`

Provided build presets use the same names.

## Python Entry Points

Examples are exposed through:

```bash
python -m baseline.run_example ex01_nanovdb_hello
python -m baseline.run_example ex02_hppfcl_distance
python -m baseline.run_example ex03_dual_sdf_gap
python -m baseline.run_example ex04_single_step_contact
python -m baseline.run_example ex05_compare_backends
python -m baseline.run_example ex06_regression_smoke
```

The Python layer does not require `pybind11`.
It simply resolves the built executable and launches it through `subprocess`.

## Examples

### `ex01_nanovdb_hello`

- Builds a simple sphere level set through `NanoVdbSdfProvider`.
- Samples SDF values and gradients at a few points.
- Writes `outputs/ex01_nanovdb_hello/samples.csv` and `summary.json`.

### `ex02_hppfcl_distance`

- Runs a reference geometry distance / collision query for sphere-box cases.
- Uses the hpp-fcl / FCL adapter layer when detected, otherwise analytic fallback.
- Writes `outputs/ex02_hppfcl_distance/distance_cases.csv` and `summary.json`.

### `ex03_dual_sdf_gap`

- Builds two simple SDF objects.
- Computes symmetric signed gap, contact normal, tangential basis, and support points.
- Writes `outputs/ex03_dual_sdf_gap/contact.csv` and `contact.json`.

### `ex04_single_step_contact`

- Builds a sphere-plane single-step contact problem.
- Solves it with `SimpleCcpSolver`.
- Reports the optional Siconos backend state.
- Writes `outputs/ex04_single_step_contact/solver_results.csv` and `summary.json`.

### `ex05_compare_backends`

- Compares reference geometry distance vs dual-SDF gap.
- Compares `SimpleCcpSolver` vs `OptionalSiconosSolver`.
- Writes `outputs/ex05_compare_backends/summary.csv` and `report.md`.

### `ex06_regression_smoke`

- Runs a tiny end-to-end smoke executable.
- Writes `outputs/ex06_regression_smoke/smoke.txt`.

## Tests

C++ tests:

- `test_sdf_sampling`
- `test_dual_sdf_gap_symmetry`
- `test_contact_frame_orthogonality`
- `test_simple_ccp_solver_smoke`
- `test_siconos_backend_smoke` when Siconos is detected

Python smoke tests:

- `tests/test_python_smoke.py`

Run them with:

```bash
ctest --output-on-failure
python -m pytest tests/test_python_smoke.py
```

## Outputs

All baseline outputs are written under:

- `outputs/ex01_nanovdb_hello/`
- `outputs/ex02_hppfcl_distance/`
- `outputs/ex03_dual_sdf_gap/`
- `outputs/ex04_single_step_contact/`
- `outputs/ex05_compare_backends/`
- `outputs/ex06_regression_smoke/`

You can override the root output directory with the `BASELINE_OUTPUT_ROOT` environment variable.

## Switching Backends

### Simple Solver vs Optional Siconos

- `SimpleCcpSolver` is always available.
- `OptionalSiconosSolver` is controlled by `WITH_SICONOS`.
- Configure with `-DWITH_SICONOS=ON` if you want CMake to probe for Siconos.
- In the current baseline, the Siconos adapter is a stable interface hook plus reporting path. The actual fc3d backend call is intentionally left as a future replacement point.

### hpp-fcl vs FCL vs Analytic Fallback

- The reference geometry query layer tries to detect `hpp-fcl`.
- If `hpp-fcl` is not found, it can still detect plain `FCL`.
- If neither is found, the project falls back to deterministic analytic distance queries for the simple baseline shapes.
- This keeps `ex02` and `ex05` runnable even on a bare Windows machine.

### OpenVDB / NanoVDB vs Fallback SDF

- `OpenVdbSdfProvider` and `NanoVdbSdfProvider` already exist as named adapter classes.
- If the actual libraries are not detected, both classes still expose the same interface but run through an analytic narrow-band fallback.
- This means downstream contact code can be developed now and upgraded later without changing the example entrypoints.

## Replacing The Baseline SDF With Your Own Code

The most direct replacement path is:

1. Keep the `SdfProvider` interface unchanged.
2. Replace the internals of `OpenVdbSdfProvider` and/or `NanoVdbSdfProvider` so `sample()` and `referencePoint()` come from your real grid objects.
3. Leave `DualSdfContactCalculator`, `ContactProblemBuilder`, and the examples intact while validating the new backend.

If your own OpenVDB pipeline already has a richer object model, keep a thin adapter layer in `src/sdf/` rather than pushing OpenVDB-specific details into `src/contact/`.

## Current Limitations

- Fallback reference geometry is axis-aligned and intentionally simple.
- The narrow-band SDF providers are analytic stand-ins unless external libraries are detected and you replace the adapter internals.
- The solver layer is single-contact and baseline-oriented, not a full NSGS / APGD / fc3d production implementation.
- No GPU path is included in the first version.
- macOS is not a priority target.

## Baseline Status

This repository is meant to be the experimental floor, not the final paper codebase.

Today it gives you:

- a clean compile/run loop,
- reproducible examples,
- a stable reporting format,
- a place to plug in your own SDF and CCP implementations next.
