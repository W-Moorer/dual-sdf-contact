#include "baseline/benchmark/benchmark_config.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <sstream>

#include "baseline/core/io_utils.h"
#include "baseline/core/simple_json.h"

namespace baseline {

namespace {

using JsonValue = json::Value;

std::string joinWithDelimiter(const std::vector<std::string>& values, std::string_view delimiter) {
  std::ostringstream stream;
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index > 0) {
      stream << delimiter;
    }
    stream << values[index];
  }
  return stream.str();
}

const JsonValue& requireObjectField(const JsonValue& object, std::string_view field) {
  if (!object.isObject() || !object.hasKey(field)) {
    throw BenchmarkConfigError("Missing required config field: " + std::string(field));
  }
  return object.at(field);
}

std::string requireStringField(const JsonValue& object, std::string_view field) {
  const JsonValue& value = requireObjectField(object, field);
  if (!value.isString()) {
    throw BenchmarkConfigError("Config field must be a string: " + std::string(field));
  }
  return value.asString();
}

double requireNumberField(const JsonValue& object, std::string_view field) {
  const JsonValue& value = requireObjectField(object, field);
  if (!value.isNumber()) {
    throw BenchmarkConfigError("Config field must be a number: " + std::string(field));
  }
  return value.asNumber();
}

bool requireBoolField(const JsonValue& object, std::string_view field) {
  const JsonValue& value = requireObjectField(object, field);
  if (!value.isBool()) {
    throw BenchmarkConfigError("Config field must be a boolean: " + std::string(field));
  }
  return value.asBool();
}

std::optional<std::string> optionalStringField(const JsonValue& object, std::string_view field) {
  if (!object.isObject() || !object.hasKey(field)) {
    return std::nullopt;
  }
  const JsonValue& value = object.at(field);
  if (!value.isString()) {
    throw BenchmarkConfigError("Config field must be a string: " + std::string(field));
  }
  return value.asString();
}

std::optional<int> optionalIntField(const JsonValue& object, std::string_view field) {
  if (!object.isObject() || !object.hasKey(field)) {
    return std::nullopt;
  }
  const JsonValue& value = object.at(field);
  if (!value.isNumber()) {
    throw BenchmarkConfigError("Config field must be a number: " + std::string(field));
  }
  return static_cast<int>(std::lround(value.asNumber()));
}

std::optional<double> optionalNumberField(const JsonValue& object, std::string_view field) {
  if (!object.isObject() || !object.hasKey(field)) {
    return std::nullopt;
  }
  const JsonValue& value = object.at(field);
  if (!value.isNumber()) {
    throw BenchmarkConfigError("Config field must be a number: " + std::string(field));
  }
  return value.asNumber();
}

Vec3 requireVec3Field(const JsonValue& object, std::string_view field) {
  const JsonValue& value = requireObjectField(object, field);
  if (!value.isArray() || value.asArray().size() != 3 || !value.asArray()[0].isNumber() ||
      !value.asArray()[1].isNumber() || !value.asArray()[2].isNumber()) {
    throw BenchmarkConfigError("Config field must be a numeric 3-array: " + std::string(field));
  }
  return {value.asArray()[0].asNumber(), value.asArray()[1].asNumber(), value.asArray()[2].asNumber()};
}

std::optional<Vec3> optionalVec3Field(const JsonValue& object, std::string_view field) {
  if (!object.isObject() || !object.hasKey(field)) {
    return std::nullopt;
  }
  return requireVec3Field(object, field);
}

std::vector<std::string> requireStringArrayField(const JsonValue& object, std::string_view field) {
  const JsonValue& value = requireObjectField(object, field);
  if (!value.isArray()) {
    throw BenchmarkConfigError("Config field must be an array: " + std::string(field));
  }
  std::vector<std::string> result;
  for (const JsonValue& entry : value.asArray()) {
    if (!entry.isString()) {
      throw BenchmarkConfigError("Config array must contain strings: " + std::string(field));
    }
    result.push_back(entry.asString());
  }
  if (result.empty()) {
    throw BenchmarkConfigError("Config array must not be empty: " + std::string(field));
  }
  return result;
}

