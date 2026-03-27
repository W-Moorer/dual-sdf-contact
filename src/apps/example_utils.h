#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include "baseline/core/io_utils.h"
#include "baseline/core/math.h"
#include "baseline/runtime/backend_factory.h"
#include "baseline/solver/contact_problem.h"

namespace baseline::apps {

struct ExampleBackendOptions {
  std::string sdf_backend;
  std::string reference_backend;
  std::string solver_backend;
  bool help_requested{false};
};

inline std::filesystem::path outputDir(const std::string& example_name) {
  return ensureExampleOutputDir(example_name);
}

inline void writeCsv(const std::filesystem::path& path,
                     const std::vector<std::string>& header,
                     const std::vector<std::vector<std::string>>& rows) {
  std::ostringstream stream;
  for (std::size_t index = 0; index < header.size(); ++index) {
    if (index > 0) {
      stream << ",";
    }
    stream << header[index];
  }
  stream << "\n";
  for (const auto& row : rows) {
    for (std::size_t index = 0; index < row.size(); ++index) {
      if (index > 0) {
        stream << ",";
      }
      stream << row[index];
    }
    stream << "\n";
  }
  writeTextFile(path, stream.str());
}

inline std::string vec3Json(const Vec3& value) {
  return "[" + formatDouble(value.x) + ", " + formatDouble(value.y) + ", " + formatDouble(value.z) + "]";
}

inline std::string vector3Json(const Vector3& value) {
  return "[" + formatDouble(value[0]) + ", " + formatDouble(value[1]) + ", " + formatDouble(value[2]) + "]";
}

inline std::string envOrEmpty(const char* name) {
#ifdef _WIN32
  char* buffer = nullptr;
  std::size_t size = 0;
  if (_dupenv_s(&buffer, &size, name) != 0 || buffer == nullptr) {
    return {};
  }
  std::string value(buffer);
  std::free(buffer);
  return value;
#else
  const char* value = std::getenv(name);
  return value ? std::string(value) : std::string{};
#endif
}

inline ExampleBackendOptions parseBackendOptions(int argc, char** argv) {
  ExampleBackendOptions options;
  options.sdf_backend = envOrEmpty("BASELINE_SDF_BACKEND");
  options.reference_backend = envOrEmpty("BASELINE_REFERENCE_BACKEND");
  options.solver_backend = envOrEmpty("BASELINE_SOLVER_BACKEND");

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--help" || argument == "-h") {
      options.help_requested = true;
      continue;
    }
    if (argument == "--sdf-backend" || argument == "--reference-backend" || argument == "--solver-backend") {
      if (index + 1 >= argc) {
        throw std::runtime_error("Missing value after " + argument);
      }
      const std::string value = argv[++index];
      if (argument == "--sdf-backend") {
        options.sdf_backend = value;
      } else if (argument == "--reference-backend") {
        options.reference_backend = value;
      } else {
        options.solver_backend = value;
      }
      continue;
    }
    throw std::runtime_error("Unknown argument: " + argument);
  }
  return options;
}

inline void printUsage(const std::string& example_name) {
  std::cout << example_name
            << " [--sdf-backend analytic|openvdb|nanovdb]"
               " [--reference-backend analytic|hppfcl|fcl]"
               " [--solver-backend simple|siconos]\n";
}

inline std::string backendListJson(const BackendAvailabilitySummary& availability) {
  if (availability.real_backends.empty()) {
    return "[]";
  }
  std::ostringstream stream;
  stream << "[";
  for (std::size_t index = 0; index < availability.real_backends.size(); ++index) {
    if (index > 0) {
      stream << ", ";
    }
    stream << quoteJson(availability.real_backends[index]);
  }
  stream << "]";
  return stream.str();
}

}  // namespace baseline::apps
