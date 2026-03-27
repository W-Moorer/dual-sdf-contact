#pragma once

#include <string>

#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/solver/contact_problem.h"

namespace baseline {

struct SingleStepContactParams {
  double inverse_mass_a{1.0};
  double inverse_mass_b{0.0};
  Vec3 relative_velocity_world{0.0, -1.0, 0.0};
  double friction_coefficient{0.5};
  double restitution{0.0};
  double tangential_regularization{1e-4};
  std::string label{"single-step-contact"};
};

class ContactProblemBuilder {
 public:
  ContactProblem build(const ContactGeometry& geometry, const SingleStepContactParams& params) const;
};

}  // namespace baseline
