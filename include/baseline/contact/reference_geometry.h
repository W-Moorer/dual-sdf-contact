#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "baseline/core/math.h"
#include "baseline/core/build_config.h"

namespace baseline {

enum class ShapeType { Sphere, Box, Plane };

struct ReferenceGeometry {
  std::string name;
  ShapeType type{ShapeType::Sphere};
  Vec3 center{0.0, 0.0, 0.0};
  Vec3 half_extents{0.5, 0.5, 0.5};
  double radius{0.5};
  Vec3 plane_normal{0.0, 1.0, 0.0};

  static ReferenceGeometry makeSphere(std::string name, const Vec3& center, double radius);
  static ReferenceGeometry makeBox(std::string name, const Vec3& center, const Vec3& half_extents);
  static ReferenceGeometry makePlane(std::string name, const Vec3& point, const Vec3& normal);

  double signedDistance(const Vec3& query) const;
  Vec3 closestPoint(const Vec3& query) const;
  Vec3 normalAt(const Vec3& query) const;
  double boundingRadius() const;
};

struct DistanceQueryResult {
  bool collision{false};
  double distance{0.0};
  double signed_distance{0.0};
  Vec3 normal{1.0, 0.0, 0.0};
  Vec3 closest_point_a{0.0, 0.0, 0.0};
  Vec3 closest_point_b{0.0, 0.0, 0.0};
  std::string backend_name;
};

class ReferenceGeometryQueryEngine {
 public:
  virtual ~ReferenceGeometryQueryEngine() = default;
  virtual std::string name() const = 0;
  virtual bool available() const { return true; }
  virtual DistanceQueryResult distance(const ReferenceGeometry& a, const ReferenceGeometry& b) const = 0;
};

class AnalyticReferenceBackend final : public ReferenceGeometryQueryEngine {
 public:
  std::string name() const override;
  DistanceQueryResult distance(const ReferenceGeometry& a, const ReferenceGeometry& b) const override;
};

class HppFclReferenceBackend final : public ReferenceGeometryQueryEngine {
 public:
  std::string name() const override;
  bool available() const override;
  DistanceQueryResult distance(const ReferenceGeometry& a, const ReferenceGeometry& b) const override;
  static bool realBackendAvailable();
  static std::string availabilitySummary();

 private:
  AnalyticReferenceBackend fallback_;
};

class FclReferenceBackend final : public ReferenceGeometryQueryEngine {
 public:
  std::string name() const override;
  bool available() const override;
  DistanceQueryResult distance(const ReferenceGeometry& a, const ReferenceGeometry& b) const override;
  static bool realBackendAvailable();
  static std::string availabilitySummary();

 private:
  AnalyticReferenceBackend fallback_;
};

using AnalyticReferenceGeometryQueryEngine = AnalyticReferenceBackend;
using OptionalHppFclReferenceGeometryQueryEngine = HppFclReferenceBackend;

std::unique_ptr<ReferenceGeometryQueryEngine> makeDefaultReferenceGeometryQueryEngine();
std::unique_ptr<ReferenceGeometryQueryEngine> makeReferenceGeometryQueryEngine(std::string_view backend_name);

}  // namespace baseline
