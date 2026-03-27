#include <iostream>

#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "baseline/solver/siconos_solver.h"

int main() {
  using namespace baseline;

  const NanoVdbSdfProvider a = NanoVdbSdfProvider::makeSphere("a", {0.0, 0.0, 0.0}, 1.0, 0.05, 2.0);
  const NanoVdbSdfProvider b = NanoVdbSdfProvider::makeSphere("b", {1.6, 0.0, 0.0}, 0.7, 0.05, 2.0);

  const DualSdfContactCalculator calculator;
  const ContactGeometry geometry = calculator.compute(a, b);
  const ContactProblemBuilder builder;
  const ContactProblem problem = builder.build(geometry, {});

  OptionalSiconosSolver solver;
  const SolverResult result = solver.solve(problem);
  if (result.solver_name.empty() || result.note.empty()) {
    std::cerr << "Expected a populated adapter result.\n";
    return 1;
  }
  return 0;
}
