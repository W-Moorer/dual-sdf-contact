#pragma once

#include <string>

#include "baseline/contact/reference_geometry.h"
#include "baseline/sdf/sdf_provider.h"

namespace baseline {

struct ContactGeometry {
  double signed_gap{0.0};
  ContactFrame frame;
  Vec3 contact_point{0.0, 0.0, 0.0};
  Vec3 support_point_a{0.0, 0.0, 0.0};
  Vec3 support_point_b{0.0, 0.0, 0.0};
  std::string method;
  std::string object_a;
  std::string object_b;
};

class DualSdfContactCalculator {
 public:
  ContactGeometry compute(const SdfProvider& a, const SdfProvider& b) const;
  ContactGeometry compute(const SdfProvider& a, const ReferenceGeometry& b) const;
};

}  // namespace baseline
