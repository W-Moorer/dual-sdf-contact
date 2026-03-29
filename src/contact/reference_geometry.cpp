#include "baseline/contact/reference_geometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#if BASELINE_REAL_FCL_AVAILABLE
#include <fcl/fcl.h>
#endif

namespace baseline {

namespace {

Vec3 absVec(const Vec3& value) { return {std::abs(value.x), std::abs(value.y), std::abs(value.z)}; }

Vec3 clampVec(const Vec3& value, const Vec3& lower, const Vec3& upper) {
  return {
      clamp(value.x, lower.x, upper.x),
      clamp(value.y, lower.y, upper.y),
      clamp(value.z, lower.z, upper.z),
  };
}

struct ClosestPointResult {
  Vec3 point{0.0, 0.0, 0.0};
  Vec3 normal{1.0, 0.0, 0.0};
  double distance_sq{0.0};
};

Vec3 localTriangleNormal(const Vec3& a, const Vec3& b, const Vec3& c) {
  return normalized(cross(b - a, c - a), {1.0, 0.0, 0.0});
}

ClosestPointResult closestPointOnTriangle(const Vec3& query, const Vec3& a, const Vec3& b, const Vec3& c) {
  const Vec3 ab = b - a;
  const Vec3 ac = c - a;
  const Vec3 ap = query - a;
  const double d1 = dot(ab, ap);
  const double d2 = dot(ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0) {
    return {a, localTriangleNormal(a, b, c), squaredNorm(query - a)};
  }

  const Vec3 bp = query - b;
  const double d3 = dot(ab, bp);
  const double d4 = dot(ac, bp);
  if (d3 >= 0.0 && d4 <= d3) {
    return {b, localTriangleNormal(a, b, c), squaredNorm(query - b)};
  }

  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
    const double v = d1 / (d1 - d3);
    const Vec3 point = a + ab * v;
    return {point, localTriangleNormal(a, b, c), squaredNorm(query - point)};
  }

  const Vec3 cp = query - c;
  const double d5 = dot(ab, cp);
  const double d6 = dot(ac, cp);
  if (d6 >= 0.0 && d5 <= d6) {
    return {c, localTriangleNormal(a, b, c), squaredNorm(query - c)};
  }

  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
    const double w = d2 / (d2 - d6);
    const Vec3 point = a + ac * w;
    return {point, localTriangleNormal(a, b, c), squaredNorm(query - point)};
  }

  const double va = d3 * d6 - d5 * d4;
  if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
    const Vec3 bc = c - b;
    const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    const Vec3 point = b + bc * w;
    return {point, localTriangleNormal(a, b, c), squaredNorm(query - point)};
  }

  const double denominator = 1.0 / (va + vb + vc);
  const double v = vb * denominator;
  const double w = vc * denominator;
  const Vec3 point = a + ab * v + ac * w;
  return {point, localTriangleNormal(a, b, c), squaredNorm(query - point)};
}

bool rayIntersectsTriangle(const Vec3& origin,
                           const Vec3& direction,
                           const Vec3& a,
                           const Vec3& b,
                           const Vec3& c) {
  const double epsilon = 1e-10;
  const Vec3 edge1 = b - a;
  const Vec3 edge2 = c - a;
  const Vec3 p = cross(direction, edge2);
  const double determinant = dot(edge1, p);
  if (std::abs(determinant) <= epsilon) {
    return false;
  }
  const double inverse_determinant = 1.0 / determinant;
  const Vec3 t = origin - a;
  const double u = dot(t, p) * inverse_determinant;
  if (u < 0.0 || u > 1.0) {
    return false;
  }
  const Vec3 q = cross(t, edge1);
  const double v = dot(direction, q) * inverse_determinant;
  if (v < 0.0 || u + v > 1.0) {
    return false;
  }
  const double distance = dot(edge2, q) * inverse_determinant;
  return distance > epsilon;
}

