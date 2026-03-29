#pragma once

#include <cstdint>

#include <string>

#include "baseline/contact/reference_geometry.h"
#include "baseline/sdf/sdf_provider.h"

namespace baseline {

enum ContactValidityFlag : std::uint32_t {
  ContactValidNormal = 1u << 0,
  ContactValidTangents = 1u << 1,
  ContactQueryAInNarrowBand = 1u << 2,
  ContactQueryBInNarrowBand = 1u << 3,
  ContactNearNarrowBandBoundary = 1u << 4,
  ContactGradientsConsistent = 1u << 5,
  ContactUsedFallbackNormal = 1u << 6,
  ContactSupportPointsValid = 1u << 7,
};

struct ContactKinematicsResult {
  double signed_gap{0.0};
  Vec3 contact_point{0.0, 0.0, 0.0};
  Vec3 normal{1.0, 0.0, 0.0};
  Vec3 tangent1{0.0, 1.0, 0.0};
  Vec3 tangent2{0.0, 0.0, 1.0};
  Vec3 point_on_a{0.0, 0.0, 0.0};
  Vec3 point_on_b{0.0, 0.0, 0.0};
  double phi_a_at_query_b{0.0};
  double phi_b_at_query_a{0.0};
  double symmetry_residual{0.0};
  double tangent_orthogonality{0.0};
  double gradient_alignment{0.0};
  std::uint32_t valid_flags{0};
  std::string method;
  std::string object_a;
  std::string object_b;

  ContactFrame frame() const { return {normal, tangent1, tangent2}; }
  bool frameIsOrthonormal(double tolerance = 1e-6) const { return frame().isOrthonormal(tolerance); }
  bool hasFlag(ContactValidityFlag flag) const { return (valid_flags & static_cast<std::uint32_t>(flag)) != 0; }
};

using ContactGeometry = ContactKinematicsResult;

class DualSdfContactCalculator {
 public:
  ContactKinematicsResult compute(const SdfProvider& a, const SdfProvider& b) const;
  ContactKinematicsResult compute(const SdfProvider& a, const ReferenceGeometry& b) const;
};

}  // namespace baseline
