# Migrate To WSL

This document is now the stage-2 WSL status note, not a future placeholder.

The repository has already been advanced onto a WSL2 Ubuntu path with real backend integration priorities:

- real SDF backend first
- real reference backend second
- solver backend optional

## Current WSL Integration Status

Validated on WSL2 Ubuntu 22.04:

- `gcc`
- `cmake`
- `ninja`
- `python3`
- `pytest`

Dependencies currently detected in the validated path:

- `Eigen3`
- `OpenVDB`
  - detected through local `libopenvdb-dev` header extraction plus system library linking
- `FCL`
  - detected from system package / CMake config

Dependencies currently not detected in the validated path:

- `NanoVDB`
- `hpp-fcl`
- `Siconos`

## What Is Actually Real On WSL Now

Real backend paths:

1. `OpenVdbSdfProvider`
   - real sphere / box level-set generation
   - real world-space sampling inside the trusted narrow band
   - explicit narrow-band handling

2. `FclReferenceBackend`
   - real `fcl::distance`
   - real `fcl::collide`
   - nearest points and collision normal for primitive cases

Still optional / fallback:

- `NanoVdbSdfProvider`
- `HppFclReferenceBackend`
- `OptionalSiconosSolver`

## Examples Running On Real Backends

Validated examples on the current WSL path:

- `ex01_nanovdb_hello`
  - real `OpenVDB`
- `ex02_hppfcl_distance`
  - real `FCL`
- `ex03_dual_sdf_gap`
  - analytic vs real `OpenVDB`
- `ex04_single_step_contact`
  - selected SDF backend can be `OpenVDB`
  - solver remains `SimpleCcpSolver`
- `ex05_compare_backends`
  - analytic reference vs real `FCL`
  - analytic SDF vs real `OpenVDB`
- `ex06_regression_smoke`
  - keeps the minimal end-to-end path intact

## Recommended WSL Workflow

Bootstrap:

```bash
bash scripts/bootstrap_wsl.sh
```

Configure and build:

```bash
bash scripts/configure_wsl.sh Release
bash scripts/build_wsl.sh Release
```

Run examples, CTest, and Python smoke:

```bash
bash scripts/run_all_examples_wsl.sh Release
```

Manual validation commands:

```bash
./build/wsl-release/bin/Release/ex01_nanovdb_hello
./build/wsl-release/bin/Release/ex02_hppfcl_distance
./build/wsl-release/bin/Release/ex03_dual_sdf_gap
./build/wsl-release/bin/Release/ex05_compare_backends
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m pytest tests/test_python_smoke.py -q
```

## Practical Notes

### OpenVDB

The stage-2 implementation uses this convention:

- queries are in world frame
- voxel size is in world units
- narrow-band width is expressed in voxel units
- the OpenVDB transform is a linear axis-aligned world/grid map

Because the current examples are primitive-backed:

- inside the trusted OpenVDB band, sampling is real OpenVDB
- outside that band, the provider explicitly falls back to the analytic primitive extension

This is deliberate.
Without that handling, center-point queries on truncated narrow-band grids would feed clipped distances into the dual-SDF formula.

### FCL

The current real reference backend covers:

- sphere-sphere
- sphere-box
- box-box

Unsupported shapes still fall back to the analytic backend logic.

## Best Next Steps

The next three modules worth spending time on are:

1. `NanoVDB`
   - add a real read-only query path from generated `OpenVDB` grids
2. `hpp-fcl`
   - add a real backend, then let WSL prefer it over `FCL`
3. richer SDF assets
   - mesh-to-level-set generation
   - file loading
   - benchmark cases beyond primitive pairs

## What Still Should Not Be The Main Focus

Not recommended as the immediate next priority:

1. `Siconos`
   - optional solver work is still lower value than geometry-layer validation
2. pybind11 bindings
   - the subprocess-based Python workflow is already sufficient for this stage
3. dynamic reconstruction / streaming SDF updates
   - not worth doing before static real-backend benchmarks are stable

## Current Stage Summary

The project is no longer just a Windows fallback skeleton.

Stage-2 WSL now has:

- real `OpenVDB` SDF support
- real `FCL` reference support
- upgraded benchmark-style examples
- backend-aware tests that pass or skip cleanly

The solver backend remains intentionally conservative in this phase.
