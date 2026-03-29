#include <cmath>
#include <iostream>

#include "baseline/sdf/openvdb_sdf_provider.h"

int main() {
  using namespace baseline;

  if (!OpenVdbSdfProvider::realBackendAvailable()) {
    std::cout << "OpenVDB backend unavailable; skipping.\n";
    return 77;
  }

  const OpenVdbSdfProvider provider =
      OpenVdbSdfProvider::makeSphere("test_openvdb_sphere", {0.0, 0.0, 0.0}, 1.0, 0.05, 4.0);

  const SdfSample center = provider.samplePhiGrad({0.0, 0.0, 0.0});
  const SdfSample near_surface = provider.samplePhiGrad({1.05, 0.0, 0.0});
  const SdfSample far = provider.samplePhiGrad({1.5, 0.0, 0.0});

  if (provider.backendName() != "openvdb-real") {
    std::cerr << "Expected real OpenVDB backend.\n";
    return 1;
  }
  if (!provider.worldAabb().valid) {
    std::cerr << "OpenVDB world AABB should be valid.\n";
    return 1;
  }
  if (std::abs(center.signed_distance + 1.0) > 5e-2) {
    std::cerr << "Unexpected center signed distance.\n";
    return 1;
  }
  if (std::abs(near_surface.signed_distance - 0.05) > 7.5e-2) {
    std::cerr << "Unexpected near-surface signed distance.\n";
    return 1;
  }
  if (!near_surface.in_narrow_band || far.in_narrow_band) {
    std::cerr << "Unexpected narrow-band classification.\n";
    return 1;
  }
  if (std::abs(near_surface.gradient.x - 1.0) > 0.1 || std::abs(near_surface.gradient.y) > 0.1 ||
      std::abs(near_surface.gradient.z) > 0.1) {
    std::cerr << "Unexpected near-surface gradient.\n";
    return 1;
  }
  return 0;
}
