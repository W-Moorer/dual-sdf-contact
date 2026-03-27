#include <iostream>
#include <sstream>

#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/contact/reference_geometry.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "baseline/solver/simple_ccp_solver.h"
#include "baseline/solver/siconos_solver.h"
#include "example_utils.h"

int main() {
  using namespace baseline;

  const std::string example_name = "ex04_single_step_contact";
  const auto output_dir = apps::outputDir(example_name);

  const NanoVdbSdfProvider sphere =
      NanoVdbSdfProvider::makeSphere("dynamic_sphere", {0.0, 0.45, 0.0}, 0.5, 0.05, 2.0);
  const ReferenceGeometry plane = ReferenceGeometry::makePlane("ground_plane", {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

  const DualSdfContactCalculator calculator;
  const ContactGeometry geometry = calculator.compute(sphere, plane);
  const Vec3 relative_velocity_world =
      fromLocal(geometry.frame, {-1.2, 0.2, 0.1});

  const ContactProblemBuilder builder;
  const ContactProblem problem = builder.build(
      geometry,
      {
          1.0,
          0.0,
          relative_velocity_world,
          0.5,
          0.0,
          1e-4,
          "sphere-plane-single-step",
      });

  const SimpleCcpSolver simple_solver;
  const SolverResult simple_result = simple_solver.solve(problem);

  OptionalSiconosSolver siconos_solver;
  const bool run_siconos = siconos_solver.available();
  const SolverResult siconos_result = siconos_solver.solve(problem);

  apps::writeCsv(
      output_dir / "solver_results.csv",
      {"solver", "available", "impulse_n", "impulse_t0", "impulse_t1", "post_v_n", "post_v_t0", "post_v_t1",
       "iterations", "residual"},
      {
          {"SimpleCcpSolver",
           "true",
           formatDouble(simple_result.impulse[0]),
           formatDouble(simple_result.impulse[1]),
           formatDouble(simple_result.impulse[2]),
           formatDouble(simple_result.post_velocity[0]),
           formatDouble(simple_result.post_velocity[1]),
           formatDouble(simple_result.post_velocity[2]),
           std::to_string(simple_result.iterations),
           formatDouble(simple_result.residual)},
          {"OptionalSiconosSolver",
           boolToString(run_siconos),
           formatDouble(siconos_result.impulse[0]),
           formatDouble(siconos_result.impulse[1]),
           formatDouble(siconos_result.impulse[2]),
           formatDouble(siconos_result.post_velocity[0]),
           formatDouble(siconos_result.post_velocity[1]),
           formatDouble(siconos_result.post_velocity[2]),
           std::to_string(siconos_result.iterations),
           formatDouble(siconos_result.residual)},
      });

  std::ostringstream summary;
  summary << "{\n"
          << "  \"example\": " << quoteJson(example_name) << ",\n"
          << "  \"signed_gap\": " << formatDouble(problem.geometry.signed_gap) << ",\n"
          << "  \"normal\": " << apps::vec3Json(problem.geometry.frame.normal) << ",\n"
          << "  \"simple_solver\": {\n"
          << "    \"impulse\": " << apps::vector3Json(simple_result.impulse) << ",\n"
          << "    \"post_velocity\": " << apps::vector3Json(simple_result.post_velocity) << ",\n"
          << "    \"iterations\": " << simple_result.iterations << ",\n"
          << "    \"residual\": " << formatDouble(simple_result.residual) << "\n"
          << "  },\n"
          << "  \"optional_siconos\": {\n"
          << "    \"available\": " << boolToString(run_siconos) << ",\n"
          << "    \"note\": " << quoteJson(siconos_result.note) << "\n"
          << "  }\n"
          << "}\n";
  writeTextFile(output_dir / "summary.json", summary.str());

  std::cout << example_name << ": gap=" << problem.geometry.signed_gap
            << ", normal_impulse=" << simple_result.impulse[0]
            << ", siconos_available=" << boolToString(run_siconos) << "\n";
  return 0;
}
