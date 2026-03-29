#include <iostream>

#include "baseline/benchmark/benchmark_config.h"
#include "baseline/core/io_utils.h"

int main() {
  using namespace baseline;

  const auto mesh_config = loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "mesh_smoke.json");
  if (mesh_config.case_family != "mesh" || mesh_config.sweep_family != "primitive") {
    std::cerr << "mesh_smoke should parse as case_family=mesh and sweep_family=primitive.\n";
    return 1;
  }
  if (!benchmarkConfigUsesMesh(mesh_config)) {
    std::cerr << "mesh_smoke should be detected as a mesh-backed config.\n";
    return 1;
  }

  const auto samples = expandBenchmarkSamples(mesh_config);
  if (samples.size() < 2) {
    std::cerr << "mesh_smoke should expand to at least two mesh-backed samples.\n";
    return 1;
  }
  if (samples.front().mesh_a.empty() || samples.front().shape_a != "mesh") {
    std::cerr << "mesh_smoke samples should carry mesh labels.\n";
    return 1;
  }
  if (samples.front().mesh_category != "convex" && samples.front().mesh_category != "nonconvex") {
    std::cerr << "mesh_smoke samples should carry mesh category labels.\n";
    return 1;
  }

  const auto orientation_config =
      loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "mesh_orientation_sweep.json");
  if (orientation_config.sweep_family != "orientation_sweep") {
    std::cerr << "mesh_orientation_sweep should parse as an orientation sweep.\n";
    return 1;
  }

  const auto nonconvex_config =
      loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "mesh_nonconvex_smoke.json");
  const auto nonconvex_samples = expandBenchmarkSamples(nonconvex_config);
  if (nonconvex_samples.empty() || nonconvex_samples.front().case_group != "mesh_nonconvex") {
    std::cerr << "mesh_nonconvex_smoke should produce mesh_nonconvex samples.\n";
    return 1;
  }

  const auto second_nonconvex_config =
      loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "mesh_nonconvex_smoke_2.json");
  const auto second_nonconvex_samples = expandBenchmarkSamples(second_nonconvex_config);
  if (second_nonconvex_samples.empty() || second_nonconvex_samples.front().mesh_a != "concave_u_block.obj") {
    std::cerr << "mesh_nonconvex_smoke_2 should produce samples for concave_u_block.obj.\n";
    return 1;
  }

  const auto second_orientation_config =
      loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "mesh_orientation_sweep_2.json");
  if (second_orientation_config.sweep_family != "orientation_sweep") {
    std::cerr << "mesh_orientation_sweep_2 should parse as an orientation sweep.\n";
    return 1;
  }

  return 0;
}
