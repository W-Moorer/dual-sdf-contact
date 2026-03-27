#include <cmath>
#include <iostream>

#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "baseline/solver/simple_ccp_solver.h"

int main() {
  using namespace baseline;

  const NanoVdbSdfProvider a = NanoVdbSdfProvider::makeSphere("a", {0.0, 0.0, 0.0}, 1.0, 0.05, 2.0);
  const NanoVdbSdfProvider b = NanoVdbSdfProvider::makeSphere("b", {1.6, 0.0, 0.0}, 0.7, 0.05, 2.0);

  const DualSdfContactCalculator calculator;
  const ContactGeometry geometry = calculator.compute(a, b);
  const ContactProblemBuilder builder;
  const ContactProblem problem = builder.build(
      geometry,
      {
          1.0,
          0.0,
          {-0.8, 0.15, -0.05},
          0.4,
          0.0,
          1e-4,
          "solver-smoke",
      });

  const SimpleCcpSolver solver;
  const SolverResult result = solver.solve(problem);

  if (result.impulse[0] <= 0.0) {
    std::cerr << "Normal impulse should be positive.\n";
    return 1;
  }
  if (result.post_velocity[0] < -1e-6) {
    std::cerr << "Post-contact normal velocity is still negative.\n";
    return 1;
  }
  const double friction_box_bound = problem.friction_coefficient * result.impulse[0] / std::sqrt(2.0);
  if (std::abs(result.impulse[1]) > friction_box_bound + 1e-8 ||
      std::abs(result.impulse[2]) > friction_box_bound + 1e-8) {
    std::cerr << "Tangential impulse violates the linearized friction box.\n";
    return 1;
  }
  return 0;
}