std::vector<double> requireNumberArrayField(const JsonValue& object, std::string_view field) {
  const JsonValue& value = requireObjectField(object, field);
  if (!value.isArray()) {
    throw BenchmarkConfigError("Config field must be an array: " + std::string(field));
  }
  std::vector<double> result;
  for (const JsonValue& entry : value.asArray()) {
    if (!entry.isNumber()) {
      throw BenchmarkConfigError("Config array must contain numbers: " + std::string(field));
    }
    result.push_back(entry.asNumber());
  }
  if (result.empty()) {
    throw BenchmarkConfigError("Config array must not be empty: " + std::string(field));
  }
  return result;
}

std::vector<double> optionalNumberArrayField(const JsonValue& object, std::string_view field) {
  if (!object.isObject() || !object.hasKey(field)) {
    return {};
  }
  return requireNumberArrayField(object, field);
}

BenchmarkShapeSpec parseShapeSpec(const JsonValue& value) {
  if (!value.isObject()) {
    throw BenchmarkConfigError("Shape spec must be an object.");
  }
  BenchmarkShapeSpec spec;
  spec.name = requireStringField(value, "name");
  spec.type = requireStringField(value, "type");
  spec.center = requireVec3Field(value, "center");
  if (const auto rotation = optionalVec3Field(value, "rotation_rpy_deg")) {
    spec.rotation_rpy_deg = *rotation;
  }
  if (spec.type == "sphere") {
    spec.radius = requireNumberField(value, "radius");
  } else if (spec.type == "box") {
    spec.half_extents = requireVec3Field(value, "half_extents");
  } else if (spec.type == "mesh") {
    spec.mesh_path = requireStringField(value, "mesh_path");
  } else {
    throw BenchmarkConfigError("Unsupported shape type in benchmark config: " + spec.type);
  }
  return spec;
}

BenchmarkCaseSpec parseCaseSpec(const JsonValue& value) {
  if (!value.isObject()) {
    throw BenchmarkConfigError("Benchmark case spec must be an object.");
  }
  BenchmarkCaseSpec spec;
  spec.name = requireStringField(value, "name");
  spec.a = parseShapeSpec(requireObjectField(value, "a"));
  spec.b = parseShapeSpec(requireObjectField(value, "b"));
  if (const auto gap_axis = optionalVec3Field(value, "gap_axis")) {
    spec.gap_axis = normalized(*gap_axis, {1.0, 0.0, 0.0});
  }
  return spec;
}

std::string shapeTypeLabel(const ReferenceGeometry& geometry) {
  switch (geometry.type) {
    case ShapeType::Sphere:
      return "sphere";
    case ShapeType::Box:
      return "box";
    case ShapeType::Plane:
      return "plane";
  }
  return "unknown";
}

ReferenceGeometry buildGeometry(const BenchmarkShapeSpec& spec, const Mat3& extra_rotation = identityMat3()) {
  const Mat3 rotation = matMul(extra_rotation, rotationFromRpyDegrees(spec.rotation_rpy_deg));
  if (spec.type == "sphere") {
    return ReferenceGeometry::makeSphere(spec.name, spec.center, spec.radius);
  }
  if (spec.type == "box") {
    return ReferenceGeometry::makeBox(spec.name, spec.center, spec.half_extents, rotation);
  }
  throw BenchmarkConfigError("Mesh-backed cases are not wired into the current benchmark execution path yet.");
}

double caseSignedDistance(const ReferenceGeometry& a, const ReferenceGeometry& b) {
  const AnalyticReferenceBackend analytic_backend;
  return analytic_backend.distance(a, b).signed_distance;
}