ClosestPointResult closestPointOnMesh(const TriangleMesh& mesh, const Vec3& local_query) {
  ClosestPointResult best;
  best.point = mesh.vertices.front();
  best.distance_sq = squaredNorm(local_query - best.point);

  for (const auto& triangle : mesh.triangles) {
    const ClosestPointResult candidate = closestPointOnTriangle(
        local_query, mesh.vertices[triangle[0]], mesh.vertices[triangle[1]], mesh.vertices[triangle[2]]);
    if (candidate.distance_sq < best.distance_sq) {
      best = candidate;
    }
  }
  return best;
}

bool pointInsideMesh(const TriangleMesh& mesh, const Vec3& local_query) {
  const Vec3 ray_direction = normalized({1.0, 0.3125, 0.125}, {1.0, 0.0, 0.0});
  int intersections = 0;
  for (const auto& triangle : mesh.triangles) {
    if (rayIntersectsTriangle(
            local_query, ray_direction, mesh.vertices[triangle[0]], mesh.vertices[triangle[1]], mesh.vertices[triangle[2]])) {
      ++intersections;
    }
  }
  return (intersections % 2) == 1;
}

Aabb3 transformAabb(const Aabb3& local_aabb, const ReferenceGeometry& geometry) {
  if (!local_aabb.valid) {
    return {};
  }

  Aabb3 world;
  for (int sx : {0, 1}) {
    for (int sy : {0, 1}) {
      for (int sz : {0, 1}) {
        const Vec3 local_corner{
            sx == 0 ? local_aabb.lower.x : local_aabb.upper.x,
            sy == 0 ? local_aabb.lower.y : local_aabb.upper.y,
            sz == 0 ? local_aabb.lower.z : local_aabb.upper.z,
        };
        const Vec3 world_corner = geometry.toWorldPoint(local_corner);
        if (!world.valid) {
          world.valid = true;
          world.lower = world_corner;
          world.upper = world_corner;
        } else {
          world.lower.x = std::min(world.lower.x, world_corner.x);
          world.lower.y = std::min(world.lower.y, world_corner.y);
          world.lower.z = std::min(world.lower.z, world_corner.z);
          world.upper.x = std::max(world.upper.x, world_corner.x);
          world.upper.y = std::max(world.upper.y, world_corner.y);
          world.upper.z = std::max(world.upper.z, world_corner.z);
        }
      }
    }
  }
  return world;
}

std::vector<Vec3> boxCorners(const ReferenceGeometry& box) {
  std::vector<Vec3> corners;
  corners.reserve(8);
  for (int sx : {-1, 1}) {
    for (int sy : {-1, 1}) {
      for (int sz : {-1, 1}) {
        corners.push_back(box.toWorldPoint(
            {sx * box.half_extents.x, sy * box.half_extents.y, sz * box.half_extents.z}));
      }
    }
  }
  return corners;
}

DistanceQueryResult makeDistanceResult(bool collision,
                                       double signed_distance,
                                       const Vec3& normal,
                                       const Vec3& point_a,
                                       const Vec3& point_b,
                                       std::string backend_name) {
  return {
      collision,
      std::max(0.0, signed_distance),
      signed_distance,
      normalized(normal, {1.0, 0.0, 0.0}),
      point_a,
      point_b,
      std::move(backend_name),
  };
}

DistanceQueryResult makeGenericResult(const ReferenceGeometry& a,
                                      const ReferenceGeometry& b,
                                      std::string backend_name) {
  const Vec3 point_a = a.closestPoint(b.center);
  const Vec3 point_b = b.closestPoint(a.center);
  const Vec3 direction = point_b - point_a;
  const Vec3 normal = normalized(direction, normalized(b.center - a.center, {1.0, 0.0, 0.0}));
  double signed_gap = dot(direction, normal);
  if (a.signedDistance(b.center) <= 0.0 || b.signedDistance(a.center) <= 0.0) {
    signed_gap = -std::max(1e-9, std::min(std::abs(a.signedDistance(b.center)), std::abs(b.signedDistance(a.center))));
  }
  return makeDistanceResult(signed_gap <= 0.0, signed_gap, normal, point_a, point_b, std::move(backend_name));
}

