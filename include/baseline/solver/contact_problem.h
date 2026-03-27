#pragma once

#include <array>
#include <string>

#include "baseline/contact/dual_sdf_contact_calculator.h"

namespace baseline {

using Vector3 = std::array<double, 3>;
using Matrix3 = std::array<std::array<double, 3>, 3>;

struct ContactProblem {
  Matrix3 delassus{{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}};
  Vector3 free_velocity{{0.0, 0.0, 0.0}};
  double friction_coefficient{0.5};
  ContactGeometry geometry;
  std::string label;
};

struct SolverResult {
  bool converged{false};
  Vector3 impulse{{0.0, 0.0, 0.0}};
  Vector3 post_velocity{{0.0, 0.0, 0.0}};
  int iterations{0};
  double residual{0.0};
  std::string solver_name;
  std::string note;
};

}  // namespace baseline
