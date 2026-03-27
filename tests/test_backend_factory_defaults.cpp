#include <iostream>
#include <string>

#include "baseline/core/build_config.h"
#include "baseline/runtime/backend_factory.h"

int main() {
  using namespace baseline;

  const auto sdf_backend = resolveSdfBackend();
  const auto reference_backend = resolveReferenceBackend();
  const auto solver_backend = resolveSolverBackend();

  if (sdf_backend.selected_name != std::string(BASELINE_DEFAULT_SDF_BACKEND)) {
    std::cerr << "Unexpected default SDF backend.\n";
    return 1;
  }
  if (reference_backend.selected_name != std::string(BASELINE_DEFAULT_REFERENCE_BACKEND)) {
    std::cerr << "Unexpected default reference backend.\n";
    return 1;
  }
  if (solver_backend.selected_name != std::string(BASELINE_DEFAULT_SOLVER_BACKEND)) {
    std::cerr << "Unexpected default solver backend.\n";
    return 1;
  }
  return 0;
}
