#include <iostream>
#include <sstream>

#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/runtime/backend_factory.h"
#include "baseline/solver/simple_ccp_solver.h"
#include "example_utils.h"

int main(int argc, char** argv) {
  using namespace baseline;

  try {
    const std::string example_name = "ex04_single_step_contact";
    const auto options = apps::parseBackendOptions(argc, argv);
    if (options.help_requested) {
      apps::printUsage(example_name);
      return 0;
    }

    const auto sdf_backend = resolveSdfBackend(options.sdf_backend);
    const auto solver_backend = resolveSolverBackend(options.solver_backend);
    const auto output_dir = apps::outputDir(example_name);

    auto sphere = makeSdfProvider(
        sdf_backend,
        {"dynamic_sphere", ReferenceGeometry::makeSphere("dynamic_sphere", {0.0, 0.45, 0.0}, 0.5), 0.05, 2.0});
    const ReferenceGeometry plane =
        ReferenceGeometry::makePlane("ground_plane", {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    const DualSdfContactCalculator calculator;
    const ContactGeometry geometry = calculator.compute(*sphere, plane);
    const Vec3 relative_velocity_world = fromLocal(geometry.frame, {-1.2, 0.2, 0.1});

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
    auto selected_solver = makeSolverBackend(solver_backend);
    const SolverResult selected_result = selected_solver->solve(problem);

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
            {selected_solver->name(),
             boolToString(selected_solver->available()),
             formatDouble(selected_result.impulse[0]),
             formatDouble(selected_result.impulse[1]),
             formatDouble(selected_result.impulse[2]),
             formatDouble(selected_result.post_velocity[0]),
             formatDouble(selected_result.post_velocity[1]),
             formatDouble(selected_result.post_velocity[2]),
             std::to_string(selected_result.iterations),
             formatDouble(selected_result.residual)},
        });

    std::ostringstream summary;
    summary << "{\n"
            << "  \"example\": " << quoteJson(example_name) << ",\n"
            << "  \"selected_sdf_backend\": " << quoteJson(sdf_backend.selected_name) << ",\n"
            << "  \"selected_solver_backend\": " << quoteJson(solver_backend.selected_name) << ",\n"
            << "  \"sdf_note\": " << quoteJson(sdf_backend.note) << ",\n"
            << "  \"solver_note\": " << quoteJson(solver_backend.note) << ",\n"
            << "  \"signed_gap\": " << formatDouble(problem.geometry.signed_gap) << ",\n"
            << "  \"normal\": " << apps::vec3Json(problem.geometry.frame.normal) << ",\n"
            << "  \"simple_solver\": {\n"
            << "    \"impulse\": " << apps::vector3Json(simple_result.impulse) << ",\n"
            << "    \"post_velocity\": " << apps::vector3Json(simple_result.post_velocity) << ",\n"
            << "    \"iterations\": " << simple_result.iterations << ",\n"
            << "    \"residual\": " << formatDouble(simple_result.residual) << "\n"
            << "  },\n"
            << "  \"selected_solver\": {\n"
            << "    \"name\": " << quoteJson(selected_solver->name()) << ",\n"
            << "    \"available\": " << boolToString(selected_solver->available()) << ",\n"
            << "    \"impulse\": " << apps::vector3Json(selected_result.impulse) << ",\n"
            << "    \"post_velocity\": " << apps::vector3Json(selected_result.post_velocity) << ",\n"
            << "    \"iterations\": " << selected_result.iterations << ",\n"
            << "    \"residual\": " << formatDouble(selected_result.residual) << ",\n"
            << "    \"note\": " << quoteJson(selected_result.note) << "\n"
            << "  }\n"
            << "}\n";
    writeTextFile(output_dir / "summary.json", summary.str());

    std::cout << example_name << ": sdf_backend=" << sdf_backend.selected_name
              << ", solver_backend=" << solver_backend.selected_name
              << ", normal_impulse=" << selected_result.impulse[0] << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex04_single_step_contact failed: " << error.what() << "\n";
    return 1;
  }
}