DistanceQueryResult sphereSphereDistance(const ReferenceGeometry& a,
                                         const ReferenceGeometry& b,
                                         std::string backend_name) {
  const Vec3 center_delta = b.center - a.center;
  const Vec3 normal = normalized(center_delta, {1.0, 0.0, 0.0});
  const Vec3 point_a = a.center + normal * a.radius;
  const Vec3 point_b = b.center - normal * b.radius;
  const double signed_gap = dot(point_b - point_a, normal);
  return makeDistanceResult(signed_gap <= 0.0, signed_gap, normal, point_a, point_b, std::move(backend_name));
}

DistanceQueryResult sphereBoxDistance(const ReferenceGeometry& sphere,
                                      const ReferenceGeometry& box,
                                      std::string backend_name) {
  const Vec3 closest_on_box = box.closestPoint(sphere.center);
  Vec3 direction = closest_on_box - sphere.center;
  if (norm(direction) <= 1e-12) {
    direction = box.normalAt(sphere.center);
  }
  const Vec3 normal = normalized(direction, {1.0, 0.0, 0.0});
  const Vec3 point_sphere = sphere.center + normal * sphere.radius;
  const double signed_gap = dot(closest_on_box - point_sphere, normal);
  return makeDistanceResult(
      signed_gap <= 0.0, signed_gap, normal, point_sphere, closest_on_box, std::move(backend_name));
}

DistanceQueryResult boxBoxDistance(const ReferenceGeometry& a,
                                   const ReferenceGeometry& b,
                                   std::string backend_name) {
  if (!a.hasIdentityRotation() || !b.hasIdentityRotation()) {
    Vec3 best_point_a = a.closestPoint(b.center);
    Vec3 best_point_b = b.closestPoint(a.center);
    double best_distance_sq = squaredNorm(best_point_b - best_point_a);

    for (const Vec3& corner : boxCorners(a)) {
      const Vec3 point_b = b.closestPoint(corner);
      const double distance_sq = squaredNorm(point_b - corner);
      if (distance_sq < best_distance_sq) {
        best_distance_sq = distance_sq;
        best_point_a = corner;
        best_point_b = point_b;
      }
    }
    for (const Vec3& corner : boxCorners(b)) {
      const Vec3 point_a = a.closestPoint(corner);
      const double distance_sq = squaredNorm(corner - point_a);
      if (distance_sq < best_distance_sq) {
        best_distance_sq = distance_sq;
        best_point_a = point_a;
        best_point_b = corner;
      }
    }

    const bool overlap = a.signedDistance(b.center) <= 0.0 || b.signedDistance(a.center) <= 0.0;
    const Vec3 fallback_normal = normalized(b.center - a.center, {1.0, 0.0, 0.0});
    const Vec3 normal = normalized(best_point_b - best_point_a, fallback_normal);
    if (overlap) {
      auto result = makeGenericResult(a, b, std::move(backend_name));
      result.closest_point_a = best_point_a;
      result.closest_point_b = best_point_b;
      result.normal = normal;
      if (result.signed_distance > 0.0) {
        result.distance = 0.0;
        result.signed_distance = -std::sqrt(best_distance_sq);
        result.collision = true;
      }
      return result;
    }
    const double signed_gap = dot(best_point_b - best_point_a, normal);
    return makeDistanceResult(
        signed_gap <= 0.0, signed_gap, normal, best_point_a, best_point_b, std::move(backend_name));
  }

  const Vec3 delta = b.center - a.center;
  const Vec3 combined = a.half_extents + b.half_extents;
  const Vec3 separation{
      std::abs(delta.x) - combined.x,
      std::abs(delta.y) - combined.y,
      std::abs(delta.z) - combined.z,
  };

  const Vec3 outside{
      std::max(separation.x, 0.0),
      std::max(separation.y, 0.0),
      std::max(separation.z, 0.0),
  };
  const double positive_distance = norm(outside);

  Vec3 normal{1.0, 0.0, 0.0};
  double signed_distance = positive_distance;
  if (positive_distance > 1e-12) {
    normal = normalized({delta.x == 0.0 ? outside.x : std::copysign(outside.x, delta.x),
                         delta.y == 0.0 ? outside.y : std::copysign(outside.y, delta.y),
                         delta.z == 0.0 ? outside.z : std::copysign(outside.z, delta.z)},
                        {1.0, 0.0, 0.0});
  } else {
    const std::array<double, 3> penetration = {-separation.x, -separation.y, -separation.z};
    const std::array<double, 3> signed_delta = {delta.x, delta.y, delta.z};
    int axis = 0;
    if (penetration[1] < penetration[axis]) {
      axis = 1;
    }
    if (penetration[2] < penetration[axis]) {
      axis = 2;
    }
    signed_distance = -penetration[axis];
    normal = {0.0, 0.0, 0.0};
    if (axis == 0) {
      normal.x = (signed_delta[0] >= 0.0) ? 1.0 : -1.0;
    } else if (axis == 1) {
      normal.y = (signed_delta[1] >= 0.0) ? 1.0 : -1.0;
    } else {
      normal.z = (signed_delta[2] >= 0.0) ? 1.0 : -1.0;
    }
  }

  const Vec3 point_a = a.closestPoint(b.center - normal * b.boundingRadius());
  const Vec3 point_b = b.closestPoint(a.center + normal * a.boundingRadius());
  return makeDistanceResult(
      signed_distance <= 0.0, signed_distance, normal, point_a, point_b, std::move(backend_name));
}