BenchmarkCaseSpec withTargetGap(const BenchmarkCaseSpec& base_case, double target_gap) {
  BenchmarkCaseSpec adjusted = base_case;
  ReferenceGeometry geometry_a = buildGeometry(base_case.a);
  ReferenceGeometry geometry_b = buildGeometry(base_case.b);
  const Vec3 axis = normalized(base_case.gap_axis, {1.0, 0.0, 0.0});

  auto evaluate_gap = [&](double translation) {
    geometry_b.center = base_case.b.center + axis * translation;
    return caseSignedDistance(geometry_a, geometry_b);
  };

  double best_translation = 0.0;
  double best_error = std::abs(evaluate_gap(0.0) - target_gap);
  double previous_translation = 0.0;
  double previous_error = evaluate_gap(0.0) - target_gap;
  bool bracketed = false;
  double bracket_low = 0.0;
  double bracket_high = 0.0;

  const double base_step = std::max(0.05, 0.25 * (geometry_a.boundingRadius() + geometry_b.boundingRadius()));
  for (int power = 0; power < 8 && !bracketed; ++power) {
    const double step = std::ldexp(base_step, power);
    for (double candidate : {step, -step}) {
      const double error = evaluate_gap(candidate) - target_gap;
      if (std::abs(error) < best_error) {
        best_error = std::abs(error);
        best_translation = candidate;
      }
      if ((previous_error <= 0.0 && error >= 0.0) || (previous_error >= 0.0 && error <= 0.0)) {
        bracket_low = previous_translation;
        bracket_high = candidate;
        bracketed = true;
        break;
      }
      previous_translation = candidate;
      previous_error = error;
    }
  }

  if (bracketed) {
    double low = bracket_low;
    double high = bracket_high;
    for (int iteration = 0; iteration < 48; ++iteration) {
      const double mid = 0.5 * (low + high);
      const double error = evaluate_gap(mid) - target_gap;
      if (std::abs(error) < best_error) {
        best_error = std::abs(error);
        best_translation = mid;
      }
      const double low_error = evaluate_gap(low) - target_gap;
      if ((low_error <= 0.0 && error >= 0.0) || (low_error >= 0.0 && error <= 0.0)) {
        high = mid;
      } else {
        low = mid;
      }
    }
  }

  adjusted.b.center = base_case.b.center + axis * best_translation;
  return adjusted;
}

JsonValue vec3Value(const Vec3& value) {
  return JsonValue::Array{JsonValue(value.x), JsonValue(value.y), JsonValue(value.z)};
}

JsonValue stringArrayValue(const std::vector<std::string>& values) {
  JsonValue::Array result;
  for (const std::string& value : values) {
    result.emplace_back(value);
  }
  return result;
}

JsonValue numberArrayValue(const std::vector<double>& values) {
  JsonValue::Array result;
  for (double value : values) {
    result.emplace_back(value);
  }
  return result;
}

JsonValue shapeSpecValue(const BenchmarkShapeSpec& spec) {
  JsonValue::Object value = {
      {"center", vec3Value(spec.center)},
      {"name", spec.name},
      {"rotation_rpy_deg", vec3Value(spec.rotation_rpy_deg)},
      {"type", spec.type},
  };
  if (spec.type == "sphere") {
    value["radius"] = JsonValue(spec.radius);
  } else if (spec.type == "box") {
    value["half_extents"] = vec3Value(spec.half_extents);
  } else if (spec.type == "mesh") {
    value["mesh_path"] = JsonValue(spec.mesh_path);
  }
  return value;
}

JsonValue caseSpecValue(const BenchmarkCaseSpec& spec) {
  return JsonValue::Object{
      {"a", shapeSpecValue(spec.a)},
      {"b", shapeSpecValue(spec.b)},
      {"gap_axis", vec3Value(spec.gap_axis)},
      {"name", spec.name},
  };
}

