# Benchmark Plan

This note captures the current benchmark coverage and the minimum paper experiment matrix supported by the repository.

## Current Coverage

Current benchmark case families:

- `primitive`
  - sphere-sphere
  - sphere-box
  - box-box
- `gap_sweep`
  - separation
  - near-contact
  - mild penetration
- `orientation_sweep`
  - deterministic yaw sweep
  - seeded random yaw samples
- `resolution_sweep`
  - coarse / medium / fine voxel sizes
  - multiple narrow-band widths

Current backend comparisons:

- SDF
  - `analytic`
  - `openvdb`
- reference
  - `analytic`
  - `fcl`
- solver
  - `simple`

## Suggested Minimum Paper Matrix

Recommended first-pass matrix:

1. `primitive_smoke`
   - sanity-check backend wiring and output schema
2. `gap_sweep`
   - use for gap error, symmetry residual, and narrow-band sensitivity
3. `orientation_sweep`
   - use for normal error and pose robustness
4. `resolution_sweep`
   - use for accuracy / runtime tradeoff figures

## Supported Ablations

Current ablations already supported:

- backend ablation
  - `analytic` vs `openvdb`
  - `analytic` vs `fcl`
- voxel resolution ablation
- narrow-band width ablation
- pose ablation
- gap ablation

## Mesh Extension Path

The config and case model already reserve a mesh path:

- `BenchmarkShapeSpec.type = "mesh"`
- `mesh_path` in config
- dedicated `mesh` case family placeholder

Current status:

- parsing is present
- execution is intentionally blocked
- primitive cases remain the mainline until the paper baseline is stable

## What To Extend Next

Best next extensions:

1. mesh-backed `OpenVDB` generation for one or two small meshes
2. richer suite postprocess and table export for the paper appendix
3. stricter benchmark grouping / naming conventions if the experiment matrix grows

## What Not To Prioritize Yet

Not recommended as the immediate next step:

- `Siconos`
- GUI
- large-scale parallel benchmarking
- deep `hpp-fcl` work while `FCL` already supports the primitive paper path