DistanceQueryResult analyticDistance(const ReferenceGeometry& a,
                                     const ReferenceGeometry& b,
                                     std::string backend_name) {
  if (a.type == ShapeType::Sphere && b.type == ShapeType::Sphere) {
    return sphereSphereDistance(a, b, std::move(backend_name));
  }
  if (a.type == ShapeType::Sphere && b.type == ShapeType::Box) {
    return sphereBoxDistance(a, b, std::move(backend_name));
  }
  if (a.type == ShapeType::Box && b.type == ShapeType::Sphere) {
    auto result = sphereBoxDistance(b, a, std::move(backend_name));
    std::swap(result.closest_point_a, result.closest_point_b);
    result.normal = -result.normal;
    return result;
  }
  if (a.type == ShapeType::Box && b.type == ShapeType::Box) {
    return boxBoxDistance(a, b, std::move(backend_name));
  }
  return makeGenericResult(a, b, std::move(backend_name));
}

#if BASELINE_REAL_FCL_AVAILABLE

fcl::Vector3d toFcl(const Vec3& value) { return {value.x, value.y, value.z}; }

Vec3 fromFcl(const fcl::Vector3d& value) { return {value.x(), value.y(), value.z()}; }

struct FclShapeInstance {
  std::shared_ptr<fcl::CollisionGeometryd> geometry;
  fcl::Transform3d transform = fcl::Transform3d::Identity();
};

Vec3 refinedReferenceNormal(const ReferenceGeometry& a,
                            const ReferenceGeometry& b,
                            const Vec3& point_a,
                            const Vec3& point_b,
                            const Vec3& fallback_normal) {
  const double probe_distance = std::max(1e-4, 1e-3 * std::max(a.boundingRadius(), b.boundingRadius()));
  const Vec3 separation_normal = normalized(point_b - point_a, fallback_normal);
  const Vec3 normal_a = a.normalAt(point_a + fallback_normal * probe_distance);
  const Vec3 normal_b = -b.normalAt(point_b - fallback_normal * probe_distance);
  Vec3 refined = normalized(normal_a + normal_b, separation_normal);
  if (dot(refined, fallback_normal) < 0.0) {
    refined = -refined;
  }
  return refined;
}