std::vector<double> generateOrientationAngles(const BenchmarkConfig& config) {
  std::vector<double> values = config.orientation_yaw_degrees;
  if (config.random_orientation_count > 0) {
    std::mt19937 generator(static_cast<std::mt19937::result_type>(config.seed));
    std::uniform_real_distribution<double> distribution(
        config.random_orientation_min_deg, config.random_orientation_max_deg);
    for (int index = 0; index < config.random_orientation_count; ++index) {
      values.push_back(distribution(generator));
    }
  }
  return values;
}

ReferenceGeometry rotatedGeometry(const BenchmarkShapeSpec& spec, const Vec3& axis, double degrees) {
  const double radians_per_degree = 3.14159265358979323846 / 180.0;
  return buildGeometry(spec, rotationAroundAxis(axis, degrees * radians_per_degree));
}

BenchmarkSampleSpec makeSample(const BenchmarkConfig& config,
                               int sample_index,
                               const BenchmarkCaseSpec& case_spec,
                               double voxel_size,
                               double narrow_band_half_width,
                               double requested_gap,
                               double orientation_angle_deg,
                               const Vec3& orientation_axis) {
  BenchmarkSampleSpec sample;
  sample.benchmark_name = config.benchmark_name;
  sample.case_family = config.case_family;
  sample.case_name = case_spec.name;
  sample.sample_name = case_spec.name + "_vs" + formatDouble(voxel_size, 3) + "_nb" +
                       formatDouble(narrow_band_half_width, 1) + "_i" + std::to_string(sample_index);
  sample.a = buildGeometry(case_spec.a);
  sample.b = buildGeometry(case_spec.b);
  if (std::abs(orientation_angle_deg) > 1e-12) {
    if (config.orientation_apply_to == "a") {
      sample.a = rotatedGeometry(case_spec.a, orientation_axis, orientation_angle_deg);
    } else {
      sample.b = rotatedGeometry(case_spec.b, orientation_axis, orientation_angle_deg);
    }
  }
  sample.shape_pair = shapeTypeLabel(sample.a) + "-" + shapeTypeLabel(sample.b);
  sample.voxel_size = voxel_size;
  sample.narrow_band_half_width = narrow_band_half_width;
  sample.requested_gap = requested_gap;
  sample.gap_axis = case_spec.gap_axis;
  sample.orientation_angle_deg = orientation_angle_deg;
  sample.orientation_axis = orientation_axis;
  sample.sample_index = sample_index;
  sample.seed = config.seed;
  return sample;
}

}  // namespace

