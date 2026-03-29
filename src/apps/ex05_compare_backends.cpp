#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "baseline/benchmark/benchmark_config.h"
#include "baseline/contact/contact_problem_builder.h"
#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/core/build_config.h"
#include "baseline/core/io_utils.h"
#include "baseline/core/simple_json.h"
#include "baseline/runtime/backend_factory.h"
#include "example_utils.h"

namespace {

using baseline::BenchmarkConfig;
using baseline::BenchmarkSampleSpec;
using baseline::ContactKinematicsResult;
using baseline::ContactProblem;
using baseline::ContactProblemBuilder;
using baseline::DistanceQueryResult;
using baseline::DualSdfContactCalculator;
using baseline::ResolvedReferenceBackend;
using baseline::ResolvedSdfBackend;
using baseline::ResolvedSolverBackend;
using baseline::SolverResult;
using JsonValue = baseline::json::Value;

struct BenchmarkOptions {
  std::string config_path;
  std::string case_family_override;
  std::string suite_name;
  std::string run_name;
  std::string sdf_backend;
  std::string reference_backend;
  std::string solver_backend;
  std::filesystem::path output_dir;
  std::optional<int> seed_override;
  bool help_requested{false};
};

struct SampleRecord {
  std::string timestamp_utc;
  std::string benchmark_name;
  std::string suite_name;
  std::string run_name;
  std::string config_summary;
  std::string case_family;
  std::string sweep_family;
  std::string case_name;
  std::string sample_name;
  std::string shape_pair;
  std::string shape_a;
  std::string shape_b;
  std::string mesh_a;
  std::string mesh_b;
  std::string mesh_category;
  std::string case_group;
  std::string sdf_backend;
  std::string sdf_provider_backend;
  std::string reference_backend;
  std::string reference_engine_name;
  std::string reference_source_label;
  std::string reference_mode;
  std::string reference_quality;
  std::string reference_diagnostic_label;
  std::string solver_backend;
  std::string solver_name;
  double voxel_size{0.0};
  double narrow_band_half_width{0.0};
  double mesh_scale_a{1.0};
  double mesh_scale_b{1.0};
  int seed{0};
  int sample_count{1};
  int orientation_index{0};
  int pose_index{0};
  double requested_gap{0.0};
  double orientation_angle_deg{0.0};
  double analytic_reference_distance{0.0};
  double analytic_reference_signed_distance{0.0};
  double reference_distance{0.0};
  double reference_signed_distance{0.0};
  double analytic_signed_gap{0.0};
  double signed_gap{0.0};
  double gap_error{0.0};
  double absolute_gap_error{0.0};
  double relative_gap_error{0.0};
  double sdf_gap_error_vs_analytic{0.0};
  double reference_gap_error_vs_analytic{0.0};
  double normal_angle_error_deg{0.0};
  double sdf_normal_angle_error_vs_analytic_deg{0.0};
  double reference_normal_angle_error_vs_analytic_deg{0.0};
  double symmetry_residual{0.0};
  double tangent_orthogonality_residual{0.0};
  double point_distance_consistency{0.0};
  double reference_point_distance_consistency{0.0};
  double reference_normal_alignment_residual{0.0};
  int invalid_result_count{0};
  int degenerate_normal_count{0};
  int tangent_frame_fallback_count{0};
  int narrow_band_edge_hit_count{0};
  int gradient_consistent_flag{0};
  int reference_warning_flag{0};
  int reference_grazing_flag{0};
  double analytic_reference_runtime_us{0.0};
  double reference_runtime_us{0.0};
  double analytic_sdf_runtime_us{0.0};
  double dual_sdf_runtime_us{0.0};
  double solver_runtime_us{0.0};
  double runtime_total_us{0.0};
  double normal_impulse{0.0};
  double tangential_impulse_magnitude{0.0};
  double solver_residual{0.0};
  double solver_iterations{0.0};
  int solver_success_flag{0};
};

struct NumericSummary {
  double mean{0.0};
  double stddev{0.0};
  double min{0.0};
  double max{0.0};
  double median{0.0};
  double p95{0.0};
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
  return std::chrono::duration<double, std::micro>(stop - start).count() /
         static_cast<double>(std::max(iterations, 1));
}

BenchmarkOptions parseOptions(int argc, char** argv) {
  BenchmarkOptions options;
  options.sdf_backend = baseline::apps::envOrEmpty("BASELINE_SDF_BACKEND");
  options.reference_backend = baseline::apps::envOrEmpty("BASELINE_REFERENCE_BACKEND");
  options.solver_backend = baseline::apps::envOrEmpty("BASELINE_SOLVER_BACKEND");

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--help" || argument == "-h") {
      options.help_requested = true;
      continue;
    }
    auto require_value = [&](const std::string& name) -> std::string {
      if (index + 1 >= argc) {
        throw std::runtime_error("Missing value after " + name);
      }
      return argv[++index];
    };
    if (argument == "--config") {
      options.config_path = require_value(argument);
    } else if (argument == "--case-family") {
      options.case_family_override = require_value(argument);
    } else if (argument == "--suite-name") {
      options.suite_name = require_value(argument);
    } else if (argument == "--run-name") {
      options.run_name = require_value(argument);
    } else if (argument == "--output-dir") {
      options.output_dir = require_value(argument);
    } else if (argument == "--seed") {
      options.seed_override = std::stoi(require_value(argument));
    } else if (argument == "--sdf-backend") {
      options.sdf_backend = require_value(argument);
    } else if (argument == "--reference-backend") {
      options.reference_backend = require_value(argument);
    } else if (argument == "--solver-backend") {
      options.solver_backend = require_value(argument);
    } else {
      throw std::runtime_error("Unknown argument: " + argument);
    }
  }
  return options;
}

void printUsage() {
  std::cout << "ex05_compare_backends"
               " [--config path]"
               " [--case-family primitive|mesh]"
               " [--suite-name name]"
               " [--run-name name]"
               " [--output-dir path]"
               " [--seed N]"
               " [--sdf-backend analytic|openvdb|nanovdb]"
               " [--reference-backend analytic|hppfcl|fcl]"
               " [--solver-backend simple|siconos]\n";
}

std::vector<std::string> singleOrOriginal(const std::vector<std::string>& values, const std::string& override_value) {
  if (!override_value.empty()) {
    return {override_value};
  }
  return values;
}

std::string joinOrNone(const std::vector<std::string>& values) {
  if (values.empty()) {
    return "none";
  }
  std::ostringstream stream;
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index > 0) {
      stream << "+";
    }
    stream << values[index];
  }
  return stream.str();
}

std::vector<ResolvedSdfBackend> resolveSdfBackends(const std::vector<std::string>& requested,
                                                   std::vector<std::string>* skipped_notes) {
  std::vector<ResolvedSdfBackend> resolved;
  for (const std::string& name : requested) {
    try {
      resolved.push_back(baseline::resolveSdfBackend(name));
    } catch (const std::exception& error) {
      skipped_notes->push_back("sdf:" + name + ": " + error.what());
    }
  }
  return resolved;
}

