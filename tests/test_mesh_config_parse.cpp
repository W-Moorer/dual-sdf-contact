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

  return 0;
}
