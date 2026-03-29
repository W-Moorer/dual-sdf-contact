#include "baseline/contact/contact_problem_builder.h"

namespace baseline {

ContactProblem ContactProblemBuilder::build(const ContactGeometry& geometry,
                                            const SingleStepContactParams& params) const {
  const auto local_velocity = toLocal(geometry.frame(), params.relative_velocity_world);
  const double effective_inverse_mass = params.inverse_mass_a + params.inverse_mass_b;
  ContactProblem problem;
  problem.geometry = geometry;
  problem.friction_coefficient = params.friction_coefficient;
  problem.label = params.label;
  problem.free_velocity = {
      local_velocity[0] + ((local_velocity[0] < 0.0) ? params.restitution * local_velocity[0] : 0.0),
      local_velocity[1],
      local_velocity[2],
  };
  problem.delassus = {{
      {{effective_inverse_mass, 0.0, 0.0}},
      {{0.0, effective_inverse_mass + params.tangential_regularization, 0.0}},
      {{0.0, 0.0, effective_inverse_mass + params.tangential_regularization}},
  }};
  return problem;
}

}  // namespace baseline