std::vector<ResolvedReferenceBackend> resolveReferenceBackends(const std::vector<std::string>& requested,
                                                               std::vector<std::string>* skipped_notes) {
  std::vector<ResolvedReferenceBackend> resolved;
  for (const std::string& name : requested) {
    try {
      resolved.push_back(baseline::resolveReferenceBackend(name));
    } catch (const std::exception& error) {
      skipped_notes->push_back("reference:" + name + ": " + error.what());
    }
  }
  return resolved;
}

std::vector<ResolvedSolverBackend> resolveSolverBackends(const std::vector<std::string>& requested,
                                                         std::vector<std::string>* skipped_notes) {
  std::vector<ResolvedSolverBackend> resolved;
  for (const std::string& name : requested) {
    try {
      resolved.push_back(baseline::resolveSolverBackend(name));
    } catch (const std::exception& error) {
      skipped_notes->push_back("solver:" + name + ": " + error.what());
    }
  }
  return resolved;
}

NumericSummary summarize(const std::vector<double>& values) {
  NumericSummary summary;
  if (values.empty()) {
    return summary;
  }
  const double count = static_cast<double>(values.size());
  summary.mean = std::accumulate(values.begin(), values.end(), 0.0) / count;
  double variance = 0.0;
  for (double value : values) {
    const double delta = value - summary.mean;
    variance += delta * delta;
  }
  summary.stddev = std::sqrt(variance / count);
  auto sorted = values;
  std::sort(sorted.begin(), sorted.end());
  summary.min = sorted.front();
  summary.max = sorted.back();
  summary.median = sorted[sorted.size() / 2];
  const std::size_t p95_index = static_cast<std::size_t>(0.95 * static_cast<double>(sorted.size() - 1));
  summary.p95 = sorted[p95_index];
  return summary;
}

JsonValue numericSummaryValue(const NumericSummary& summary) {
  return JsonValue::Object{
      {"max", summary.max},
      {"mean", summary.mean},
      {"median", summary.median},
      {"min", summary.min},
      {"p95", summary.p95},
      {"std", summary.stddev},
  };
}

double pointDistanceConsistency(const ContactKinematicsResult& contact) {
  return std::abs(baseline::norm(contact.point_on_b - contact.point_on_a) - std::abs(contact.signed_gap));
}

double referencePointDistanceConsistency(const DistanceQueryResult& reference) {
  return std::abs(
      baseline::norm(reference.closest_point_b - reference.closest_point_a) - std::abs(reference.signed_distance));
}

double referenceNormalAlignmentResidual(const DistanceQueryResult& reference) {
  const baseline::Vec3 separation = reference.closest_point_b - reference.closest_point_a;
  const double separation_norm = baseline::norm(separation);
  if (separation_norm <= 1e-10) {
    return 1.0;
  }
  const baseline::Vec3 separation_normal = separation / separation_norm;
  return 1.0 -
         std::abs(baseline::dot(separation_normal, baseline::normalized(reference.normal, {1.0, 0.0, 0.0})));
}

bool invalidContact(const ContactKinematicsResult& contact) {
  return !std::isfinite(contact.signed_gap) || !contact.hasFlag(baseline::ContactSupportPointsValid);
}

struct ReferenceMetadata {
  std::string source_label;
  std::string mode;
  std::string quality;
  std::string diagnostic_label;
  int warning_flag{0};
  int grazing_flag{0};
};

ReferenceMetadata classifyReferenceBase(const BenchmarkSampleSpec& sample,
                                        const ResolvedReferenceBackend& backend,
                                        const baseline::ReferenceGeometryQueryEngine& engine) {
  const std::string engine_name = engine.name();
  const bool mesh_case = sample.case_family == "mesh";
  if (backend.selected_name == "fcl" && engine_name == "fcl-real") {
    return {engine_name, mesh_case ? "fcl_mesh_distance" : "fcl_primitive_distance", "high", "ok", 0, 0};
  }
  if (!mesh_case && backend.selected_name == "analytic") {
    return {engine_name, "analytic_closed_form", "exact", "ok", 0, 0};
  }
  if (mesh_case && backend.selected_name == "analytic") {
    return {engine_name, "analytic_mesh_proxy", "approximate", "mesh-proxy", 1, 0};
  }
  if (engine_name.find("fallback") != std::string::npos) {
    return {engine_name, "fallback_analytic", "approximate", "fallback", 1, 0};
  }
  return {engine_name,
          mesh_case ? "mesh_generic" : "primitive_generic",
          mesh_case ? "approximate" : "medium",
          mesh_case ? "mesh-generic" : "ok",
          mesh_case ? 1 : 0,
          0};
}

ReferenceMetadata finalizeReferenceMetadata(const BenchmarkSampleSpec& sample,
                                            const DistanceQueryResult& reference,
                                            ReferenceMetadata metadata) {
  const double scale = std::max(sample.a.boundingRadius(), sample.b.boundingRadius());
  const double point_consistency = referencePointDistanceConsistency(reference);
  const double normal_alignment_residual = referenceNormalAlignmentResidual(reference);
  const bool near_contact = std::abs(reference.signed_distance) <= std::max(1e-4, 0.10 * scale);
  const bool grazing = near_contact && normal_alignment_residual > 0.20;
  const bool point_mismatch = point_consistency > std::max(1e-4, 0.025 * scale);

  if (grazing && point_mismatch) {
    metadata.diagnostic_label = "grazing+point-mismatch";
  } else if (grazing) {
    metadata.diagnostic_label = "grazing";
  } else if (point_mismatch) {
    metadata.diagnostic_label = "point-mismatch";
  }

  if (grazing) {
    metadata.grazing_flag = 1;
    metadata.warning_flag = 1;
    if (metadata.quality == "high") {
      metadata.quality = "grazing-caution";
    } else if (metadata.quality == "medium") {
      metadata.quality = "caution";
    } else if (metadata.quality == "approximate") {
      metadata.quality = "low";
    }
  } else if (point_mismatch) {
    metadata.warning_flag = 1;
    if (metadata.quality == "high") {
      metadata.quality = "medium";
    }
  }

  return metadata;
}

JsonValue defaultExperimentFreezeValue() {
  return JsonValue::Object{
      {"default_seed", 17},
      {"freeze_name", "paper_experiment_freeze_v1"},
      {"notes", "Phase-6 paper sprint freeze with OpenVDB+FCL+Simple as the recommended WSL mainline."},
      {"recommended_backends",
       JsonValue::Object{
           {"reference", "fcl"},
           {"sdf", "openvdb"},
           {"solver", "simple"},
       }},
      {"recommended_narrow_band_half_widths",
       JsonValue::Object{
           {"default", 4.0},
           {"tight", 2.0},
           {"wide", 8.0},
       }},
      {"recommended_suites",
       JsonValue::Array{
           JsonValue("paper_minimal"),
           JsonValue("paper_extended"),
       }},
      {"recommended_voxel_sizes",
       JsonValue::Object{
           {"coarse", 0.16},
           {"fine", 0.04},
           {"medium", 0.08},
       }},
  };
}

