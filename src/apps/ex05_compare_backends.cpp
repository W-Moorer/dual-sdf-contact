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
    const std::string example_name = "ex05_compare_backends";
    const auto options = apps::parseBackendOptions(argc, argv);
    if (options.help_requested) {
      apps::printUsage(example_name);
      return 0;
    }

    const auto availability = queryBackendAvailability();
    const auto sdf_backend = resolveSdfBackend(options.sdf_backend);
    const auto reference_backend = resolveReferenceBackend(options.reference_backend);
    const auto solver_backend = resolveSolverBackend(options.solver_backend);
    const auto output_dir = apps::outputDir(example_name);

    const ReferenceGeometry sphere_geometry = ReferenceGeometry::makeSphere("sphere_ref", {0.0, 0.0, 0.0}, 0.8);
    const ReferenceGeometry box_geometry = ReferenceGeometry::makeBox("box_ref", {1.15, 0.1, 0.0}, {0.4, 0.5, 0.4});

    const AnalyticReferenceBackend analytic_reference_backend;
    const DistanceQueryResult analytic_reference_result = analytic_reference_backend.distance(sphere_geometry, box_geometry);
    auto selected_reference_backend = makeReferenceBackend(reference_backend);
    const DistanceQueryResult selected_reference_result =
        selected_reference_backend->distance(sphere_geometry, box_geometry);

    auto sphere_sdf =
        makeSdfProvider(sdf_backend, {"sphere_sdf", sphere_geometry, 0.05, 2.0});
    auto box_sdf =
        makeSdfProvider(sdf_backend, {"box_sdf", box_geometry, 0.05, 2.0});

    const DualSdfContactCalculator calculator;
    const ContactGeometry dual_sdf_result = calculator.compute(*sphere_sdf, *box_sdf);
    const Vec3 relative_velocity_world = fromLocal(dual_sdf_result.frame, {-0.8, 0.05, 0.0});

    const ContactProblemBuilder builder;
    const ContactProblem problem = builder.build(
        dual_sdf_result,
        {
            1.0,
            0.0,
            relative_velocity_world,
            0.4,
            0.0,
            1e-4,
            "compare-backends-contact",
        });

    const SimpleCcpSolver simple_solver;
    const SolverResult simple_result = simple_solver.solve(problem);
    auto selected_solver = makeSolverBackend(solver_backend);
    const SolverResult selected_result = selected_solver->solve(problem);

    apps::writeCsv(
        output_dir / "summary.csv",
        {"metric", "analytic_reference", "selected_reference", "dual_sdf", "simple_solver", "selected_solver"},
        {
            {"gap_or_distance",
             formatDouble(analytic_reference_result.distance),
             formatDouble(selected_reference_result.distance),
             formatDouble(dual_sdf_result.signed_gap),
             "",
             ""},
            {"normal_x",
             formatDouble(analytic_reference_result.normal.x),
             formatDouble(selected_reference_result.normal.x),
             formatDouble(dual_sdf_result.frame.normal.x),
             "",
             ""},
            {"normal_impulse", "", "", "", formatDouble(simple_result.impulse[0]), formatDouble(selected_result.impulse[0])},
            {"residual", "", "", "", formatDouble(simple_result.residual), formatDouble(selected_result.residual)},
        });

    std::ostringstream report;
    report << "# ex05_compare_backends\n\n"
           << "- Platform track: `" << availability.platform_track << "`\n"
           << "- Analytic reference backend: `" << analytic_reference_backend.name() << "`\n"
           << "- Selected reference backend: `" << reference_backend.selected_name << "` (`"
           << selected_reference_backend->name() << "`)\n"
           << "- Selected SDF backend: `" << sdf_backend.selected_name << "` (`"
           << sphere_sdf->backendName() << "`)\n"
           << "- Selected solver backend: `" << solver_backend.selected_name << "` (`"
           << selected_solver->name() << "`)\n"
           << "- Reference note: `" << reference_backend.note << "`\n"
           << "- SDF note: `" << sdf_backend.note << "`\n"
           << "- Solver note: `" << solver_backend.note << "`\n"
           << "- Analytic reference distance vs dual-SDF gap: `" << formatDouble(analytic_reference_result.distance)
           << "` vs `" << formatDouble(dual_sdf_result.signed_gap) << "`\n"
           << "- Selected reference distance: `" << formatDouble(selected_reference_result.distance) << "`\n"
           << "- Simple solver normal impulse: `" << formatDouble(simple_result.impulse[0]) << "`\n"
           << "- Selected solver normal impulse: `" << formatDouble(selected_result.impulse[0]) << "`\n";
    writeTextFile(output_dir / "report.md", report.str());

    std::cout << example_name << ": reference_backend=" << reference_backend.selected_name
              << ", sdf_backend=" << sdf_backend.selected_name
              << ", solver_backend=" << solver_backend.selected_name << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex05_compare_backends failed: " << error.what() << "\n";
    return 1;
  }
}
