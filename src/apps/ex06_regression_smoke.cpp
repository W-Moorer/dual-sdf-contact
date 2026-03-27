#include <iostream>

#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/contact/reference_geometry.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "baseline/solver/simple_ccp_solver.h"
#include "baseline/contact/contact_problem_builder.h"
#include "example_utils.h"

int main() {
  using namespace baseline;

  const std::string example_name = "ex06_regression_smoke";
  const auto output_dir = apps::outputDir(example_name);

  const NanoVdbSdfProvider sphere_a =
      NanoVdbSdfProvider::makeSphere("smoke_a", {0.0, 0.0, 0.0}, 1.0, 0.1, 2.0);
  const NanoVdbSdfProvider sphere_b =
      NanoVdbSdfProvider::makeSphere("smoke_b", {1.6, 0.0, 0.0}, 0.7, 0.1, 2.0);

  const DualSdfContactCalculator calculator;
  const ContactGeometry geometry = calculator.compute(sphere_a, sphere_b);

  const ContactProblemBuilder builder;
  const ContactProblem problem = builder.build(
      geometry,
      {
          1.0,
          0.0,
          {0.0, -0.5, 0.0},
          0.3,
          0.0,
          1e-4,
          "smoke-contact",
      });
  const SimpleCcpSolver solver;
  const SolverResult result = solver.solve(problem);

  const bool passed = geometry.frame.isOrthonormal() && result.iterations > 0;
  writeTextFile(output_dir / "smoke.txt", passed ? "PASS\n" : "FAIL\n");

  std::cout << example_name << ": " << (passed ? "PASS" : "FAIL") << "\n";
  return passed ? 0 : 1;
}