baseline::SingleStepContactParams makeSolverParams(const ContactKinematicsResult& contact) {
  baseline::SingleStepContactParams params;
  params.inverse_mass_a = 1.0;
  params.inverse_mass_b = 0.0;
  params.relative_velocity_world = baseline::fromLocal(contact.frame(), {-0.8, 0.05, 0.0});
  params.friction_coefficient = 0.4;
  params.restitution = 0.0;
  params.tangential_regularization = 1e-4;
  params.label = "benchmark-contact";
  return params;
}

JsonValue sampleRecordValue(const SampleRecord& record) {
  return JsonValue::Object{
      {"absolute_gap_error", record.absolute_gap_error},
      {"analytic_reference_distance", record.analytic_reference_distance},
      {"analytic_reference_runtime_us", record.analytic_reference_runtime_us},
      {"analytic_reference_signed_distance", record.analytic_reference_signed_distance},
      {"analytic_signed_gap", record.analytic_signed_gap},
      {"analytic_sdf_runtime_us", record.analytic_sdf_runtime_us},
      {"benchmark_name", record.benchmark_name},
      {"case_family", record.case_family},
      {"case_group", record.case_group},
      {"case_name", record.case_name},
      {"config_summary", record.config_summary},
      {"degenerate_normal_count", record.degenerate_normal_count},
      {"dual_sdf_runtime_us", record.dual_sdf_runtime_us},
      {"gap_error", record.gap_error},
      {"gradient_consistent_flag", record.gradient_consistent_flag},
      {"invalid_result_count", record.invalid_result_count},
      {"mesh_a", record.mesh_a},
      {"mesh_b", record.mesh_b},
      {"mesh_category", record.mesh_category},
      {"mesh_scale_a", record.mesh_scale_a},
      {"mesh_scale_b", record.mesh_scale_b},
      {"narrow_band_edge_hit_count", record.narrow_band_edge_hit_count},
      {"narrow_band_half_width", record.narrow_band_half_width},
      {"normal_angle_error_deg", record.normal_angle_error_deg},
      {"normal_impulse", record.normal_impulse},
      {"orientation_angle_deg", record.orientation_angle_deg},
      {"orientation_index", record.orientation_index},
      {"point_distance_consistency", record.point_distance_consistency},
      {"reference_backend", record.reference_backend},
      {"reference_distance", record.reference_distance},
      {"reference_diagnostic_label", record.reference_diagnostic_label},
      {"reference_engine_name", record.reference_engine_name},
      {"reference_gap_error_vs_analytic", record.reference_gap_error_vs_analytic},
      {"reference_grazing_flag", record.reference_grazing_flag},
      {"reference_mode", record.reference_mode},
      {"reference_normal_alignment_residual", record.reference_normal_alignment_residual},
      {"reference_normal_angle_error_vs_analytic_deg", record.reference_normal_angle_error_vs_analytic_deg},
      {"reference_point_distance_consistency", record.reference_point_distance_consistency},
      {"reference_quality", record.reference_quality},
      {"reference_source_label", record.reference_source_label},
      {"reference_warning_flag", record.reference_warning_flag},
      {"reference_runtime_us", record.reference_runtime_us},
      {"reference_signed_distance", record.reference_signed_distance},
      {"relative_gap_error", record.relative_gap_error},
      {"pose_index", record.pose_index},
      {"requested_gap", record.requested_gap},
      {"run_name", record.run_name},
      {"runtime_total_us", record.runtime_total_us},
      {"sample_count", record.sample_count},
      {"sample_name", record.sample_name},
      {"sdf_backend", record.sdf_backend},
      {"sdf_gap_error_vs_analytic", record.sdf_gap_error_vs_analytic},
      {"sdf_normal_angle_error_vs_analytic_deg", record.sdf_normal_angle_error_vs_analytic_deg},
      {"sdf_provider_backend", record.sdf_provider_backend},
      {"seed", record.seed},
      {"shape_a", record.shape_a},
      {"shape_b", record.shape_b},
      {"shape_pair", record.shape_pair},
      {"signed_gap", record.signed_gap},
      {"solver_backend", record.solver_backend},
      {"solver_iterations", record.solver_iterations},
      {"solver_name", record.solver_name},
      {"solver_residual", record.solver_residual},
      {"solver_runtime_us", record.solver_runtime_us},
      {"solver_success_flag", record.solver_success_flag},
      {"suite_name", record.suite_name},
      {"sweep_family", record.sweep_family},
      {"symmetry_residual", record.symmetry_residual},
      {"tangent_frame_fallback_count", record.tangent_frame_fallback_count},
      {"tangent_orthogonality_residual", record.tangent_orthogonality_residual},
      {"tangential_impulse_magnitude", record.tangential_impulse_magnitude},
      {"timestamp_utc", record.timestamp_utc},
      {"voxel_size", record.voxel_size},
  };
}

}  // namespace

