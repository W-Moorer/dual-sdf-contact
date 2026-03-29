#include "baseline/mesh/triangle_mesh.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace baseline {

namespace {

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string trim(const std::string& value) {
  std::size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  std::size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(start, end - start);
}

void appendFace(std::vector<std::array<int, 3>>* triangles, const std::vector<int>& indices) {
  if (indices.size() < 3) {
    return;
  }
  for (std::size_t index = 1; index + 1 < indices.size(); ++index) {
    triangles->push_back({indices[0], indices[index], indices[index + 1]});
  }
}

int resolveObjIndex(int raw_index, std::size_t vertex_count) {
  if (raw_index > 0) {
    return raw_index - 1;
  }
  if (raw_index < 0) {
    return static_cast<int>(vertex_count) + raw_index;
  }
  throw std::runtime_error("OBJ face index 0 is invalid.");
}

TriangleMesh loadObjMesh(const std::filesystem::path& path) {
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("Failed to open OBJ mesh: " + path.string());
  }

  TriangleMesh mesh;
  mesh.source_path = path.generic_string();

  std::string line;
  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream parser(line);
    std::string token;
    parser >> token;
    if (token == "v") {
      Vec3 vertex;
      parser >> vertex.x >> vertex.y >> vertex.z;
      mesh.vertices.push_back(vertex);
    } else if (token == "f") {
      std::vector<int> indices;
      std::string face_token;
      while (parser >> face_token) {
        const std::size_t slash = face_token.find('/');
        const std::string index_token = slash == std::string::npos ? face_token : face_token.substr(0, slash);
        indices.push_back(resolveObjIndex(std::stoi(index_token), mesh.vertices.size()));
      }
      appendFace(&mesh.triangles, indices);
    }
  }
  return mesh;
}

TriangleMesh loadAsciiStlMesh(const std::filesystem::path& path) {
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("Failed to open STL mesh: " + path.string());
  }

  TriangleMesh mesh;
  mesh.source_path = path.generic_string();

  std::string line;
  std::vector<int> current_face;
  while (std::getline(stream, line)) {
    line = trim(line);
    const std::string lower = toLower(line);
    if (lower.rfind("vertex ", 0) == 0) {
      std::istringstream parser(line.substr(7));
      Vec3 vertex;
      parser >> vertex.x >> vertex.y >> vertex.z;
      mesh.vertices.push_back(vertex);
      current_face.push_back(static_cast<int>(mesh.vertices.size() - 1));
      if (current_face.size() == 3) {
        mesh.triangles.push_back({current_face[0], current_face[1], current_face[2]});
        current_face.clear();
      }
    }
  }
  return mesh;
}

void finalizeMesh(TriangleMesh* mesh, const TriangleMeshLoadOptions& options) {
  if (mesh->vertices.empty() || mesh->triangles.empty()) {
    throw std::runtime_error("Triangle mesh is empty after loading.");
  }

  Aabb3 bounds;
  bounds.valid = true;
  bounds.lower = mesh->vertices.front();
  bounds.upper = mesh->vertices.front();
  for (const Vec3& vertex : mesh->vertices) {
    bounds.lower.x = std::min(bounds.lower.x, vertex.x);
    bounds.lower.y = std::min(bounds.lower.y, vertex.y);
    bounds.lower.z = std::min(bounds.lower.z, vertex.z);
    bounds.upper.x = std::max(bounds.upper.x, vertex.x);
    bounds.upper.y = std::max(bounds.upper.y, vertex.y);
    bounds.upper.z = std::max(bounds.upper.z, vertex.z);
  }

  Vec3 center = bounds.center();
  if (options.recenter) {
    for (Vec3& vertex : mesh->vertices) {
      vertex = vertex - center;
    }
    bounds.lower = bounds.lower - center;
    bounds.upper = bounds.upper - center;
    center = {0.0, 0.0, 0.0};
  }

  double normalization_scale = 1.0;
  if (options.normalize) {
    double max_radius = 0.0;
    for (const Vec3& vertex : mesh->vertices) {
      max_radius = std::max(max_radius, norm(vertex));
    }
    if (max_radius > 1e-12) {
      normalization_scale = 1.0 / max_radius;
    }
  }

  const double final_scale = options.scale * normalization_scale;
  if (std::abs(final_scale - 1.0) > 1e-12) {
    for (Vec3& vertex : mesh->vertices) {
      vertex = vertex * final_scale;
    }
  }

  mesh->local_aabb = {};
  mesh->local_aabb.valid = true;
  mesh->local_aabb.lower = mesh->vertices.front();
  mesh->local_aabb.upper = mesh->vertices.front();
  mesh->bounding_radius = 0.0;
  for (const Vec3& vertex : mesh->vertices) {
    mesh->local_aabb.lower.x = std::min(mesh->local_aabb.lower.x, vertex.x);
    mesh->local_aabb.lower.y = std::min(mesh->local_aabb.lower.y, vertex.y);
    mesh->local_aabb.lower.z = std::min(mesh->local_aabb.lower.z, vertex.z);
    mesh->local_aabb.upper.x = std::max(mesh->local_aabb.upper.x, vertex.x);
    mesh->local_aabb.upper.y = std::max(mesh->local_aabb.upper.y, vertex.y);
    mesh->local_aabb.upper.z = std::max(mesh->local_aabb.upper.z, vertex.z);
    mesh->bounding_radius = std::max(mesh->bounding_radius, norm(vertex));
  }
}

std::string cacheKey(const std::filesystem::path& path, const TriangleMeshLoadOptions& options) {
  std::ostringstream stream;
  stream << path.lexically_normal().generic_string() << "|scale=" << options.scale
         << "|recenter=" << (options.recenter ? 1 : 0)
         << "|normalize=" << (options.normalize ? 1 : 0);
  return stream.str();
}

}  // namespace

TriangleMesh loadTriangleMesh(const std::filesystem::path& path, const TriangleMeshLoadOptions& options) {
  const std::string extension = toLower(path.extension().string());
  TriangleMesh mesh;
  if (extension == ".obj") {
    mesh = loadObjMesh(path);
  } else if (extension == ".stl") {
    mesh = loadAsciiStlMesh(path);
  } else {
    throw std::runtime_error("Unsupported mesh format: " + path.string());
  }
  finalizeMesh(&mesh, options);
  return mesh;
}

std::shared_ptr<TriangleMesh> loadTriangleMeshShared(const std::filesystem::path& path,
                                                     const TriangleMeshLoadOptions& options) {
  static std::map<std::string, std::weak_ptr<TriangleMesh>> cache;

  const std::string key = cacheKey(path, options);
  const auto it = cache.find(key);
  if (it != cache.end()) {
    if (auto cached = it->second.lock()) {
      return cached;
    }
  }

  auto mesh = std::make_shared<TriangleMesh>(loadTriangleMesh(path, options));
  cache[key] = mesh;
  return mesh;
}

}  // namespace baseline
