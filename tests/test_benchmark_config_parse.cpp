#include <iostream>

#include "baseline/benchmark/benchmark_config.h"
#include "baseline/core/io_utils.h"

int main() {
  using namespace baseline;

  const auto primitive_config = loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "primitive_smoke.json");
  if (primitive_config.benchmark_name != "primitive_smoke" || primitive_config.case_family != "primitive") {
    std::cerr << "Failed to parse primitive_smoke benchmark config.\n";
    return 1;
  }
  if (expandBenchmarkSamples(primitive_config).size() < 3) {
    std::cerr << "primitive_smoke should expand to at least three samples.\n";
    return 1;
  }

  const auto resolution_config = loadBenchmarkConfig(sourceDir() / "configs" / "benchmarks" / "resolution_sweep.json");
  if (resolution_config.case_family != "resolution_sweep") {
    std::cerr << "Failed to parse resolution_sweep benchmark config.\n";
    return 1;
  }
  if (expandBenchmarkSamples(resolution_config).size() < 9) {
    std::cerr << "resolution_sweep should expand voxel/narrow-band combinations.\n";
    return 1;
  }
  return 0;
}