int main(int argc, char** argv) {
  using namespace baseline;

  try {
    const std::string example_name = "ex05_compare_backends";
    const BenchmarkOptions options = parseOptions(argc, argv);
    if (options.help_requested) {
      printUsage();
      return 0;
    }

    const std::filesystem::path config_path =
        options.config_path.empty() ? sourceDir() / "configs" / "benchmarks" / "primitive_smoke.json"
                                    : std::filesystem::path(options.config_path);

    BenchmarkConfig config = loadBenchmarkConfig(config_path);
    if (!options.case_family_override.empty()) {
      config.case_family = options.case_family_override;
    }
    if (!options.suite_name.empty()) {
      config.suite_name = options.suite_name;
    }
    if (!options.run_name.empty()) {
      config.run_name = options.run_name;
    }
    if (options.seed_override.has_value()) {
      config.seed = *options.seed_override;
    }
    config.sdf_backends = singleOrOriginal(config.sdf_backends, options.sdf_backend);
    config.reference_backends = singleOrOriginal(config.reference_backends, options.reference_backend);
    config.solver_backends = singleOrOriginal(config.solver_backends, options.solver_backend);

    std::vector<std::string> skipped_backend_notes;
    const std::vector<ResolvedSdfBackend> sdf_backends = resolveSdfBackends(config.sdf_backends, &skipped_backend_notes);
    const std::vector<ResolvedReferenceBackend> reference_backends =
        resolveReferenceBackends(config.reference_backends, &skipped_backend_notes);
    const std::vector<ResolvedSolverBackend> solver_backends =
        resolveSolverBackends(config.solver_backends, &skipped_backend_notes);
    if (sdf_backends.empty() || reference_backends.empty() || solver_backends.empty()) {
      throw std::runtime_error("No executable backend combination remains after availability filtering.");
    }

    const std::vector<BenchmarkSampleSpec> samples = expandBenchmarkSamples(config);
    if (samples.empty()) {
      throw std::runtime_error("Benchmark config produced zero samples.");
    }

    const std::filesystem::path output_root =
        options.output_dir.empty() ? apps::outputDir(example_name) : options.output_dir;
    const std::filesystem::path benchmark_output_dir = output_root / config.benchmark_name;
    std::filesystem::create_directories(benchmark_output_dir);

    const BackendAvailabilitySummary availability = queryBackendAvailability();
    const ResolvedSdfBackend analytic_sdf_backend = resolveSdfBackend("analytic");
    const ResolvedReferenceBackend analytic_reference_backend = resolveReferenceBackend("analytic");
    auto analytic_reference_engine = makeReferenceBackend(analytic_reference_backend);

    const DualSdfContactCalculator calculator;
    const ContactProblemBuilder builder;

    std::vector<SampleRecord> records;
    records.reserve(samples.size() * sdf_backends.size() * reference_backends.size() * solver_backends.size());

    for (const BenchmarkSampleSpec& sample : samples) {
      const DistanceQueryResult analytic_reference_result =
          analytic_reference_engine->distance(sample.a, sample.b);
      auto analytic_sdf_a = makeSdfProvider(
          analytic_sdf_backend, {"analytic_a", sample.a, sample.voxel_size, sample.narrow_band_half_width});
      auto analytic_sdf_b = makeSdfProvider(
          analytic_sdf_backend, {"analytic_b", sample.b, sample.voxel_size, sample.narrow_band_half_width});
      const ContactKinematicsResult analytic_sdf_result = calculator.compute(*analytic_sdf_a, *analytic_sdf_b);

      const double analytic_reference_runtime_us = averageRuntimeMicros(config.runtime_iterations, [&]() {
        return analytic_reference_engine->distance(sample.a, sample.b).signed_distance;
      });
      const double analytic_sdf_runtime_us = averageRuntimeMicros(config.runtime_iterations, [&]() {
        return calculator.compute(*analytic_sdf_a, *analytic_sdf_b).signed_gap;
      });

      for (const ResolvedReferenceBackend& reference_backend : reference_backends) {
        auto reference_engine = makeReferenceBackend(reference_backend);
        const DistanceQueryResult reference_result = reference_engine->distance(sample.a, sample.b);
        const ReferenceMetadata reference_metadata =
            finalizeReferenceMetadata(sample, reference_result, classifyReferenceBase(sample, reference_backend, *reference_engine));
        const double reference_runtime_us = averageRuntimeMicros(config.runtime_iterations, [&]() {
          return reference_engine->distance(sample.a, sample.b).signed_distance;
        });

        for (const ResolvedSdfBackend& sdf_backend : sdf_backends) {
          auto sdf_a =
              makeSdfProvider(sdf_backend, {"selected_a", sample.a, sample.voxel_size, sample.narrow_band_half_width});
          auto sdf_b =
              makeSdfProvider(sdf_backend, {"selected_b", sample.b, sample.voxel_size, sample.narrow_band_half_width});
          const ContactKinematicsResult selected_contact = calculator.compute(*sdf_a, *sdf_b);
          const double dual_sdf_runtime_us = averageRuntimeMicros(config.runtime_iterations, [&]() {
            return calculator.compute(*sdf_a, *sdf_b).signed_gap;
          });

          for (const ResolvedSolverBackend& solver_backend : solver_backends) {
            auto solver = makeSolverBackend(solver_backend);
            SolverResult solver_result;
            double solver_runtime_us = 0.0;

            if (config.enable_solver_probe) {
              const ContactProblem problem = builder.build(selected_contact, makeSolverParams(selected_contact));
              solver_result = solver->solve(problem);
              solver_runtime_us = averageRuntimeMicros(config.runtime_iterations, [&]() {
                return solver->solve(problem).impulse[0];
              });
            } else {
              solver_result.solver_name = solver->name();
              solver_result.note = "Solver probe disabled by benchmark config.";
            }

            SampleRecord record;
            record.timestamp_utc = utcTimestampNow();
            record.benchmark_name = config.benchmark_name;
            record.suite_name = sample.suite_name;
            record.run_name = sample.run_name;
            record.config_summary = benchmarkConfigSummary(config);
            record.case_family = sample.case_family;
            record.sweep_family = sample.sweep_family;
            record.case_name = sample.case_name;
            record.sample_name = sample.sample_name;
            record.shape_pair = sample.shape_pair;
            record.shape_a = sample.shape_a;
            record.shape_b = sample.shape_b;
            record.mesh_a = sample.mesh_a;
            record.mesh_b = sample.mesh_b;
            record.mesh_category = sample.mesh_category;
            record.case_group = sample.case_group;
            record.mesh_scale_a = sample.mesh_scale_a;
            record.mesh_scale_b = sample.mesh_scale_b;
            record.sdf_backend = sdf_backend.selected_name;
            record.sdf_provider_backend = sdf_a->backendName();
            record.reference_backend = reference_backend.selected_name;
            record.reference_engine_name = reference_engine->name();
            record.reference_source_label = reference_metadata.source_label;
            record.reference_mode = reference_metadata.mode;
            record.reference_quality = reference_metadata.quality;
            record.reference_diagnostic_label = reference_metadata.diagnostic_label;
            record.solver_backend = solver_backend.selected_name;
            record.solver_name = solver->name();
            record.voxel_size = sample.voxel_size;
            record.narrow_band_half_width = sample.narrow_band_half_width;
            record.seed = sample.seed;
            record.sample_count = 1;
            record.orientation_index = sample.orientation_index;
            record.pose_index = sample.pose_index;
            record.requested_gap = sample.requested_gap;
            record.orientation_angle_deg = sample.orientation_angle_deg;
            record.analytic_reference_distance = analytic_reference_result.distance;
            record.analytic_reference_signed_distance = analytic_reference_result.signed_distance;
            record.reference_distance = reference_result.distance;
            record.reference_signed_distance = reference_result.signed_distance;
            record.analytic_signed_gap = analytic_sdf_result.signed_gap;
            record.signed_gap = selected_contact.signed_gap;
            record.gap_error = selected_contact.signed_gap - reference_result.signed_distance;
            record.absolute_gap_error = std::abs(record.gap_error);
            record.relative_gap_error = record.absolute_gap_error /
                                        std::max(std::abs(reference_result.signed_distance), sample.voxel_size);
            record.sdf_gap_error_vs_analytic = selected_contact.signed_gap - analytic_sdf_result.signed_gap;
            record.reference_gap_error_vs_analytic =
                reference_result.signed_distance - analytic_reference_result.signed_distance;
            record.normal_angle_error_deg = apps::normalAngleDegrees(selected_contact.normal, reference_result.normal);
            record.sdf_normal_angle_error_vs_analytic_deg =
                apps::normalAngleDegrees(analytic_sdf_result.normal, selected_contact.normal);
            record.reference_normal_angle_error_vs_analytic_deg =
                apps::normalAngleDegrees(analytic_reference_result.normal, reference_result.normal);
            record.symmetry_residual = selected_contact.symmetry_residual;
            record.tangent_orthogonality_residual = selected_contact.tangent_orthogonality;
            record.point_distance_consistency = pointDistanceConsistency(selected_contact);
            record.reference_point_distance_consistency = referencePointDistanceConsistency(reference_result);
            record.reference_normal_alignment_residual = referenceNormalAlignmentResidual(reference_result);
            record.invalid_result_count = invalidContact(selected_contact) ? 1 : 0;
            record.degenerate_normal_count =
                selected_contact.hasFlag(ContactUsedFallbackNormal) ? 1 : 0;
            record.tangent_frame_fallback_count =
                selected_contact.hasFlag(ContactValidTangents) ? 0 : 1;
            record.narrow_band_edge_hit_count =
                selected_contact.hasFlag(ContactNearNarrowBandBoundary) ? 1 : 0;
            record.gradient_consistent_flag =
                selected_contact.hasFlag(ContactGradientsConsistent) ? 1 : 0;
            record.reference_warning_flag = reference_metadata.warning_flag;
            record.reference_grazing_flag = reference_metadata.grazing_flag;
            record.analytic_reference_runtime_us = analytic_reference_runtime_us;
            record.reference_runtime_us = reference_runtime_us;
            record.analytic_sdf_runtime_us = analytic_sdf_runtime_us;
            record.dual_sdf_runtime_us = dual_sdf_runtime_us;
            record.solver_runtime_us = solver_runtime_us;
            record.runtime_total_us = reference_runtime_us + dual_sdf_runtime_us + solver_runtime_us;
            record.normal_impulse = solver_result.impulse[0];
            record.tangential_impulse_magnitude =
                std::sqrt(solver_result.impulse[1] * solver_result.impulse[1] +
                          solver_result.impulse[2] * solver_result.impulse[2]);
            record.solver_residual = solver_result.residual;
            record.solver_iterations = static_cast<double>(solver_result.iterations);
            record.solver_success_flag = solver_result.converged ? 1 : 0;
            records.push_back(std::move(record));
          }
        }
      }
    }

    std::vector<std::vector<std::string>> sample_rows;
    sample_rows.reserve(records.size());
    JsonValue::Array sample_json_rows;
    for (const SampleRecord& record : records) {
      sample_rows.push_back({
          record.timestamp_utc,
          record.suite_name,
          record.run_name,
          record.benchmark_name,
          record.case_family,
          record.case_group,
          record.sweep_family,
          record.case_name,
          record.sample_name,
          record.shape_a,
          record.shape_b,
          record.shape_pair,
          record.mesh_a,
          record.mesh_b,
          record.mesh_category,
          formatDouble(record.mesh_scale_a),
          formatDouble(record.mesh_scale_b),
          record.sdf_backend,
          record.sdf_provider_backend,
          record.reference_backend,
          record.reference_engine_name,
          record.reference_source_label,
          record.reference_mode,
          record.reference_quality,
          record.reference_diagnostic_label,
          record.solver_backend,
          record.solver_name,
          formatDouble(record.voxel_size),
          formatDouble(record.narrow_band_half_width),
          std::to_string(record.seed),
          std::to_string(record.sample_count),
          std::to_string(record.orientation_index),
          std::to_string(record.pose_index),
          formatDouble(record.requested_gap),
          formatDouble(record.orientation_angle_deg),
          formatDouble(record.analytic_reference_signed_distance),
          formatDouble(record.reference_signed_distance),
          formatDouble(record.analytic_signed_gap),
          formatDouble(record.signed_gap),
          formatDouble(record.gap_error),
          formatDouble(record.absolute_gap_error),
          formatDouble(record.relative_gap_error),
          formatDouble(record.sdf_gap_error_vs_analytic),
          formatDouble(record.reference_gap_error_vs_analytic),
          formatDouble(record.normal_angle_error_deg),
          formatDouble(record.sdf_normal_angle_error_vs_analytic_deg),
          formatDouble(record.reference_normal_angle_error_vs_analytic_deg),
          formatDouble(record.symmetry_residual),
          formatDouble(record.tangent_orthogonality_residual),
          formatDouble(record.point_distance_consistency),
          formatDouble(record.reference_point_distance_consistency),
          formatDouble(record.reference_normal_alignment_residual),
          std::to_string(record.invalid_result_count),
          std::to_string(record.degenerate_normal_count),
          std::to_string(record.tangent_frame_fallback_count),
          std::to_string(record.narrow_band_edge_hit_count),
          std::to_string(record.reference_warning_flag),
          std::to_string(record.reference_grazing_flag),
          formatDouble(record.reference_runtime_us),
          formatDouble(record.dual_sdf_runtime_us),
          formatDouble(record.solver_runtime_us),
          formatDouble(record.runtime_total_us),
          formatDouble(record.normal_impulse),
          formatDouble(record.tangential_impulse_magnitude),
          formatDouble(record.solver_residual),
          formatDouble(record.solver_iterations),
          std::to_string(record.solver_success_flag),
      });
      sample_json_rows.push_back(sampleRecordValue(record));
    }

    apps::writeCsv(
        benchmark_output_dir / "samples.csv",
        {"timestamp_utc", "suite_name", "run_name", "benchmark_name", "case_family", "case_group", "sweep_family",
         "case_name", "sample_name", "shape_a", "shape_b", "shape_pair", "mesh_a", "mesh_b", "mesh_category",
         "mesh_scale_a", "mesh_scale_b", "sdf_backend", "sdf_provider_backend", "reference_backend",
         "reference_engine_name", "reference_source_label", "reference_mode", "reference_quality",
         "reference_diagnostic_label", "solver_backend", "solver_name", "voxel_size", "narrow_band_half_width",
         "seed", "sample_count", "orientation_index", "pose_index", "requested_gap", "orientation_angle_deg",
         "analytic_reference_signed_distance",
         "reference_signed_distance", "analytic_signed_gap", "signed_gap", "gap_error", "absolute_gap_error",
         "relative_gap_error", "sdf_gap_error_vs_analytic", "reference_gap_error_vs_analytic",
         "normal_angle_error_deg", "sdf_normal_angle_error_vs_analytic_deg",
         "reference_normal_angle_error_vs_analytic_deg", "symmetry_residual", "tangent_orthogonality_residual",
         "point_distance_consistency", "reference_point_distance_consistency",
         "reference_normal_alignment_residual", "invalid_result_count", "degenerate_normal_count",
         "tangent_frame_fallback_count", "narrow_band_edge_hit_count", "reference_warning_flag",
         "reference_grazing_flag", "reference_runtime_us", "dual_sdf_runtime_us", "solver_runtime_us",
         "runtime_total_us", "normal_impulse", "tangential_impulse_magnitude", "solver_residual",
         "solver_iterations", "solver_success_flag"},
        sample_rows);

    std::map<std::string, std::vector<const SampleRecord*>> grouped_records;
    for (const SampleRecord& record : records) {
      const std::string key = record.suite_name + "|" + record.run_name + "|" + record.benchmark_name + "|" +
                              record.case_family + "|" + record.case_group + "|" + record.sweep_family + "|" +
                              record.case_name + "|" + record.shape_pair + "|" + record.mesh_a + "|" +
                              record.mesh_b + "|" + record.mesh_category + "|" + record.sdf_backend + "|" +
                              record.reference_backend + "|" + record.reference_mode + "|" + record.solver_backend +
                              "|" + formatDouble(record.voxel_size) + "|" +
                              formatDouble(record.narrow_band_half_width) + "|" + std::to_string(record.seed) + "|" +
                              std::to_string(record.orientation_index) + "|" +
                              formatDouble(record.orientation_angle_deg);
      grouped_records[key].push_back(&record);
    }

    std::vector<std::vector<std::string>> summary_rows;
    JsonValue::Array summary_json_rows;
    std::ostringstream report;
    report << "# " << config.benchmark_name << "\n\n"
           << "## Benchmark\n\n"
           << "- Suite: `" << config.suite_name << "`\n"
           << "- Run: `" << config.run_name << "`\n"
           << "- Case family: `" << config.case_family << "`\n"
           << "- Sweep family: `" << config.sweep_family << "`\n"
           << "- Config summary: `" << benchmarkConfigSummary(config) << "`\n"
           << "- Samples: `" << records.size() << "`\n";
    report << "- Active backends:\n"
           << "  - SDF: `" << joinOrNone(config.sdf_backends) << "`\n"
           << "  - Reference: `" << joinOrNone(config.reference_backends) << "`\n"
           << "  - Solver: `" << joinOrNone(config.solver_backends) << "`\n";
    report << "\n## Cases\n\n";
    std::map<std::string, const BenchmarkSampleSpec*> unique_cases;
    for (const BenchmarkSampleSpec& sample : samples) {
      unique_cases.emplace(sample.case_name, &sample);
    }
    for (const auto& [case_name, sample_ptr] : unique_cases) {
      const BenchmarkSampleSpec& sample = *sample_ptr;
      report << "- `" << case_name << "`: `" << sample.shape_a << "-" << sample.shape_b << "`"
             << " (`case_group=" << sample.case_group << "`, `mesh_category=" << sample.mesh_category << "`)";
      if (!sample.mesh_a.empty() || !sample.mesh_b.empty()) {
        report << " (`mesh_a=" << (sample.mesh_a.empty() ? "-" : sample.mesh_a) << "`, `mesh_b="
               << (sample.mesh_b.empty() ? "-" : sample.mesh_b) << "`)";
      }
      report << "\n";
    }
    if (!skipped_backend_notes.empty()) {
      report << "- Skipped backends: ";
      for (std::size_t index = 0; index < skipped_backend_notes.size(); ++index) {
        if (index > 0) {
          report << "; ";
        }
        report << "`" << skipped_backend_notes[index] << "`";
      }
      report << "\n";
    }
    report << "\n## Aggregate Summary\n\n";

    for (const auto& [key, values] : grouped_records) {
      (void)key;
      const SampleRecord& first = *values.front();
      auto collect = [&](const auto& accessor) {
        std::vector<double> series;
        series.reserve(values.size());
        for (const SampleRecord* record : values) {
          series.push_back(accessor(*record));
        }
        return summarize(series);
      };
      const NumericSummary signed_gap = collect([](const SampleRecord& record) { return record.signed_gap; });
      const NumericSummary reference_signed_distance =
          collect([](const SampleRecord& record) { return record.reference_signed_distance; });
      const NumericSummary gap_error = collect([](const SampleRecord& record) { return record.gap_error; });
      const NumericSummary absolute_gap_error =
          collect([](const SampleRecord& record) { return record.absolute_gap_error; });
      const NumericSummary relative_gap_error =
          collect([](const SampleRecord& record) { return record.relative_gap_error; });
      const NumericSummary sdf_gap_error_vs_analytic =
          collect([](const SampleRecord& record) { return record.sdf_gap_error_vs_analytic; });
      const NumericSummary reference_gap_error_vs_analytic =
          collect([](const SampleRecord& record) { return record.reference_gap_error_vs_analytic; });
      const NumericSummary normal_angle_error =
          collect([](const SampleRecord& record) { return record.normal_angle_error_deg; });
      const NumericSummary sdf_normal_angle_error_vs_analytic =
          collect([](const SampleRecord& record) { return record.sdf_normal_angle_error_vs_analytic_deg; });
      const NumericSummary reference_normal_angle_error_vs_analytic =
          collect([](const SampleRecord& record) { return record.reference_normal_angle_error_vs_analytic_deg; });
      const NumericSummary symmetry_residual =
          collect([](const SampleRecord& record) { return record.symmetry_residual; });
      const NumericSummary tangent_orthogonality =
          collect([](const SampleRecord& record) { return record.tangent_orthogonality_residual; });
      const NumericSummary point_distance =
          collect([](const SampleRecord& record) { return record.point_distance_consistency; });
      const NumericSummary reference_point_distance =
          collect([](const SampleRecord& record) { return record.reference_point_distance_consistency; });
      const NumericSummary reference_normal_alignment =
          collect([](const SampleRecord& record) { return record.reference_normal_alignment_residual; });
      const NumericSummary runtime_total =
          collect([](const SampleRecord& record) { return record.runtime_total_us; });
      const NumericSummary runtime_reference =
          collect([](const SampleRecord& record) { return record.reference_runtime_us; });
      const NumericSummary runtime_sdf =
          collect([](const SampleRecord& record) { return record.dual_sdf_runtime_us; });
      const NumericSummary solver_residual =
          collect([](const SampleRecord& record) { return record.solver_residual; });
      const NumericSummary solver_iterations =
          collect([](const SampleRecord& record) { return record.solver_iterations; });
      const NumericSummary normal_impulse =
          collect([](const SampleRecord& record) { return record.normal_impulse; });
      const NumericSummary tangential_impulse =
          collect([](const SampleRecord& record) { return record.tangential_impulse_magnitude; });

      int invalid_result_count = 0;
      int degenerate_normal_count = 0;
      int tangent_frame_fallback_count = 0;
      int narrow_band_edge_hit_count = 0;
      int reference_warning_count = 0;
      int reference_grazing_count = 0;
      int solver_success_count = 0;
      double batch_total_runtime_us = 0.0;
      for (const SampleRecord* record : values) {
        invalid_result_count += record->invalid_result_count;
        degenerate_normal_count += record->degenerate_normal_count;
        tangent_frame_fallback_count += record->tangent_frame_fallback_count;
        narrow_band_edge_hit_count += record->narrow_band_edge_hit_count;
        reference_warning_count += record->reference_warning_flag;
        reference_grazing_count += record->reference_grazing_flag;
        solver_success_count += record->solver_success_flag;
        batch_total_runtime_us += record->runtime_total_us;
      }
      const double solver_success_rate =
          static_cast<double>(solver_success_count) / static_cast<double>(values.size());

      summary_rows.push_back({
          first.suite_name,
          first.run_name,
          first.benchmark_name,
          first.case_family,
          first.case_group,
          first.sweep_family,
          first.case_name,
          first.shape_a,
          first.shape_b,
          first.shape_pair,
          first.mesh_a,
          first.mesh_b,
          first.mesh_category,
          formatDouble(first.mesh_scale_a),
          formatDouble(first.mesh_scale_b),
          first.sdf_backend,
          first.reference_backend,
          first.reference_source_label,
          first.reference_mode,
          first.reference_quality,
          first.reference_diagnostic_label,
          first.solver_backend,
          formatDouble(first.voxel_size),
          formatDouble(first.narrow_band_half_width),
          std::to_string(first.seed),
          std::to_string(first.orientation_index),
          std::to_string(first.pose_index),
          formatDouble(first.orientation_angle_deg),
          std::to_string(values.size()),
          std::to_string(invalid_result_count),
          std::to_string(degenerate_normal_count),
          std::to_string(tangent_frame_fallback_count),
          std::to_string(narrow_band_edge_hit_count),
          std::to_string(reference_warning_count),
          std::to_string(reference_grazing_count),
          formatDouble(signed_gap.mean),
          formatDouble(reference_signed_distance.mean),
          formatDouble(gap_error.mean),
          formatDouble(absolute_gap_error.mean),
          formatDouble(relative_gap_error.mean),
          formatDouble(sdf_gap_error_vs_analytic.mean),
          formatDouble(reference_gap_error_vs_analytic.mean),
          formatDouble(normal_angle_error.mean),
          formatDouble(sdf_normal_angle_error_vs_analytic.mean),
          formatDouble(reference_normal_angle_error_vs_analytic.mean),
          formatDouble(symmetry_residual.mean),
          formatDouble(tangent_orthogonality.mean),
          formatDouble(point_distance.mean),
          formatDouble(reference_point_distance.mean),
          formatDouble(reference_normal_alignment.mean),
          formatDouble(runtime_reference.mean),
          formatDouble(runtime_sdf.mean),
          formatDouble(runtime_total.mean),
          formatDouble(runtime_total.median),
          formatDouble(runtime_total.p95),
          formatDouble(batch_total_runtime_us),
          formatDouble(normal_impulse.mean),
          formatDouble(tangential_impulse.mean),
          formatDouble(solver_residual.mean),
          formatDouble(solver_iterations.mean),
          formatDouble(solver_success_rate),
      });

      summary_json_rows.push_back(JsonValue::Object{
          {"absolute_gap_error", numericSummaryValue(absolute_gap_error)},
          {"batch_total_runtime_us", batch_total_runtime_us},
          {"benchmark_name", first.benchmark_name},
          {"case_family", first.case_family},
          {"case_group", first.case_group},
          {"case_name", first.case_name},
          {"degenerate_normal_count", degenerate_normal_count},
          {"gap_error", numericSummaryValue(gap_error)},
          {"invalid_result_count", invalid_result_count},
          {"mesh_a", first.mesh_a},
          {"mesh_b", first.mesh_b},
          {"mesh_category", first.mesh_category},
          {"mesh_scale_a", first.mesh_scale_a},
          {"mesh_scale_b", first.mesh_scale_b},
          {"narrow_band_edge_hit_count", narrow_band_edge_hit_count},
          {"narrow_band_half_width", first.narrow_band_half_width},
          {"normal_angle_error_deg", numericSummaryValue(normal_angle_error)},
          {"normal_impulse", numericSummaryValue(normal_impulse)},
          {"orientation_angle_deg", first.orientation_angle_deg},
          {"orientation_index", first.orientation_index},
          {"point_distance_consistency", numericSummaryValue(point_distance)},
          {"reference_backend", first.reference_backend},
          {"reference_diagnostic_label", first.reference_diagnostic_label},
          {"reference_gap_error_vs_analytic", numericSummaryValue(reference_gap_error_vs_analytic)},
          {"reference_grazing_count", reference_grazing_count},
          {"reference_mode", first.reference_mode},
          {"reference_normal_alignment_residual", numericSummaryValue(reference_normal_alignment)},
          {"reference_normal_angle_error_vs_analytic_deg",
           numericSummaryValue(reference_normal_angle_error_vs_analytic)},
          {"reference_point_distance_consistency", numericSummaryValue(reference_point_distance)},
          {"reference_quality", first.reference_quality},
          {"reference_source_label", first.reference_source_label},
          {"reference_warning_count", reference_warning_count},
          {"reference_runtime_us", numericSummaryValue(runtime_reference)},
          {"reference_signed_distance", numericSummaryValue(reference_signed_distance)},
          {"relative_gap_error", numericSummaryValue(relative_gap_error)},
          {"pose_index", first.pose_index},
          {"run_name", first.run_name},
          {"sample_count", static_cast<int>(values.size())},
          {"sdf_backend", first.sdf_backend},
          {"sdf_gap_error_vs_analytic", numericSummaryValue(sdf_gap_error_vs_analytic)},
          {"sdf_normal_angle_error_vs_analytic_deg", numericSummaryValue(sdf_normal_angle_error_vs_analytic)},
          {"seed", first.seed},
          {"shape_a", first.shape_a},
          {"shape_b", first.shape_b},
          {"shape_pair", first.shape_pair},
          {"signed_gap", numericSummaryValue(signed_gap)},
          {"solver_backend", first.solver_backend},
          {"solver_iterations", numericSummaryValue(solver_iterations)},
          {"solver_residual", numericSummaryValue(solver_residual)},
          {"solver_success_rate", solver_success_rate},
          {"suite_name", first.suite_name},
          {"sweep_family", first.sweep_family},
          {"symmetry_residual", numericSummaryValue(symmetry_residual)},
          {"tangent_frame_fallback_count", tangent_frame_fallback_count},
          {"tangent_orthogonality_residual", numericSummaryValue(tangent_orthogonality)},
          {"tangential_impulse_magnitude", numericSummaryValue(tangential_impulse)},
          {"total_runtime_us", numericSummaryValue(runtime_total)},
          {"voxel_size", first.voxel_size},
      });

      report << "- `" << first.case_name << "` / `sdf=" << first.sdf_backend << "` / `reference="
             << first.reference_backend << "` / `solver=" << first.solver_backend << "` / `voxel="
             << formatDouble(first.voxel_size, 3) << "` / `nb=" << formatDouble(first.narrow_band_half_width, 1)
             << "` / `ref_quality=" << first.reference_quality << "` / `ref_diag="
             << first.reference_diagnostic_label << "`";
      if (std::abs(first.orientation_angle_deg) > 1e-12 || first.sweep_family == "orientation_sweep") {
        report << " / `rot=" << formatDouble(first.orientation_angle_deg, 1) << "deg`";
      }
      report << ": mean abs gap error `" << formatDouble(absolute_gap_error.mean)
             << "`, mean normal angle error `" << formatDouble(normal_angle_error.mean)
             << " deg`, mean runtime `" << formatDouble(runtime_total.mean) << " us`"
             << ", ref point residual `" << formatDouble(reference_point_distance.mean)
             << "`, ref grazing count `" << reference_grazing_count << "`\n";
    }

    apps::writeCsv(
        benchmark_output_dir / "summary.csv",
        {"suite_name", "run_name", "benchmark_name", "case_family", "case_group", "sweep_family", "case_name",
         "shape_a", "shape_b", "shape_pair", "mesh_a", "mesh_b", "mesh_category", "mesh_scale_a",
         "mesh_scale_b", "sdf_backend", "reference_backend", "reference_source_label", "reference_mode",
         "reference_quality", "reference_diagnostic_label", "solver_backend", "voxel_size",
         "narrow_band_half_width", "seed",
         "orientation_index", "pose_index", "orientation_angle_deg", "sample_count", "invalid_result_count",
         "degenerate_normal_count", "tangent_frame_fallback_count", "narrow_band_edge_hit_count",
         "reference_warning_count", "reference_grazing_count", "signed_gap_mean", "reference_signed_distance_mean",
         "gap_error_mean", "absolute_gap_error_mean", "relative_gap_error_mean",
         "sdf_gap_error_vs_analytic_mean", "reference_gap_error_vs_analytic_mean", "normal_angle_error_deg_mean",
         "sdf_normal_angle_error_vs_analytic_deg_mean",
         "reference_normal_angle_error_vs_analytic_deg_mean", "symmetry_residual_mean",
         "tangent_orthogonality_residual_mean", "point_distance_consistency_mean",
         "reference_point_distance_consistency_mean", "reference_normal_alignment_residual_mean",
         "reference_runtime_us_mean", "dual_sdf_runtime_us_mean", "runtime_total_us_mean",
         "runtime_total_us_median", "runtime_total_us_p95", "batch_total_runtime_us", "normal_impulse_mean",
         "tangential_impulse_magnitude_mean", "solver_residual_mean", "solver_iterations_mean", "solver_success_rate"},
        summary_rows);

    const JsonValue resolved_config_json = baseline::json::parse(benchmarkConfigToJson(config));
    JsonValue::Array resolved_sdf_backends;
    for (const auto& backend : sdf_backends) {
      resolved_sdf_backends.emplace_back(backend.selected_name);
    }
    JsonValue::Array resolved_reference_backends;
    for (const auto& backend : reference_backends) {
      resolved_reference_backends.emplace_back(backend.selected_name);
    }
    JsonValue::Array resolved_solver_backends;
    for (const auto& backend : solver_backends) {
      resolved_solver_backends.emplace_back(backend.selected_name);
    }
    JsonValue::Array skipped_json;
    for (const std::string& note : skipped_backend_notes) {
      skipped_json.emplace_back(note);
    }

    writeTextFile(
        benchmark_output_dir / "config_resolved.json",
        json::stringify(JsonValue::Object{
            {"config", resolved_config_json},
            {"config_path", config_path.string()},
            {"config_summary", benchmarkConfigSummary(config)},
            {"experiment_freeze", defaultExperimentFreezeValue()},
            {"run_name", config.run_name},
            {"resolved_reference_backends", resolved_reference_backends},
            {"resolved_sdf_backends", resolved_sdf_backends},
            {"resolved_solver_backends", resolved_solver_backends},
            {"skipped_backends", skipped_json},
            {"suite_name", config.suite_name},
            {"timestamp_utc", utcTimestampNow()},
        }) +
            "\n");

    writeTextFile(
        benchmark_output_dir / "environment.json",
        json::stringify(JsonValue::Object{
            {"active_backends",
             JsonValue::Object{
                 {"reference", resolved_reference_backends},
                 {"sdf", resolved_sdf_backends},
                 {"solver", resolved_solver_backends},
             }},
            {"cmake_generator", BASELINE_CMAKE_GENERATOR},
            {"compiler",
             JsonValue::Object{
                 {"id", BASELINE_CXX_COMPILER_ID},
                 {"version", BASELINE_CXX_COMPILER_VERSION},
             }},
            {"dependency_availability",
             JsonValue::Object{
                 {"fcl", availability.fcl_available},
                 {"hppfcl", availability.hppfcl_available},
                 {"nanovdb", availability.nanovdb_available},
                 {"openvdb", availability.openvdb_available},
                 {"siconos", availability.siconos_available},
             }},
            {"experiment_freeze", defaultExperimentFreezeValue()},
            {"fallback_enabled",
             JsonValue::Object{
                 {"reference", availability.force_fallback_reference},
                 {"sdf", availability.force_fallback_sdf},
                 {"solver", availability.force_simple_solver},
             }},
            {"run_name", config.run_name},
            {"platform_track", availability.platform_track},
            {"suite_name", config.suite_name},
            {"timestamp_utc", utcTimestampNow()},
        }) +
            "\n");

    report << "\n## Output Files\n\n"
           << "- `config_resolved.json`\n"
           << "- `environment.json`\n"
           << "- `samples.csv`\n"
           << "- `summary.csv`\n"
           << "- `summary.json`\n"
           << "- `report.md`\n";
    writeTextFile(benchmark_output_dir / "report.md", report.str());
    writeTextFile(
        benchmark_output_dir / "summary.json",
        json::stringify(JsonValue::Object{
            {"benchmark_name", config.benchmark_name},
            {"case_family", config.case_family},
            {"config_summary", benchmarkConfigSummary(config)},
            {"run_name", config.run_name},
            {"sample_count", static_cast<int>(records.size())},
            {"samples", sample_json_rows},
            {"skipped_backends", skipped_json},
            {"suite_name", config.suite_name},
            {"sweep_family", config.sweep_family},
            {"summary_rows", summary_json_rows},
            {"timestamp_utc", utcTimestampNow()},
        }) +
            "\n");

    std::cout << example_name << ": benchmark=" << config.benchmark_name << ", samples=" << records.size()
              << ", output_dir=" << benchmark_output_dir.string() << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex05_compare_backends failed: " << error.what() << "\n";
    return 1;
  }
}
