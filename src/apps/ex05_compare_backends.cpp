#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/runtime/backend_factory.h"
#include "baseline/solver/simple_ccp_solver.h"
#include "example_utils.h"

namespace {

using baseline::ContactKinematicsResult;
using baseline::DistanceQueryResult;
using baseline::ReferenceGeometry;
using baseline::ResolvedReferenceBackend;
using baseline::ResolvedSdfBackend;
using baseline::ResolvedSolverBackend;

struct BenchmarkCase {
  std::string name;
  ReferenceGeometry a;
  ReferenceGeometry b;
};

template <typename Fn>
double averageRuntimeMicros(int iterations, Fn&& fn) {
  volatile double sink = 0.0;
  const auto start = std::chrono::steady_clock::now();
  for (int index = 0; index < iterations; ++index) {
    sink += fn();
  }
  const auto stop = std::chrono::steady_clock::now();
  (void)sink;
  return std::chrono::duration<double, std::micro>(stop - start).count() / static_cast<double>(iterations);
}

std::vector<BenchmarkCase> makeBenchmarkCases() {
  return {
      {"sphere_sphere_sep",
       ReferenceGeometry::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 0.8),
       ReferenceGeometry::makeSphere("sphere_b", {1.95, 0.0, 0.0}, 0.65)},
      {"sphere_box_sep",
       ReferenceGeometry::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 0.8),
       ReferenceGeometry::makeBox("box_b", {1.25, 0.15, 0.0}, {0.35, 0.45, 0.40})},
      {"sphere_box_pen",
       ReferenceGeometry::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 0.8),
       ReferenceGeometry::makeBox("box_b", {0.95, 0.05, 0.0}, {0.45, 0.50, 0.35})},
      {"box_box_sep",
       ReferenceGeometry::makeBox("box_a", {0.0, 0.0, 0.0}, {0.45, 0.30, 0.25}),
       ReferenceGeometry::makeBox("box_b", {1.25, 0.10, 0.0}, {0.40, 0.35, 0.30})},
      {"box_box_pen",
       ReferenceGeometry::makeBox("box_a", {0.0, 0.0, 0.0}, {0.45, 0.30, 0.25}),
       ReferenceGeometry::makeBox("box_b", {0.70, 0.05, 0.0}, {0.40, 0.35, 0.30})},
  };
}

}  // namespace

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
    const ResolvedSdfBackend analytic_sdf_backend = resolveSdfBackend("analytic");
    const auto selected_sdf_backend = resolveSdfBackend(options.sdf_backend);
    const ResolvedReferenceBackend analytic_reference_backend = resolveReferenceBackend("analytic");
    const auto selected_reference_backend = resolveReferenceBackend(options.reference_backend);
    const auto solver_backend = resolveSolverBackend(options.solver_backend);
    const auto output_dir = apps::outputDir(example_name);

    const auto analytic_reference_engine = makeReferenceBackend(analytic_reference_backend);
    const auto selected_reference_engine = makeReferenceBackend(selected_reference_backend);
    auto selected_solver = makeSolverBackend(solver_backend);

    const DualSdfContactCalculator calculator;
    const ContactProblemBuilder builder;
    const SimpleCcpSolver simple_solver;

    std::vector<std::vector<std::string>> csv_rows;
    std::ostringstream json_rows;
    std::ostringstream report;
    report << "# ex05_compare_backends\n\n"
           << "- Platform track: `" << availability.platform_track << "`\n"
           << "- Selected SDF backend: `" << selected_sdf_backend.selected_name << "` (`"
           << selected_sdf_backend.note << "`)\n"
           << "- Selected reference backend: `" << selected_reference_backend.selected_name << "` (`"
           << selected_reference_backend.note << "`)\n"
           << "- Selected solver backend: `" << solver_backend.selected_name << "` (`" << solver_backend.note
           << "`)\n\n"
           << "## Case Summary\n\n";

    json_rows << "  \"cases\": [\n";
    bool first_case = true;

    ContactKinematicsResult last_selected_contact;
    ContactProblem last_problem;
    SolverResult last_simple_result;
    SolverResult last_selected_solver_result;

    for (const auto& benchmark_case : makeBenchmarkCases()) {
      const DistanceQueryResult analytic_reference_result =
          analytic_reference_engine->distance(benchmark_case.a, benchmark_case.b);
      const DistanceQueryResult selected_reference_result =
          selected_reference_engine->distance(benchmark_case.a, benchmark_case.b);

      auto analytic_sdf_a = makeSdfProvider(analytic_sdf_backend, {"analytic_a", benchmark_case.a, 0.05, 3.0});
      auto analytic_sdf_b = makeSdfProvider(analytic_sdf_backend, {"analytic_b", benchmark_case.b, 0.05, 3.0});
      auto selected_sdf_a = makeSdfProvider(selected_sdf_backend, {"selected_a", benchmark_case.a, 0.05, 3.0});
      auto selected_sdf_b = makeSdfProvider(selected_sdf_backend, {"selected_b", benchmark_case.b, 0.05, 3.0});

      const ContactKinematicsResult analytic_sdf_result = calculator.compute(*analytic_sdf_a, *analytic_sdf_b);
      const ContactKinematicsResult selected_sdf_result = calculator.compute(*selected_sdf_a, *selected_sdf_b);

      const double analytic_reference_runtime =
          averageRuntimeMicros(200, [&]() { return analytic_reference_engine->distance(benchmark_case.a, benchmark_case.b).signed_distance; });
      const double selected_reference_runtime =
          averageRuntimeMicros(200, [&]() { return selected_reference_engine->distance(benchmark_case.a, benchmark_case.b).signed_distance; });
      const double analytic_sdf_runtime =
          averageRuntimeMicros(200, [&]() { return calculator.compute(*analytic_sdf_a, *analytic_sdf_b).signed_gap; });
      const double selected_sdf_runtime =
          averageRuntimeMicros(200, [&]() { return calculator.compute(*selected_sdf_a, *selected_sdf_b).signed_gap; });

      const double reference_gap_error =
          std::abs(selected_reference_result.signed_distance - analytic_reference_result.signed_distance);
      const double sdf_gap_error = std::abs(selected_sdf_result.signed_gap - analytic_sdf_result.signed_gap);
      const double reference_normal_angle_error =
          apps::normalAngleDegrees(analytic_reference_result.normal, selected_reference_result.normal);
      const double sdf_normal_angle_error =
          apps::normalAngleDegrees(analytic_sdf_result.normal, selected_sdf_result.normal);

      csv_rows.push_back({
          benchmark_case.name,
          formatDouble(analytic_reference_result.distance),
          formatDouble(selected_reference_result.distance),
          formatDouble(analytic_reference_result.signed_distance),
          formatDouble(selected_reference_result.signed_distance),
          formatDouble(analytic_sdf_result.signed_gap),
          formatDouble(selected_sdf_result.signed_gap),
          formatDouble(reference_gap_error),
          formatDouble(sdf_gap_error),
          formatDouble(reference_normal_angle_error),
          formatDouble(sdf_normal_angle_error),
          formatDouble(analytic_reference_runtime),
          formatDouble(selected_reference_runtime),
          formatDouble(analytic_sdf_runtime),
          formatDouble(selected_sdf_runtime),
      });

      if (!first_case) {
        json_rows << ",\n";
      }
      first_case = false;
      json_rows << "    {\n"
                << "      \"case\": " << quoteJson(benchmark_case.name) << ",\n"
                << "      \"analytic_reference_distance\": " << formatDouble(analytic_reference_result.distance) << ",\n"
                << "      \"selected_reference_distance\": " << formatDouble(selected_reference_result.distance) << ",\n"
                << "      \"analytic_reference_signed_distance\": " << formatDouble(analytic_reference_result.signed_distance) << ",\n"
                << "      \"selected_reference_signed_distance\": " << formatDouble(selected_reference_result.signed_distance) << ",\n"
                << "      \"analytic_sdf_gap\": " << formatDouble(analytic_sdf_result.signed_gap) << ",\n"
                << "      \"selected_sdf_gap\": " << formatDouble(selected_sdf_result.signed_gap) << ",\n"
                << "      \"reference_gap_error\": " << formatDouble(reference_gap_error) << ",\n"
                << "      \"sdf_gap_error\": " << formatDouble(sdf_gap_error) << ",\n"
                << "      \"reference_normal_angle_error_deg\": " << formatDouble(reference_normal_angle_error) << ",\n"
                << "      \"sdf_normal_angle_error_deg\": " << formatDouble(sdf_normal_angle_error) << ",\n"
                << "      \"selected_reference_collision\": " << boolToString(selected_reference_result.collision) << ",\n"
                << "      \"selected_sdf_valid_flags\": " << selected_sdf_result.valid_flags << ",\n"
                << "      \"selected_reference_runtime_us\": " << formatDouble(selected_reference_runtime) << ",\n"
                << "      \"selected_sdf_runtime_us\": " << formatDouble(selected_sdf_runtime) << "\n"
                << "    }";

      report << "- `" << benchmark_case.name << "`: ref signed distance `"
             << formatDouble(analytic_reference_result.signed_distance) << "` -> `"
             << formatDouble(selected_reference_result.signed_distance) << "`, dual-SDF gap `"
             << formatDouble(analytic_sdf_result.signed_gap) << "` -> `"
             << formatDouble(selected_sdf_result.signed_gap) << "`, ref gap error `"
             << formatDouble(reference_gap_error) << "`, sdf gap error `"
             << formatDouble(sdf_gap_error) << "`, sdf normal angle error `"
             << formatDouble(sdf_normal_angle_error) << " deg`\n";

      last_selected_contact = selected_sdf_result;
      const Vec3 relative_velocity_world = fromLocal(selected_sdf_result.frame(), {-0.8, 0.05, 0.0});
      last_problem = builder.build(
          selected_sdf_result,
          {
              1.0,
              0.0,
              relative_velocity_world,
              0.4,
              0.0,
              1e-4,
              "compare-backends-contact",
          });
      last_simple_result = simple_solver.solve(last_problem);
      last_selected_solver_result = selected_solver->solve(last_problem);
    }

    json_rows << "\n  ]";

    apps::writeCsv(
        output_dir / "summary.csv",
        {"case", "analytic_reference_distance", "selected_reference_distance", "analytic_reference_signed_distance",
         "selected_reference_signed_distance", "analytic_sdf_gap", "selected_sdf_gap", "reference_gap_error",
         "sdf_gap_error", "reference_normal_angle_error_deg", "sdf_normal_angle_error_deg",
         "analytic_reference_runtime_us", "selected_reference_runtime_us", "analytic_sdf_runtime_us",
         "selected_sdf_runtime_us"},
        csv_rows);

    std::ostringstream summary;
    summary << "{\n"
            << "  \"example\": " << quoteJson(example_name) << ",\n"
            << "  \"selected_sdf_backend\": " << quoteJson(selected_sdf_backend.selected_name) << ",\n"
            << "  \"selected_reference_backend\": " << quoteJson(selected_reference_backend.selected_name) << ",\n"
            << "  \"selected_solver_backend\": " << quoteJson(solver_backend.selected_name) << ",\n"
            << "  \"real_backends_available\": " << apps::backendListJson(availability) << ",\n"
            << "  \"selected_sdf_note\": " << quoteJson(selected_sdf_backend.note) << ",\n"
            << "  \"selected_reference_note\": " << quoteJson(selected_reference_backend.note) << ",\n"
            << "  \"selected_solver_note\": " << quoteJson(solver_backend.note) << ",\n"
            << json_rows.str() << ",\n"
            << "  \"solver_probe\": {\n"
            << "    \"signed_gap\": " << formatDouble(last_problem.geometry.signed_gap) << ",\n"
            << "    \"normal\": " << apps::vec3Json(last_selected_contact.normal) << ",\n"
            << "    \"simple_solver_normal_impulse\": " << formatDouble(last_simple_result.impulse[0]) << ",\n"
            << "    \"selected_solver_normal_impulse\": " << formatDouble(last_selected_solver_result.impulse[0]) << ",\n"
            << "    \"selected_solver_name\": " << quoteJson(selected_solver->name()) << "\n"
            << "  }\n"
            << "}\n";
    writeTextFile(output_dir / "summary.json", summary.str());
    writeTextFile(output_dir / "report.md", report.str());

    std::cout << example_name << ": reference_backend=" << selected_reference_backend.selected_name
              << ", sdf_backend=" << selected_sdf_backend.selected_name
              << ", solver_backend=" << solver_backend.selected_name << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex05_compare_backends failed: " << error.what() << "\n";
    return 1;
  }
}
