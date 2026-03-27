#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "baseline/core/io_utils.h"
#include "baseline/core/math.h"
#include "baseline/solver/contact_problem.h"

namespace baseline::apps {

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

}  // namespace baseline::apps
