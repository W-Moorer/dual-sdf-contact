# Migrate To WSL

This document is the handoff checklist for moving the current Windows-native fallback baseline onto a future WSL2 Ubuntu 22.04 / 24.04 device.

## What The Windows Baseline Already Gives You

Already completed on the current Windows-native host:

- CMake project and presets
- fallback analytic narrow-band SDF provider
- fallback analytic reference geometry backend
- `SimpleCcpSolver`
- backend factory / registry layer
- examples with backend reporting
- Python runner with backend flags
- tests for backend defaults, unavailable backend handling, and example backend reporting

This means the next WSL phase should focus on replacing internals, not redesigning project structure.

## First Steps On The Future WSL Device

Recommended order:

1. clone the repository into the Linux filesystem, not under `/mnt/c`
2. run `bash scripts/bootstrap_wsl.sh`
3. configure with `cmake --preset wsl-release`
4. build with `cmake --build --preset wsl-release`
5. run `bash scripts/run_all_examples_wsl.sh`
6. inspect the configure summary to see which dependencies were detected

## Recommended Install Order

Start with the lowest-risk path and validate incrementally:

1. `Eigen`
2. `OpenVDB / NanoVDB`
3. `hpp-fcl / FCL`
4. optional `Siconos`

Why this order:

- `Eigen` is low risk and unblocks general math cleanup later.
- `OpenVDB / NanoVDB` is the main SDF integration target.
- `hpp-fcl / FCL` is the main reference backend target.
- `Siconos` can come after geometry and SDF validation are already stable.

## Recommended First Validation Set On WSL

After the initial WSL build, validate in this order:

1. `ex01_nanovdb_hello`
2. `ex02_hppfcl_distance`
3. `ex03_dual_sdf_gap`
4. `ex04_single_step_contact`
5. `ex05_compare_backends`

Reasoning:

- `ex01` isolates SDF backend selection.
- `ex02` isolates reference backend selection.
- `ex03` checks dual-SDF contact geometry behavior.
- `ex04` checks the solver path.
- `ex05` checks cross-backend reporting and comparison output.

## Highest-Value Modules To Replace First

Prioritize these three modules:

1. `SdfProvider`
   Files:
   - `include/baseline/sdf/openvdb_sdf_provider.h`
   - `include/baseline/sdf/nanovdb_sdf_provider.h`
   - `src/sdf/openvdb_sdf_provider.cpp`
   - `src/sdf/nanovdb_sdf_provider.cpp`

2. reference backend layer
   Files:
   - `include/baseline/contact/reference_geometry.h`
   - `src/contact/reference_geometry.cpp`

3. real dual-SDF backend validation around contact geometry
   Files:
   - `include/baseline/contact/dual_sdf_contact_calculator.h`
   - `src/contact/dual_sdf_contact_calculator.cpp`

These three give the biggest experimental value for the least architecture churn.

## Modules That Can Wait

Useful later, but not first:

- `OptionalSiconosSolver`
- `pybind11`
- parallel batching / pair-loop optimization

Reason:

- solver replacement is more valuable once geometry and SDF backends are already trustworthy
- Python bindings are not required for the current subprocess-based workflow
- parallel optimization is premature before backend correctness is validated

## Configure Knobs To Watch On WSL

Relevant options:

- `BASELINE_WITH_OPENVDB`
- `BASELINE_WITH_NANOVDB`
- `BASELINE_WITH_HPP_FCL`
- `BASELINE_WITH_FCL`
- `BASELINE_WITH_SICONOS`
- `BASELINE_FORCE_FALLBACK_SDF`
- `BASELINE_FORCE_FALLBACK_REFERENCE`
- `BASELINE_FORCE_SIMPLE_SOLVER`

On WSL, the presets already disable the force-fallback knobs.
That means once the adapter internals are upgraded, examples can switch by configuration and factory selection instead of example rewrites.

## Practical Goal For The First WSL Week

Aim for this sequence:

1. keep all existing fallback tests green
2. get one real SDF adapter wired in
3. get one real reference backend wired in
4. re-run `ex05_compare_backends`
5. only then consider the optional Siconos path
