#include <cmath>
#include <iostream>

#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/runtime/backend_factory.h"
#include "baseline/sdf/openvdb_sdf_provider.h"

int main() {
  using namespace baseline;

  if (!OpenVdbSdfProvider::realBackendAvailable()) {
    std::cout << "OpenVDB backend unavailable; skipping.\n";
    return 77;
  }

  const auto analytic_backend = resolveSdfBackend("analytic");
  const auto openvdb_backend = resolveSdfBackend("openvdb");

  const ReferenceGeometry sphere_a = ReferenceGeometry::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 1.0);
  const ReferenceGeometry sphere_b = ReferenceGeometry::makeSphere("sphere_b", {1.65, 0.05, 0.0}, 0.75);

  auto analytic_a = makeSdfProvider(analytic_backend, {"analytic_a", sphere_a, 0.05, 4.0});
  auto analytic_b = makeSdfProvider(analytic_backend, {"analytic_b", sphere_b, 0.05, 4.0});
  auto openvdb_a = makeSdfProvider(openvdb_backend, {"openvdb_a", sphere_a, 0.05, 4.0});
  auto openvdb_b = makeSdfProvider(openvdb_backend, {"openvdb_b", sphere_b, 0.05, 4.0});

  const DualSdfContactCalculator calculator;
  const ContactKinematicsResult analytic_result = calculator.compute(*analytic_a, *analytic_b);
  const ContactKinematicsResult openvdb_result = calculator.compute(*openvdb_a, *openvdb_b);

  const double normal_angle_cosine = clamp(dot(analytic_result.normal, openvdb_result.normal), -1.0, 1.0);
  const double normal_angle_error = std::acos(normal_angle_cosine);

  if (std::abs(analytic_result.signed_gap - openvdb_result.signed_gap) > 7.5e-2) {
    std::cerr << "OpenVDB dual-SDF gap diverges from analytic baseline.\n";
    return 1;
  }
  if (normal_angle_error > 0.1) {
    std::cerr << "OpenVDB normal diverges from analytic baseline.\n";
    return 1;
  }
  if (!openvdb_result.frameIsOrthonormal()) {
    std::cerr << "OpenVDB contact frame is not orthonormal.\n";
    return 1;
  }
  return 0;
}
