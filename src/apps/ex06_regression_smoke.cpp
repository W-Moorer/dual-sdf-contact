#include <iostream>

#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/runtime/backend_factory.h"
#include "example_utils.h"

int main(int argc, char** argv) {
  using namespace baseline;

  try {
    const std::string example_name = "ex06_regression_smoke";
    const auto options = apps::parseBackendOptions(argc, argv);
    if (options.help_requested) {
      apps::printUsage(example_name);
      return 0;
    }

    const auto sdf_backend = resolveSdfBackend(options.sdf_backend);
    const auto solver_backend = resolveSolverBackend(options.solver_backend);
    const auto output_dir = apps::outputDir(example_name);

    auto sphere_a = makeSdfProvider(
        sdf_backend,
        {"smoke_a", ReferenceGeometry::makeSphere("smoke_a", {0.0, 0.0, 0.0}, 1.0), 0.1, 2.0});
    auto sphere_b = makeSdfProvider(
        sdf_backend,
        {"smoke_b", ReferenceGeometry::makeSphere("smoke_b", {1.6, 0.0, 0.0}, 0.7), 0.1, 2.0});

    const DualSdfContactCalculator calculator;
    const ContactGeometry geometry = calculator.compute(*sphere_a, *sphere_b);

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
    auto solver = makeSolverBackend(solver_backend);
    const SolverResult result = solver->solve(problem);

    const bool passed = geometry.frameIsOrthonormal() && result.iterations > 0;
    writeTextFile(output_dir / "smoke.txt", passed ? "PASS\n" : "FAIL\n");

    std::cout << example_name << ": " << (passed ? "PASS" : "FAIL")
              << ", sdf_backend=" << sdf_backend.selected_name
              << ", solver_backend=" << solver_backend.selected_name << "\n";
    return passed ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "ex06_regression_smoke failed: " << error.what() << "\n";
    return 1;
  }
}
