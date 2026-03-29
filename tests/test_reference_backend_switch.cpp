#include <cmath>
#include <iostream>

#include "baseline/contact/reference_geometry.h"
#include "baseline/runtime/backend_factory.h"

int main() {
  using namespace baseline;

  const auto analytic_backend = resolveReferenceBackend("analytic");
  const auto analytic_engine = makeReferenceBackend(analytic_backend);
  if (analytic_engine->name() != "analytic") {
    std::cerr << "Expected analytic reference backend.\n";
    return 1;
  }

  if (!FclReferenceBackend::realBackendAvailable()) {
    std::cout << "FCL backend unavailable; skipping.\n";
    return 77;
  }

  const auto fcl_backend = resolveReferenceBackend("fcl");
  if (fcl_backend.uses_fallback_execution) {
    std::cerr << "FCL backend should not be marked as fallback execution.\n";
    return 1;
  }

  const auto fcl_engine = makeReferenceBackend(fcl_backend);
  const ReferenceGeometry sphere = ReferenceGeometry::makeSphere("sphere", {0.0, 0.0, 0.0}, 0.8);
  const ReferenceGeometry box = ReferenceGeometry::makeBox("box", {1.15, 0.1, 0.0}, {0.4, 0.5, 0.4});

  const DistanceQueryResult analytic_result = analytic_engine->distance(sphere, box);
  const DistanceQueryResult fcl_result = fcl_engine->distance(sphere, box);

  if (fcl_engine->name() != "fcl-real") {
    std::cerr << "Expected real FCL backend name.\n";
    return 1;
  }
  if (analytic_result.collision != fcl_result.collision) {
    std::cerr << "Collision state mismatch between analytic and FCL.\n";
    return 1;
  }
  if (std::abs(analytic_result.signed_distance - fcl_result.signed_distance) > 2e-2) {
    std::cerr << "Signed distance mismatch between analytic and FCL.\n";
    return 1;
  }
  return 0;
}
