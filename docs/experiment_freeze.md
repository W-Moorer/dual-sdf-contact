# Experiment Freeze

This note records the current paper-sprint benchmark freeze used by the repository.

## Active WSL Mainline

- SDF backend: `openvdb`
- reference backend: `fcl`
- solver backend: `simple`
- Windows fallback remains:
  - `analytic` SDF
  - `analytic` reference
  - `simple` solver

## Recommended Freeze Settings

- freeze name: `paper_experiment_freeze_v1`
- default main-text seed: `17`
- recommended voxel sizes:
  - coarse: `0.16`
  - medium: `0.08`
  - fine: `0.04`
- recommended narrow-band half widths:
  - tight: `2.0`
  - default: `4.0`
  - wide: `8.0`

## Recommended Suites

- `paper_minimal`
  - main-text matrix
  - faster regression and figure/table refresh
- `paper_extended`
  - appendix and supplementary mesh matrix
  - includes the second nonconvex mesh case

## Traceability

Each suite run now emits:

- `freeze_manifest.json`
- per-benchmark `freeze_manifest.json`
- `outputs/latest_freeze_manifest.json`

Each benchmark directory also carries the freeze object inside:

- `config_resolved.json`
- `environment.json`

When git is available, the freeze metadata records:

- `commit_hash`
- `commit_short`
- `dirty`
- a short `status_summary`