std::optional<FclShapeInstance> makeFclShape(const ReferenceGeometry& geometry) {
  using MeshModel = fcl::BVHModel<fcl::OBBRSSd>;

  FclShapeInstance shape;
  shape.transform = fcl::Transform3d::Identity();
  shape.transform.translation() = toFcl(geometry.center);
  shape.transform.linear() << geometry.rotation[0][0], geometry.rotation[0][1], geometry.rotation[0][2],
      geometry.rotation[1][0], geometry.rotation[1][1], geometry.rotation[1][2], geometry.rotation[2][0],
      geometry.rotation[2][1], geometry.rotation[2][2];

  if (geometry.type == ShapeType::Sphere) {
    shape.geometry = std::make_shared<fcl::Sphered>(geometry.radius);
    return shape;
  }

  if (geometry.type == ShapeType::Box) {
    shape.geometry = std::make_shared<fcl::Boxd>(
        2.0 * geometry.half_extents.x, 2.0 * geometry.half_extents.y, 2.0 * geometry.half_extents.z);
    return shape;
  }

  if (geometry.type == ShapeType::Mesh && geometry.mesh) {
    static std::map<const TriangleMesh*, std::weak_ptr<fcl::CollisionGeometryd>> cache;
    const auto cached = cache.find(geometry.mesh.get());
    if (cached != cache.end()) {
      if (auto existing = cached->second.lock()) {
        shape.geometry = existing;
        return shape;
      }
    }

    auto model = std::make_shared<MeshModel>();
    std::vector<fcl::Vector3d> vertices;
    vertices.reserve(geometry.mesh->vertices.size());
    for (const Vec3& vertex : geometry.mesh->vertices) {
      vertices.push_back(toFcl(vertex));
    }
    std::vector<fcl::Triangle> triangles;
    triangles.reserve(geometry.mesh->triangles.size());
    for (const auto& triangle : geometry.mesh->triangles) {
      triangles.emplace_back(triangle[0], triangle[1], triangle[2]);
    }
    model->beginModel();
    model->addSubModel(vertices, triangles);
    model->endModel();
    shape.geometry = model;
    cache[geometry.mesh.get()] = model;
    return shape;
  }

  return std::nullopt;
}

DistanceQueryResult fclDistance(const ReferenceGeometry& a,
                                const ReferenceGeometry& b,
                                std::string backend_name) {
  const auto shape_a = makeFclShape(a);
  const auto shape_b = makeFclShape(b);
  if (!shape_a || !shape_b) {
    return analyticDistance(a, b, std::move(backend_name));
  }

  fcl::CollisionObjectd object_a(shape_a->geometry, shape_a->transform);
  fcl::CollisionObjectd object_b(shape_b->geometry, shape_b->transform);

  fcl::DistanceRequestd distance_request(true, true);
  fcl::DistanceResultd distance_result;
  const double signed_distance = fcl::distance(&object_a, &object_b, distance_request, distance_result);

  fcl::CollisionRequestd collision_request(1, true);
  fcl::CollisionResultd collision_result;
  (void)fcl::collide(&object_a, &object_b, collision_request, collision_result);

  Vec3 point_a = fromFcl(distance_result.nearest_points[0]);
  Vec3 point_b = fromFcl(distance_result.nearest_points[1]);
  Vec3 fallback_normal = normalized(b.center - a.center, {1.0, 0.0, 0.0});
  Vec3 normal = normalized(point_b - point_a, fallback_normal);
  const bool collision = collision_result.isCollision() || signed_distance <= 0.0;

  if (collision_result.isCollision() && collision_result.numContacts() > 0) {
    const auto& contact = collision_result.getContact(0);
    normal = normalized(fromFcl(contact.normal), fallback_normal);
    if (norm(point_b - point_a) <= 1e-10) {
      const Vec3 contact_point = fromFcl(contact.pos);
      point_a = contact_point - normal * (0.5 * contact.penetration_depth);
      point_b = contact_point + normal * (0.5 * contact.penetration_depth);
    }
  }

  if (collision && norm(point_b - point_a) <= 1e-10) {
    point_a = a.closestPoint(b.center);
    point_b = b.closestPoint(a.center);
  }
  normal = refinedReferenceNormal(a, b, point_a, point_b, normal);
  if (dot(normal, fallback_normal) < 0.0) {
    normal = -normal;
  }

  return makeDistanceResult(collision, signed_distance, normal, point_a, point_b, std::move(backend_name));
}

#endif

}  // namespace

ReferenceGeometry ReferenceGeometry::makeSphere(std::string name, const Vec3& center, double radius) {
  ReferenceGeometry geometry;
  geometry.name = std::move(name);
  geometry.type = ShapeType::Sphere;
  geometry.center = center;
  geometry.radius = radius;
  geometry.half_extents = {radius, radius, radius};
  return geometry;
}

