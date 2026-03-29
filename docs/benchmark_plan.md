# Benchmark Plan

This note captures the current phase-4 benchmark coverage and the minimum paper experiment matrix supported by the repository.

## Current Paper-Minimal Matrix

The current `paper_minimal` suite contains:

1. `primitive_smoke`
   - primitive correctness sanity check
   - sphere-sphere, sphere-box, box-box
2. `gap_sweep`
   - primitive gap and narrow-band sensitivity
3. `orientation_sweep`
   - primitive box-box pose stability
4. `resolution_sweep`
   - primitive resolution / runtime ablation
5. `mesh_smoke`
   - mesh-backed correctness sanity check
   - mesh-sphere, mesh-box
6. `mesh_gap_sweep`
   - mesh-backed gap and narrow-band sensitivity
7. `mesh_resolution_sweep`
   - mesh-backed resolution / runtime ablation

## Primitive And Mesh Coverage

Primitive benchmark coverage:

- sphere-sphere
- sphere-box
- box-box

Mesh benchmark coverage:

- mesh-sphere
- mesh-box
- repository-local OBJ assets in `data/meshes/`
  - `unit_cube.obj`
  - `unit_octahedron.obj`

Current mesh execution path:

- config parse
- triangle mesh load
- `ReferenceGeometry` mesh-backed analytic fallback
- OpenVDB mesh-to-level-set build
- FCL mesh-backed distance query
- ex05 sample / summary / report output

## Main Paper Metrics

Primary metrics:

- `absolute_gap_error`
- `normal_angle_error_deg`
- `symmetry_residual`
- `runtime_total_us_mean`
- `runtime_total_us_p95`

Primary stability diagnostics:

- `invalid_result_count`
- `degenerate_normal_count`
- `tangent_frame_fallback_count`
- `narrow_band_edge_hit_count`

Reference and consistency metrics:

- `reference_signed_distance`
- `gap_error`
- `relative_gap_error`
- `point_distance_consistency`

Solver-side diagnostics:

- `normal_impulse`
- `tangential_impulse_magnitude`
- `solver_residual`
- `solver_iterations`
- `solver_success_rate`

## Supported Ablations

Backend ablations:

- `analytic` vs `openvdb` SDF
- `analytic` vs `fcl` reference

Geometry-family ablations:

- primitive vs mesh-backed benchmark cases

Resolution ablations:

- coarse / medium / fine voxel sizes

Narrow-band ablations:

- at least three half-width settings in the main sweep configs

Pose and gap ablations:

- deterministic orientation sweep
- seeded random pose samples
- separation / near-contact / mild penetration

## Current Table And Plot Export

The suite postprocess currently exports:

- `aggregate_summary.csv`
- `aggregate_summary.json`
- `aggregate_table.md`
- `paper_table_accuracy.csv`
- `paper_table_accuracy.md`
- `paper_table_efficiency.csv`
- `paper_table_efficiency.md`
- `paper_table_ablation.csv`
- `paper_table_ablation.md`
- `plot_gap_vs_resolution.csv`
- `plot_runtime_vs_resolution.csv`
- `plot_symmetry_vs_bandwidth.csv`

These outputs are intended to be stable enough for:

- manual paper table drafting
- Python plotting scripts outside the repo
- appendix-style result inspection

## Recommended First Figures And Tables

Most useful immediate paper artifacts:

1. Accuracy table
   - use `paper_table_accuracy.csv`
   - highlight `absolute_gap_error`, `normal_angle_error_deg`, `symmetry_residual`
2. Efficiency table
   - use `paper_table_efficiency.csv`
   - highlight `dual_sdf_runtime_us_mean`, `runtime_total_us_mean`, `runtime_total_us_p95`
3. Resolution figure
   - use `plot_gap_vs_resolution.csv`
   - pair with `plot_runtime_vs_resolution.csv`
4. Narrow-band stability figure
   - use `plot_symmetry_vs_bandwidth.csv`

## Suggested Next Extensions

Most valuable next steps after the current phase:

1. richer mesh asset coverage
   - a second non-convex but lightweight mesh
   - one more paper-friendly mesh-box pose sweep
2. more explicit benchmark naming conventions for larger suites
3. small plotting helpers tailored to the final paper figure set

## Not The Current Priority

Still not recommended as the immediate next step:

- deep `Siconos` integration
- `NanoVDB` mainline query path
- replacing `FCL` with `hpp-fcl`
- GUI or interactive visualization
- large-scale parallel benchmark infrastructure
