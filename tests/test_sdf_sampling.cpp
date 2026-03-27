#include <cmath>
#include <iostream>

#include "baseline/sdf/nanovdb_sdf_provider.h"

int main() {
  using namespace baseline;

  const NanoVdbSdfProvider provider =
      NanoVdbSdfProvider::makeSphere("test_sphere", {0.0, 0.0, 0.0}, 1.0, 0.05, 2.0);

  const SdfSample center = provider.sample({0.0, 0.0, 0.0});
  const SdfSample outside = provider.sample({1.5, 0.0, 0.0});

  if (std::abs(center.signed_distance + 1.0) > 1e-6) {
    std::cerr << "Unexpected center SDF value.\n";
    return 1;
  }
  if (std::abs(outside.signed_distance - 0.5) > 1e-6) {
    std::cerr << "Unexpected outside SDF value.\n";
    return 1;
  }
  if (std::abs(outside.gradient.x - 1.0) > 1e-3 || std::abs(outside.gradient.y) > 1e-3 ||
      std::abs(outside.gradient.z) > 1e-3) {
    std::cerr << "Unexpected outside SDF gradient.\n";
    return 1;
  }
  return 0;
}