ReferenceGeometry ReferenceGeometry::makeBox(std::string name, const Vec3& center, const Vec3& half_extents) {
  return makeBox(std::move(name), center, half_extents, identityMat3());
}

ReferenceGeometry ReferenceGeometry::makeBox(std::string name,
                                             const Vec3& center,
                                             const Vec3& half_extents,
                                             const Mat3& rotation) {
  ReferenceGeometry geometry;
  geometry.name = std::move(name);
  geometry.type = ShapeType::Box;
  geometry.center = center;
  geometry.half_extents = half_extents;
  geometry.radius = 0.0;
  geometry.rotation = rotation;
  return geometry;
}

ReferenceGeometry ReferenceGeometry::makePlane(std::string name, const Vec3& point, const Vec3& normal) {
  ReferenceGeometry geometry;
  geometry.name = std::move(name);
  geometry.type = ShapeType::Plane;
  geometry.center = point;
  geometry.plane_normal = normalized(normal, {0.0, 1.0, 0.0});
  geometry.radius = 0.0;
  geometry.half_extents = {0.0, 0.0, 0.0};
  return geometry;
}

ReferenceGeometry ReferenceGeometry::makeMesh(std::string name,
                                              const Vec3& center,
                                              std::shared_ptr<const TriangleMesh> mesh) {
  return makeMesh(std::move(name), center, std::move(mesh), identityMat3());
}

ReferenceGeometry ReferenceGeometry::makeMesh(std::string name,
                                              const Vec3& center,
                                              std::shared_ptr<const TriangleMesh> mesh,
                                              const Mat3& rotation) {
  if (!mesh) {
    throw std::runtime_error("ReferenceGeometry::makeMesh requires a non-null TriangleMesh.");
  }

  ReferenceGeometry geometry;
  geometry.name = std::move(name);
  geometry.type = ShapeType::Mesh;
  geometry.center = center;
  geometry.rotation = rotation;
  geometry.mesh = std::move(mesh);
  geometry.radius = geometry.mesh->bounding_radius;
  geometry.half_extents = geometry.mesh->local_aabb.extent() * 0.5;
  return geometry;
}

double ReferenceGeometry::signedDistance(const Vec3& query) const {
  if (type == ShapeType::Sphere) {
    return norm(query - center) - radius;
  }
  if (type == ShapeType::Plane) {
    return dot(query - center, plane_normal);
  }
  if (type == ShapeType::Mesh) {
    const Vec3 local_query = toLocalPoint(query);
    const ClosestPointResult closest = closestPointOnMesh(*mesh, local_query);
    const double distance = std::sqrt(std::max(0.0, closest.distance_sq));
    return pointInsideMesh(*mesh, local_query) ? -distance : distance;
  }

  const Vec3 local = toLocalPoint(query);
  const Vec3 q = absVec(local) - half_extents;
  const Vec3 outside{std::max(q.x, 0.0), std::max(q.y, 0.0), std::max(q.z, 0.0)};
  const double outside_distance = norm(outside);
  const double inside_distance = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0);
  return outside_distance + inside_distance;
}