BenchmarkConfig loadBenchmarkConfig(const std::filesystem::path& path) {
  const JsonValue root = json::parse(readTextFile(path));
  if (!root.isObject()) {
    throw BenchmarkConfigError("Benchmark config root must be a JSON object.");
  }

  BenchmarkConfig config;
  config.benchmark_name = requireStringField(root, "benchmark_name");
  config.description = optionalStringField(root, "description").value_or("");
  config.case_family = requireStringField(root, "case_family");
  config.sdf_backends = requireStringArrayField(root, "sdf_backends");
  config.reference_backends = requireStringArrayField(root, "reference_backends");
  config.solver_backends = requireStringArrayField(root, "solver_backends");
  config.voxel_sizes = requireNumberArrayField(root, "voxel_sizes");
  config.narrow_band_half_widths = requireNumberArrayField(root, "narrow_band_half_widths");
  config.runtime_iterations = static_cast<int>(std::lround(requireNumberField(root, "runtime_iterations")));
  config.enable_solver_probe = requireBoolField(root, "enable_solver_probe");
  config.seed = static_cast<int>(std::lround(requireNumberField(root, "seed")));

  if (root.hasKey("base_case")) {
    config.base_case = parseCaseSpec(root.at("base_case"));
    config.has_base_case = true;
  }
  if (root.hasKey("primitive_cases")) {
    const JsonValue& primitive_cases = root.at("primitive_cases");
    if (!primitive_cases.isArray()) {
      throw BenchmarkConfigError("primitive_cases must be an array.");
    }
    for (const JsonValue& value : primitive_cases.asArray()) {
      config.primitive_cases.push_back(parseCaseSpec(value));
    }
  }
  config.gap_values = optionalNumberArrayField(root, "gap_values");
  config.orientation_yaw_degrees = optionalNumberArrayField(root, "orientation_yaw_degrees");
  if (const auto orientation_axis = optionalVec3Field(root, "orientation_axis")) {
    config.orientation_axis = normalized(*orientation_axis, {0.0, 0.0, 1.0});
  }
  if (const auto orientation_apply_to = optionalStringField(root, "orientation_apply_to")) {
    config.orientation_apply_to = *orientation_apply_to;
  }
  config.random_orientation_count = optionalIntField(root, "random_orientation_count").value_or(0);
  config.random_orientation_min_deg = optionalNumberField(root, "random_orientation_min_deg").value_or(-180.0);
  config.random_orientation_max_deg = optionalNumberField(root, "random_orientation_max_deg").value_or(180.0);

  if (config.case_family == "primitive") {
    if (config.primitive_cases.empty()) {
      throw BenchmarkConfigError("primitive case_family requires primitive_cases.");
    }
  } else if (config.case_family == "gap_sweep") {
    if (!config.has_base_case || config.gap_values.empty()) {
      throw BenchmarkConfigError("gap_sweep requires base_case and gap_values.");
    }
  } else if (config.case_family == "orientation_sweep") {
    if (!config.has_base_case) {
      throw BenchmarkConfigError("orientation_sweep requires base_case.");
    }
    if (config.orientation_yaw_degrees.empty() && config.random_orientation_count <= 0) {
      throw BenchmarkConfigError(
          "orientation_sweep requires orientation_yaw_degrees and/or random_orientation_count.");
    }
  } else if (config.case_family == "resolution_sweep") {
    if (!config.has_base_case) {
      throw BenchmarkConfigError("resolution_sweep requires base_case.");
    }
  } else if (config.case_family == "mesh") {
    if (!config.has_base_case) {
      throw BenchmarkConfigError("mesh case_family requires base_case.");
    }
  } else {
    throw BenchmarkConfigError("Unsupported case_family: " + config.case_family);
  }

  return config;
}

ReferenceGeometry makeGeometryFromShapeSpec(const BenchmarkShapeSpec& spec) { return buildGeometry(spec); }

std::vector<BenchmarkSampleSpec> expandBenchmarkSamples(const BenchmarkConfig& config) {
  if (benchmarkConfigUsesMesh(config)) {
    throw BenchmarkConfigError(
        "Mesh-backed benchmark cases are parsed but not executable in the current stage. Keep primitive configs on the main path.");
  }

  std::vector<BenchmarkCaseSpec> structural_cases;
  std::vector<double> orientation_values;
  double requested_gap = 0.0;

  if (config.case_family == "primitive") {
    structural_cases = config.primitive_cases;
    orientation_values = {0.0};
  } else if (config.case_family == "gap_sweep") {
    orientation_values = {0.0};
    for (double target_gap : config.gap_values) {
      BenchmarkCaseSpec adjusted = withTargetGap(config.base_case, target_gap);
      adjusted.name = config.base_case.name + "_gap_" + formatDouble(target_gap, 3);
      structural_cases.push_back(std::move(adjusted));
    }
  } else if (config.case_family == "orientation_sweep") {
    structural_cases = {config.base_case};
    orientation_values = generateOrientationAngles(config);
  } else if (config.case_family == "resolution_sweep") {
    structural_cases = {config.base_case};
    orientation_values = {0.0};
  } else if (config.case_family == "mesh") {
    structural_cases = {config.base_case};
    orientation_values = {0.0};
  }

  if (orientation_values.empty()) {
    orientation_values = {0.0};
  }

  std::vector<BenchmarkSampleSpec> samples;
  int sample_index = 0;
  for (const BenchmarkCaseSpec& case_spec : structural_cases) {
    const double case_gap = (config.case_family == "gap_sweep") ? caseSignedDistance(buildGeometry(case_spec.a), buildGeometry(case_spec.b))
                                                                : requested_gap;
    for (double orientation_angle_deg : orientation_values) {
      for (double voxel_size : config.voxel_sizes) {
        for (double narrow_band_half_width : config.narrow_band_half_widths) {
          samples.push_back(makeSample(config,
                                       sample_index++,
                                       case_spec,
                                       voxel_size,
                                       narrow_band_half_width,
                                       case_gap,
                                       orientation_angle_deg,
                                       config.orientation_axis));
        }
      }
    }
  }
  return samples;
}

