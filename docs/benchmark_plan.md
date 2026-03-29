# Benchmark Plan

This note captures the current phase-6 benchmark coverage and the paper-oriented experiment matrix supported by the repository.

## Current Benchmark Matrix

The repository now maintains two paper-facing suites.

`paper_minimal`:

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
   - convex mesh-backed correctness sanity check
6. `mesh_nonconvex_smoke`
   - first nonconvex mesh sanity check
7. `mesh_gap_sweep`
   - mesh-backed gap and narrow-band sensitivity
8. `mesh_orientation_sweep`
   - first nonconvex mesh orientation stability
9. `mesh_resolution_sweep`
   - mesh-backed resolution / runtime ablation

`paper_extended`:

1. all `paper_minimal` benchmarks
2. `mesh_nonconvex_smoke_2`
   - second nonconvex mesh sanity check
3. `mesh_gap_sweep_2`
   - second nonconvex mesh gap and narrow-band sensitivity
4. `mesh_orientation_sweep_2`
   - second nonconvex mesh orientation stability

Use `paper_minimal` for fast regression and the smallest paper table set.
Use `paper_extended` for supplement-style mesh coverage, final camera-ready exports, and the paper asset bundle.

## Primitive, Convex Mesh, And Nonconvex Mesh Coverage

Primitive coverage:

- sphere-sphere
- sphere-box
- box-box

Convex mesh coverage:

- mesh-sphere
- mesh-box
- repository assets
  - `unit_cube.obj`
  - `unit_octahedron.obj`

Nonconvex mesh coverage:

- nonconvex mesh-sphere
- nonconvex mesh-box
- repository asset
  - `concave_l_block.obj`
  - `concave_u_block.obj`

Current mesh execution path:

- config parse
- triangle mesh load
- mesh category labeling
- OpenVDB mesh-to-level-set build
- FCL mesh-backed distance query
- analytic mesh proxy fallback where needed
- ex05 sample / summary / report output

## Benchmark Roles In The Paper

Suggested paper main results:

- `primitive_smoke`
  - primitive correctness table
- `resolution_sweep`
  - primitive accuracy / efficiency tradeoff
- `mesh_smoke`
  - convex mesh correctness table
- `mesh_resolution_sweep`
  - mesh accuracy / efficiency tradeoff
- `mesh_nonconvex_smoke`
  - first nonconvex mesh correctness sanity row
- `mesh_orientation_sweep`
  - main-text nonconvex pose stability row

Suggested supplementary results:

- `gap_sweep`
  - narrow-band sensitivity on primitives
- `orientation_sweep`
  - primitive orientation stability
- `mesh_gap_sweep`
  - mesh narrow-band sensitivity
- `mesh_nonconvex_smoke_2`
  - second nonconvex mesh sanity coverage
- `mesh_gap_sweep_2`
  - second nonconvex narrow-band sensitivity
- `mesh_orientation_sweep_2`
  - second nonconvex orientation stability

## Main Paper Metrics

Primary result metrics:

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
- `reference_warning_count`
- `reference_grazing_count`

Reference-quality diagnostics:

- `reference_signed_distance`
- `point_distance_consistency`
- `reference_mode`
- `reference_quality`
- `reference_diagnostic_label`
- `reference_source_label`
- `reference_point_distance_consistency`
- `reference_normal_alignment_residual`

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

- primitive vs convex mesh vs nonconvex mesh

Resolution ablations:

- coarse / medium / fine voxel sizes
- primitive and mesh coverage

Narrow-band ablations:

- at least three half-width settings in the main sweep configs
- primitive and mesh gap sweeps

Pose and gap ablations:

- deterministic orientation sweep
- seeded random orientation samples
- separation / near-contact / mild penetration
- primitive and nonconvex mesh coverage

## Orientation Sweep Role

Orientation sweeps are the main stability probe for the paper.

`orientation_sweep`:

- primitive box-box baseline
- useful for controlled tangent-frame and symmetry checks

`mesh_orientation_sweep`:

- nonconvex `concave_l_block.obj` vs box
- highlights normal stability under changing contact topology
- feeds the orientation plot exports:
  - `plot_normal_vs_orientation.csv`
  - `plot_runtime_vs_orientation.csv`

`mesh_orientation_sweep_2`:

- nonconvex `concave_u_block.obj` vs box
- provides a second contact-topology pattern for appendix validation
- useful for grazing-contact caution rows and `reference_diagnostic_label` inspection

## Current Table And Export Layers

Suite postprocess exports:

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
- `plot_normal_vs_orientation.csv`
- `plot_runtime_vs_orientation.csv`
- `plot_symmetry_vs_bandwidth.csv`

Camera-ready export layer:

- `camera_ready_table_accuracy.csv`
- `camera_ready_table_accuracy.md`
- `camera_ready_table_efficiency.csv`
- `camera_ready_table_efficiency.md`
- `camera_ready_table_ablation.csv`
- `camera_ready_table_ablation.md`
- `camera_ready_table_main_results.csv`
- `camera_ready_table_main_results.md`
- `camera_ready_table_appendix.csv`
- `camera_ready_table_appendix.md`
- `figure_manifest.json`

Final paper asset layer:

- `paper_assets_manifest.json`
- `appendix_assets_manifest.json`
- `docs/results_summary.md`

The final export layer is intentionally thin:

- it reads existing paper-table outputs
- it performs column selection, ordering, light renaming, and unit cleanup
- it does not replace the benchmark aggregation logic

## Result Labels And Traceability

Stable labels carried into sample, summary, and aggregate outputs include:

- `benchmark_name`
- `suite_name`
- `run_name`
- `case_family`
- `sweep_family`
- `case_group`
- `shape_a`
- `shape_b`
- `mesh_a`
- `mesh_b`
- `mesh_category`
- `sdf_backend`
- `reference_backend`
- `solver_backend`
- `reference_mode`
- `reference_quality`
- `reference_diagnostic_label`
- `voxel_size`
- `narrow_band_half_width`
- `sample_count`
- `seed`
- `orientation_index`
- `pose_index`

These labels are the contract that downstream paper tables and plotting scripts rely on.

## Main Text Vs Appendix Mapping

Recommended main-text outputs:

- suite: `paper_minimal`
- tables:
  - `camera_ready_table_main_results.csv`
  - `camera_ready_table_efficiency.csv`
  - `camera_ready_table_ablation.csv`
- figures:
  - `plot_gap_vs_resolution.csv`
  - `plot_runtime_vs_resolution.csv`
  - `plot_normal_vs_orientation.csv`
  - `plot_symmetry_vs_bandwidth.csv`

Recommended appendix outputs:

- suite: `paper_extended`
- tables:
  - `camera_ready_table_appendix.csv`
  - `camera_ready_table_accuracy.csv`
  - `camera_ready_table_ablation.csv`
- figures:
  - `plot_runtime_vs_orientation.csv`
  - `plot_symmetry_vs_bandwidth.csv`
- caution rows:
  - `reference_quality=low`
  - `reference_quality=grazing-caution`
  - nonzero `reference_grazing_count`
  - large `reference_normal_alignment_residual_mean`

## Freeze And Reproducibility

The current frozen paper profile is recorded in:

- `docs/experiment_freeze.md`
- suite-level `freeze_manifest.json`
- per-benchmark `freeze_manifest.json`
- `outputs/latest_freeze_manifest.json`

When git is available, the freeze metadata includes commit hash and dirty-state information.

## Suggested Next Extensions

Most valuable next steps after the current phase:

1. tighten mesh-point and mesh-normal reference consistency for difficult grazing contacts
2. add one more selective appendix export that groups only warning-heavy rows
3. add one more lightweight mesh with a different out-of-plane concavity pattern

## Not The Current Priority

Still not recommended as the immediate next step:

- deep `Siconos` integration
- `NanoVDB` mainline query path
- replacing `FCL` with `hpp-fcl`
- GUI or interactive visualization
- large-scale parallel benchmark infrastructure
