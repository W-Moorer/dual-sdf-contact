#include <functional>
#include <iostream>

#include "baseline/runtime/backend_factory.h"

int main() {
  using namespace baseline;

  const BackendAvailabilitySummary availability = queryBackendAvailability();

  auto expect_throw = [](const std::function<void()>& fn, const char* label) -> bool {
    try {
      fn();
    } catch (const BackendSelectionError&) {
      return true;
    }
    std::cerr << "Expected backend selection error for " << label << ".\n";
    return false;
  };

  if (!availability.openvdb_available || availability.force_fallback_sdf) {
    return expect_throw([] { (void)resolveSdfBackend("openvdb"); }, "openvdb") ? 0 : 1;
  }
  if (!availability.hppfcl_available || availability.force_fallback_reference) {
    return expect_throw([] { (void)resolveReferenceBackend("hppfcl"); }, "hppfcl") ? 0 : 1;
  }
  if (!availability.siconos_available || availability.force_simple_solver) {
    return expect_throw([] { (void)resolveSolverBackend("siconos"); }, "siconos") ? 0 : 1;
  }
  return expect_throw([] { (void)resolveSdfBackend("definitely-not-a-backend"); }, "invalid-token") ? 0 : 1;
}