std::string benchmarkConfigToJson(const BenchmarkConfig& config) {
  JsonValue::Object root = {
      {"benchmark_name", config.benchmark_name},
      {"case_family", config.case_family},
      {"description", config.description},
      {"enable_solver_probe", config.enable_solver_probe},
      {"narrow_band_half_widths", numberArrayValue(config.narrow_band_half_widths)},
      {"reference_backends", stringArrayValue(config.reference_backends)},
      {"runtime_iterations", config.runtime_iterations},
      {"sdf_backends", stringArrayValue(config.sdf_backends)},
      {"seed", config.seed},
      {"solver_backends", stringArrayValue(config.solver_backends)},
      {"voxel_sizes", numberArrayValue(config.voxel_sizes)},
  };
  if (config.has_base_case) {
    root["base_case"] = caseSpecValue(config.base_case);
  }
  if (!config.primitive_cases.empty()) {
    JsonValue::Array primitive_cases;
    for (const BenchmarkCaseSpec& value : config.primitive_cases) {
      primitive_cases.push_back(caseSpecValue(value));
    }
    root["primitive_cases"] = primitive_cases;
  }
  if (!config.gap_values.empty()) {
    root["gap_values"] = numberArrayValue(config.gap_values);
  }
  if (!config.orientation_yaw_degrees.empty()) {
    root["orientation_yaw_degrees"] = numberArrayValue(config.orientation_yaw_degrees);
  }
  root["orientation_axis"] = vec3Value(config.orientation_axis);
  root["orientation_apply_to"] = config.orientation_apply_to;
  root["random_orientation_count"] = config.random_orientation_count;
  root["random_orientation_min_deg"] = config.random_orientation_min_deg;
  root["random_orientation_max_deg"] = config.random_orientation_max_deg;
  return json::stringify(JsonValue(std::move(root))) + "\n";
}

std::string benchmarkConfigSummary(const BenchmarkConfig& config) {
  std::ostringstream stream;
  stream << "benchmark=" << config.benchmark_name << " case_family=" << config.case_family
         << " sdf=" << joinWithDelimiter(config.sdf_backends, "+")
         << " reference=" << joinWithDelimiter(config.reference_backends, "+")
         << " solver=" << joinWithDelimiter(config.solver_backends, "+")
         << " seed=" << config.seed;
  return stream.str();
}

bool benchmarkConfigUsesMesh(const BenchmarkConfig& config) {
  auto shape_is_mesh = [](const BenchmarkShapeSpec& spec) { return spec.type == "mesh"; };
  if (config.has_base_case &&
      (shape_is_mesh(config.base_case.a) || shape_is_mesh(config.base_case.b))) {
    return true;
  }
  for (const BenchmarkCaseSpec& primitive_case : config.primitive_cases) {
    if (shape_is_mesh(primitive_case.a) || shape_is_mesh(primitive_case.b)) {
      return true;
    }
  }
  return false;
}

}  // namespace baseline
