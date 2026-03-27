#include <iostream>
#include <memory>
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

  const std::string example_name = "ex05_compare_backends";
  const auto output_dir = apps::outputDir(example_name);

  const auto reference_engine = makeDefaultReferenceGeometryQueryEngine();
  const ReferenceGeometry sphere_geometry = ReferenceGeometry::makeSphere("sphere_ref", {0.0, 0.0, 0.0}, 0.8);
  const ReferenceGeometry box_geometry = ReferenceGeometry::makeBox("box_ref", {1.15, 0.1, 0.0}, {0.4, 0.5, 0.4});
  const NanoVdbSdfProvider sphere_sdf =
      NanoVdbSdfProvider::makeSphere("sphere_sdf", sphere_geometry.center, sphere_geometry.radius, 0.05, 2.0);
  const NanoVdbSdfProvider box_sdf =
      NanoVdbSdfProvider::makeBox("box_sdf", box_geometry.center, box_geometry.half_extents, 0.05, 2.0);

  const DistanceQueryResult reference_result = reference_engine->distance(sphere_geometry, box_geometry);
  const DualSdfContactCalculator calculator;
  const ContactGeometry dual_sdf_result = calculator.compute(sphere_sdf, box_sdf);
  const Vec3 relative_velocity_world =
      fromLocal(dual_sdf_result.frame, {-0.8, 0.05, 0.0});

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
  OptionalSiconosSolver siconos_solver;
  const SolverResult optional_result = siconos_solver.solve(problem);

  apps::writeCsv(
      output_dir / "summary.csv",
      {"metric", "reference", "dual_sdf", "simple_solver", "optional_siconos"},
      {
          {"gap_or_distance",
           formatDouble(reference_result.distance),
           formatDouble(dual_sdf_result.signed_gap),
           "",
           ""},
          {"normal_x",
           formatDouble(reference_result.normal.x),
           formatDouble(dual_sdf_result.frame.normal.x),
           "",
           ""},
          {"normal_impulse", "", "", formatDouble(simple_result.impulse[0]), formatDouble(optional_result.impulse[0])},
          {"residual", "", "", formatDouble(simple_result.residual), formatDouble(optional_result.residual)},
      });

  std::ostringstream report;
  report << "# ex05_compare_backends\n\n"
         << "- Reference engine: `" << reference_engine->name() << "`\n"
         << "- Dual-SDF backend A/B: `" << sphere_sdf.backendName() << "` / `" << box_sdf.backendName() << "`\n"
         << "- Distance vs gap: `" << formatDouble(reference_result.distance) << "` vs `"
         << formatDouble(dual_sdf_result.signed_gap) << "`\n"
         << "- Simple solver normal impulse: `" << formatDouble(simple_result.impulse[0]) << "`\n"
         << "- Optional Siconos note: `" << optional_result.note << "`\n";
  writeTextFile(output_dir / "report.md", report.str());

  std::cout << example_name << ": reference_distance=" << reference_result.distance
            << ", dual_sdf_gap=" << dual_sdf_result.signed_gap << "\n";
  return 0;
}
