#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "baseline/core/math.h"

namespace baseline {

struct TriangleMesh {
  std::string source_path;
  std::vector<Vec3> vertices;
  std::vector<std::array<int, 3>> triangles;
  Aabb3 local_aabb;
  double bounding_radius{0.0};
};

struct TriangleMeshLoadOptions {
  double scale{1.0};
  bool recenter{false};
  bool normalize{false};
};

TriangleMesh loadTriangleMesh(const std::filesystem::path& path,
                              const TriangleMeshLoadOptions& options = {});
std::shared_ptr<TriangleMesh> loadTriangleMeshShared(const std::filesystem::path& path,
                                                     const TriangleMeshLoadOptions& options = {});

}  // namespace baseline
