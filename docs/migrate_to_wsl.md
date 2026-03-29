# Migrate To WSL

This document is now the stage-3 WSL status note.

## Current WSL Status

Validated on WSL2 Ubuntu 22.04:

- `gcc`
- `cmake`
- `ninja`
- `python3`
- `pytest`

Detected dependencies on the validated path:

- `Eigen3`
- `OpenVDB`
- `FCL`

Not detected on the validated path:

- `NanoVDB`
- `hpp-fcl`
- `Siconos`

## What Is Actually Real On WSL

Real benchmark-path backends:

1. `OpenVdbSdfProvider`
   - real sphere / box level-set generation
   - rotated primitive box support for the current benchmark path
   - real world-space sampling inside the trusted narrow band
2. `FclReferenceBackend`
   - real distance / collision queries
   - rotated box transforms through the current primitive benchmark path

Still optional / non-mainline:

- `NanoVdbSdfProvider`
- `HppFclReferenceBackend`
- `OptionalSiconosSolver`

## What Now Runs On The Real Benchmark Path

Validated examples:

- `ex01_nanovdb_hello`
- `ex02_hppfcl_distance`
- `ex03_dual_sdf_gap`
- `ex04_single_step_contact`
- `ex05_compare_backends`
- `ex06_regression_smoke`

Validated benchmark configs:

- `primitive_smoke`
- `gap_sweep`
- `orientation_sweep`
- `resolution_sweep`

Validated suite:

- `default`

## Recommended WSL Workflow

Configure and build:

```bash
bash scripts/configure_wsl.sh Release
bash scripts/build_wsl.sh Release
```

Functional smoke:

```bash
bash scripts/run_all_examples_wsl.sh Release
```

Single benchmark:

```bash
bash scripts/run_benchmark_wsl.sh Release configs/benchmarks/primitive_smoke.json
```

Default paper-benchmark smoke:

```bash
bash scripts/run_default_suite_wsl.sh Release
```

Manual validation:

```bash
PYTHONPATH=python python3 -m baseline.run_benchmark --config configs/benchmarks/primitive_smoke.json --build-config Release
PYTHONPATH=python python3 -m baseline.run_benchmark --suite default --build-config Release
ctest --test-dir build/wsl-release --output-on-failure
PYTHONPATH=python python3 -m pytest tests/test_python_smoke.py tests/test_benchmark_runner.py -q
```

## What The Stage-3 Benchmark Layer Adds

`ex05_compare_backends` now provides:

- config-driven benchmark execution
- sample-level CSV / JSON output
- aggregate summary CSV / JSON
- markdown report
- resolved config snapshot
- environment snapshot
- suite-level aggregate tables

This is the main WSL deliverable for current paper experiments.

## Best Next Steps

The next three modules worth extending are:

1. mesh-backed benchmark execution
   - the config model is ready
   - execution path is still intentionally blocked until primitive experiments are exhausted
2. richer benchmark postprocess
   - current CSV / JSON / markdown outputs are enough for plotting
   - next value is more opinionated table export for the paper
3. `NanoVDB` or `hpp-fcl`
   - only after the primitive benchmark matrix is stable

## What Still Should Not Be The Main Focus

Not recommended as the next priority:

1. `Siconos`
2. GUI or heavyweight visualization
3. large-scale parallel optimization
4. deep `hpp-fcl` work while `FCL` already supports the paper mainline