Vec3 ReferenceGeometry::closestPoint(const Vec3& query) const {
  if (type == ShapeType::Sphere) {
    return center + normalized(query - center, {1.0, 0.0, 0.0}) * radius;
  }
  if (type == ShapeType::Plane) {
    return query - plane_normal * signedDistance(query);
  }
  if (type == ShapeType::Mesh) {
    const Vec3 local_query = toLocalPoint(query);
    return toWorldPoint(closestPointOnMesh(*mesh, local_query).point);
  }

  const Vec3 local_query = toLocalPoint(query);
  const Vec3 lower = -half_extents;
  const Vec3 upper = half_extents;
  Vec3 point = clampVec(local_query, lower, upper);

  if (signedDistance(query) < 0.0) {
    const std::array<double, 3> distances = {
        std::abs(upper.x - local_query.x) < std::abs(local_query.x - lower.x) ? std::abs(upper.x - local_query.x)
                                                                               : std::abs(local_query.x - lower.x),
        std::abs(upper.y - local_query.y) < std::abs(local_query.y - lower.y) ? std::abs(upper.y - local_query.y)
                                                                               : std::abs(local_query.y - lower.y),
        std::abs(upper.z - local_query.z) < std::abs(local_query.z - lower.z) ? std::abs(upper.z - local_query.z)
                                                                               : std::abs(local_query.z - lower.z),
    };
    int axis = 0;
    if (distances[1] < distances[axis]) {
      axis = 1;
    }
    if (distances[2] < distances[axis]) {
      axis = 2;
    }
    if (axis == 0) {
      point.x = (std::abs(upper.x - local_query.x) < std::abs(local_query.x - lower.x)) ? upper.x : lower.x;
      point.y = local_query.y;
      point.z = local_query.z;
    } else if (axis == 1) {
      point.x = local_query.x;
      point.y = (std::abs(upper.y - local_query.y) < std::abs(local_query.y - lower.y)) ? upper.y : lower.y;
      point.z = local_query.z;
    } else {
      point.x = local_query.x;
      point.y = local_query.y;
      point.z = (std::abs(upper.z - local_query.z) < std::abs(local_query.z - lower.z)) ? upper.z : lower.z;
    }
  }
  return toWorldPoint(point);
}

Vec3 ReferenceGeometry::normalAt(const Vec3& query) const {
  if (type == ShapeType::Sphere) {
    return normalized(query - center, {1.0, 0.0, 0.0});
  }
  if (type == ShapeType::Plane) {
    return plane_normal;
  }
  if (type == ShapeType::Mesh) {
    const Vec3 local_query = toLocalPoint(query);
    const ClosestPointResult closest = closestPointOnMesh(*mesh, local_query);
    const Vec3 world_point = toWorldPoint(closest.point);
    Vec3 world_normal = toWorldDirection(closest.normal);
    const Vec3 offset = query - world_point;
    const double phi = signedDistance(query);
    if (phi >= 0.0) {
      if (dot(offset, world_normal) < 0.0) {
        world_normal = -world_normal;
      }
    } else if (dot(offset, world_normal) > 0.0) {
      world_normal = -world_normal;
    }
    return normalized(world_normal, {1.0, 0.0, 0.0});
  }

  if (signedDistance(query) >= 0.0) {
    return normalized(query - closestPoint(query), {1.0, 0.0, 0.0});
  }

  const Vec3 local = toLocalPoint(query);
  const Vec3 distance_to_face{
      half_extents.x - std::abs(local.x),
      half_extents.y - std::abs(local.y),
      half_extents.z - std::abs(local.z),
  };
  int axis = 0;
  if (distance_to_face.y < distance_to_face.x) {
    axis = 1;
  }
  if (distance_to_face.z < ((axis == 0) ? distance_to_face.x : distance_to_face.y)) {
    axis = 2;
  }

  Vec3 normal{0.0, 0.0, 0.0};
  if (axis == 0) {
    normal.x = (local.x >= 0.0) ? 1.0 : -1.0;
  } else if (axis == 1) {
    normal.y = (local.y >= 0.0) ? 1.0 : -1.0;
  } else {
    normal.z = (local.z >= 0.0) ? 1.0 : -1.0;
  }
  return toWorldDirection(normal);
}

double ReferenceGeometry::boundingRadius() const {
  if (type == ShapeType::Sphere) {
    return radius;
  }
  if (type == ShapeType::Plane) {
    return 1.0e9;
  }
  if (type == ShapeType::Mesh && mesh) {
    return mesh->bounding_radius;
  }
  return norm(half_extents);
}

Aabb3 ReferenceGeometry::localAabb() const {
  if (type == ShapeType::Plane) {
    return {};
  }
  if (type == ShapeType::Sphere) {
    return {{-radius, -radius, -radius}, {radius, radius, radius}, true};
  }
  if (type == ShapeType::Mesh && mesh) {
    return mesh->local_aabb;
  }
  return {-half_extents, half_extents, true};
}

