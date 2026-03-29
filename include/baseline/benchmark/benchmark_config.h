#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "baseline/contact/reference_geometry.h"

namespace baseline {

struct BenchmarkShapeSpec {
  std::string name;
  std::string type;
  Vec3 center{0.0, 0.0, 0.0};
  Vec3 half_extents{0.5, 0.5, 0.5};
  double radius{0.5};
  Vec3 rotation_rpy_deg{0.0, 0.0, 0.0};
  std::string mesh_path;
  double mesh_scale{1.0};
  bool mesh_recenter{false};
  bool mesh_normalize{false};
};

struct BenchmarkCaseSpec {
  std::string name;
  BenchmarkShapeSpec a;
  BenchmarkShapeSpec b;
  Vec3 gap_axis{1.0, 0.0, 0.0};
};

struct BenchmarkConfig {
  std::string benchmark_name;
  std::string suite_name;
  std::string run_name;
  std::string description;
  std::string case_family;
  std::string sweep_family;
  std::vector<std::string> sdf_backends;
  std::vector<std::string> reference_backends;
  std::vector<std::string> solver_backends;
  std::vector<double> voxel_sizes;
  std::vector<double> narrow_band_half_widths;
  int runtime_iterations{64};
  bool enable_solver_probe{true};
  int seed{1};
  bool has_base_case{false};
  BenchmarkCaseSpec base_case;
  std::vector<BenchmarkCaseSpec> primitive_cases;
  std::vector<double> gap_values;
  std::vector<double> orientation_yaw_degrees;
  Vec3 orientation_axis{0.0, 0.0, 1.0};
  std::string orientation_apply_to{"b"};
  int random_orientation_count{0};
  double random_orientation_min_deg{-180.0};
  double random_orientation_max_deg{180.0};
};

struct BenchmarkSampleSpec {
  std::string benchmark_name;
  std::string suite_name;
  std::string run_name;
  std::string case_family;
  std::string sweep_family;
  std::string case_name;
  std::string sample_name;
  std::string shape_pair;
  std::string shape_a;
  std::string shape_b;
  std::string mesh_a;
  std::string mesh_b;
  double mesh_scale_a{1.0};
  double mesh_scale_b{1.0};
  ReferenceGeometry a;
  ReferenceGeometry b;
  double voxel_size{0.1};
  double narrow_band_half_width{3.0};
  double requested_gap{0.0};
  Vec3 gap_axis{1.0, 0.0, 0.0};
  double orientation_angle_deg{0.0};
  Vec3 orientation_axis{0.0, 0.0, 1.0};
  int sample_index{0};
  int seed{1};
};

class BenchmarkConfigError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

BenchmarkConfig loadBenchmarkConfig(const std::filesystem::path& path);
std::vector<BenchmarkSampleSpec> expandBenchmarkSamples(const BenchmarkConfig& config);
ReferenceGeometry makeGeometryFromShapeSpec(const BenchmarkShapeSpec& spec);
std::string benchmarkConfigToJson(const BenchmarkConfig& config);
std::string benchmarkConfigSummary(const BenchmarkConfig& config);
bool benchmarkConfigUsesMesh(const BenchmarkConfig& config);

}  // namespace baseline
