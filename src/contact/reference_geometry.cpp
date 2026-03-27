#include "baseline/contact/reference_geometry.h"

#include <algorithm>
#include <array>
#include <cmath>

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

DistanceQueryResult makeGenericResult(const ReferenceGeometry& a,
                                      const ReferenceGeometry& b,
                                      std::string backend_name) {
  const Vec3 point_a = a.closestPoint(b.center);
  const Vec3 point_b = b.closestPoint(a.center);
  const Vec3 direction = point_b - point_a;
  const Vec3 normal = normalized(direction, normalized(b.center - a.center, {1.0, 0.0, 0.0}));
  const double signed_gap = dot(direction, normal);
  return {
      signed_gap <= 0.0,
      std::max(0.0, signed_gap),
      normal,
      point_a,
      point_b,
      std::move(backend_name),
  };
}

DistanceQueryResult sphereSphereDistance(const ReferenceGeometry& a,
                                        const ReferenceGeometry& b,
                                        std::string backend_name) {
  const Vec3 center_delta = b.center - a.center;
  const Vec3 normal = normalized(center_delta, {1.0, 0.0, 0.0});
  const Vec3 point_a = a.center + normal * a.radius;
  const Vec3 point_b = b.center - normal * b.radius;
  const double signed_gap = dot(point_b - point_a, normal);
  return {signed_gap <= 0.0, std::max(0.0, signed_gap), normal, point_a, point_b, std::move(backend_name)};
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
  return {
      signed_gap <= 0.0,
      std::max(0.0, signed_gap),
      normal,
      point_sphere,
      closest_on_box,
      std::move(backend_name),
  };
}

DistanceQueryResult boxBoxDistance(const ReferenceGeometry& a,
                                   const ReferenceGeometry& b,
                                   std::string backend_name) {
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
  const double distance = norm(outside);
  Vec3 normal{1.0, 0.0, 0.0};
  if (distance > 1e-12) {
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
  return {distance <= 0.0, distance, normal, point_a, point_b, std::move(backend_name)};
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
  ReferenceGeometry geometry;
  geometry.name = std::move(name);
  geometry.type = ShapeType::Box;
  geometry.center = center;
  geometry.half_extents = half_extents;
  geometry.radius = 0.0;
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

double ReferenceGeometry::signedDistance(const Vec3& query) const {
  if (type == ShapeType::Sphere) {
    return norm(query - center) - radius;
  }
  if (type == ShapeType::Plane) {
    return dot(query - center, plane_normal);
  }

  const Vec3 local = query - center;
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

  const Vec3 lower = center - half_extents;
  const Vec3 upper = center + half_extents;
  Vec3 point = clampVec(query, lower, upper);

  if (signedDistance(query) < 0.0) {
    const std::array<double, 3> distances = {
        std::abs(upper.x - query.x) < std::abs(query.x - lower.x) ? std::abs(upper.x - query.x)
                                                                  : std::abs(query.x - lower.x),
        std::abs(upper.y - query.y) < std::abs(query.y - lower.y) ? std::abs(upper.y - query.y)
                                                                  : std::abs(query.y - lower.y),
        std::abs(upper.z - query.z) < std::abs(query.z - lower.z) ? std::abs(upper.z - query.z)
                                                                  : std::abs(query.z - lower.z),
    };
    int axis = 0;
    if (distances[1] < distances[axis]) {
      axis = 1;
    }
    if (distances[2] < distances[axis]) {
      axis = 2;
    }
    if (axis == 0) {
      point.x = (std::abs(upper.x - query.x) < std::abs(query.x - lower.x)) ? upper.x : lower.x;
      point.y = query.y;
      point.z = query.z;
    } else if (axis == 1) {
      point.x = query.x;
      point.y = (std::abs(upper.y - query.y) < std::abs(query.y - lower.y)) ? upper.y : lower.y;
      point.z = query.z;
    } else {
      point.x = query.x;
      point.y = query.y;
      point.z = (std::abs(upper.z - query.z) < std::abs(query.z - lower.z)) ? upper.z : lower.z;
    }
  }
  return point;
}

Vec3 ReferenceGeometry::normalAt(const Vec3& query) const {
  if (type == ShapeType::Sphere) {
    return normalized(query - center, {1.0, 0.0, 0.0});
  }
  if (type == ShapeType::Plane) {
    return plane_normal;
  }

  if (signedDistance(query) >= 0.0) {
    return normalized(query - closestPoint(query), {1.0, 0.0, 0.0});
  }

  const Vec3 local = query - center;
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
  return normal;
}

double ReferenceGeometry::boundingRadius() const {
  if (type == ShapeType::Sphere) {
    return radius;
  }
  if (type == ShapeType::Plane) {
    return 1.0e9;
  }
  return norm(half_extents);
}

std::string AnalyticReferenceBackend::name() const { return "analytic"; }

DistanceQueryResult AnalyticReferenceBackend::distance(const ReferenceGeometry& a,
                                                       const ReferenceGeometry& b) const {
  return analyticDistance(a, b, "analytic-fallback");
}

bool HppFclReferenceBackend::realBackendAvailable() { return BASELINE_REAL_HPP_FCL_AVAILABLE != 0; }

std::string HppFclReferenceBackend::availabilitySummary() {
  if (realBackendAvailable()) {
    return "hpp-fcl dependency detected; adapter skeleton is available and currently delegates to analytic reference geometry.";
  }
  return "hpp-fcl dependency not detected; backend remains a compile-time skeleton only.";
}

std::string HppFclReferenceBackend::name() const {
  return realBackendAvailable() ? "hppfcl-adapter-skeleton" : "hppfcl-unavailable-skeleton";
}

bool HppFclReferenceBackend::available() const { return realBackendAvailable(); }

DistanceQueryResult HppFclReferenceBackend::distance(const ReferenceGeometry& a,
                                                     const ReferenceGeometry& b) const {
  return analyticDistance(a, b, name());
}

bool FclReferenceBackend::realBackendAvailable() { return BASELINE_REAL_FCL_AVAILABLE != 0; }

std::string FclReferenceBackend::availabilitySummary() {
  if (realBackendAvailable()) {
    return "FCL dependency detected; adapter skeleton is available and currently delegates to analytic reference geometry.";
  }
  return "FCL dependency not detected; backend remains a compile-time skeleton only.";
}

std::string FclReferenceBackend::name() const {
  return realBackendAvailable() ? "fcl-adapter-skeleton" : "fcl-unavailable-skeleton";
}

bool FclReferenceBackend::available() const { return realBackendAvailable(); }

DistanceQueryResult FclReferenceBackend::distance(const ReferenceGeometry& a, const ReferenceGeometry& b) const {
  return analyticDistance(a, b, name());
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