Aabb3 ReferenceGeometry::worldAabb() const {
  if (type == ShapeType::Plane) {
    return {};
  }
  if (type == ShapeType::Sphere) {
    const Vec3 extents{radius, radius, radius};
    return {center - extents, center + extents, true};
  }
  return transformAabb(localAabb(), *this);
}

Vec3 ReferenceGeometry::toLocalPoint(const Vec3& world_point) const {
  return matMul(transpose(rotation), world_point - center);
}

Vec3 ReferenceGeometry::toWorldPoint(const Vec3& local_point) const { return center + matMul(rotation, local_point); }

Vec3 ReferenceGeometry::toLocalDirection(const Vec3& world_direction) const {
  return matMul(transpose(rotation), world_direction);
}

Vec3 ReferenceGeometry::toWorldDirection(const Vec3& local_direction) const {
  return normalized(matMul(rotation, local_direction), {1.0, 0.0, 0.0});
}

bool ReferenceGeometry::hasIdentityRotation(double tolerance) const {
  return isIdentityRotation(rotation, tolerance);
}

std::string AnalyticReferenceBackend::name() const { return "analytic"; }

DistanceQueryResult AnalyticReferenceBackend::distance(const ReferenceGeometry& a,
                                                       const ReferenceGeometry& b) const {
  return analyticDistance(a, b, "analytic");
}

bool HppFclReferenceBackend::realBackendAvailable() { return BASELINE_REAL_HPP_FCL_AVAILABLE != 0; }

std::string HppFclReferenceBackend::availabilitySummary() {
  if (realBackendAvailable()) {
    return "hpp-fcl detected, but this stage still keeps it in skeleton status; the backend falls back to analytic queries.";
  }
  return "hpp-fcl not detected; backend falls back to analytic queries.";
}

std::string HppFclReferenceBackend::name() const {
  return realBackendAvailable() ? "hppfcl-skeleton" : "hppfcl-fallback-analytic";
}

bool HppFclReferenceBackend::available() const { return realBackendAvailable(); }

DistanceQueryResult HppFclReferenceBackend::distance(const ReferenceGeometry& a,
                                                     const ReferenceGeometry& b) const {
  return analyticDistance(a, b, name());
}

bool FclReferenceBackend::realBackendAvailable() { return BASELINE_REAL_FCL_AVAILABLE != 0; }

std::string FclReferenceBackend::availabilitySummary() {
  if (realBackendAvailable()) {
    return "FCL detected and wired through fcl::distance / fcl::collide for sphere/box primitives and mesh-backed benchmark geometries.";
  }
  return "FCL not detected; backend falls back to analytic queries.";
}

std::string FclReferenceBackend::name() const {
  return realBackendAvailable() ? "fcl-real" : "fcl-fallback-analytic";
}

bool FclReferenceBackend::available() const { return realBackendAvailable(); }

DistanceQueryResult FclReferenceBackend::distance(const ReferenceGeometry& a, const ReferenceGeometry& b) const {
#if BASELINE_REAL_FCL_AVAILABLE
  return fclDistance(a, b, name());
#else
  return analyticDistance(a, b, name());
#endif
}

std::unique_ptr<ReferenceGeometryQueryEngine> makeDefaultReferenceGeometryQueryEngine() {
  if constexpr ((BASELINE_FORCE_FALLBACK_REFERENCE == 0) && (BASELINE_REAL_HPP_FCL_AVAILABLE != 0)) {
    return std::make_unique<HppFclReferenceBackend>();
  }
  if constexpr ((BASELINE_FORCE_FALLBACK_REFERENCE == 0) && (BASELINE_REAL_FCL_AVAILABLE != 0)) {
    return std::make_unique<FclReferenceBackend>();
  }
  return std::make_unique<AnalyticReferenceBackend>();
}

std::unique_ptr<ReferenceGeometryQueryEngine> makeReferenceGeometryQueryEngine(std::string_view backend_name) {
  const std::string token(backend_name);
  if (token == "hppfcl") {
    return std::make_unique<HppFclReferenceBackend>();
  }
  if (token == "fcl") {
    return std::make_unique<FclReferenceBackend>();
  }
  return std::make_unique<AnalyticReferenceBackend>();
}

}  // namespace baseline
