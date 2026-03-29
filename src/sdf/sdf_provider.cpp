#include "baseline/sdf/sdf_provider.h"

#include <algorithm>

namespace baseline {

namespace {

Aabb3 computeAnalyticWorldAabb(const ReferenceGeometry& geometry, double narrow_band_world) {
  Aabb3 bounds = geometry.worldAabb();
  if (!bounds.valid) {
    return {};
  }
  const Vec3 margin{narrow_band_world, narrow_band_world, narrow_band_world};
  bounds.lower = bounds.lower - margin;
  bounds.upper = bounds.upper + margin;
  return bounds;
}

}  // namespace

SdfSample SdfProvider::samplePhiGrad(const Vec3& query) const {
  return {samplePhi(query), sampleGrad(query), isNarrowBand(query)};
}

SdfSample SdfProvider::sample(const Vec3& query) const { return samplePhiGrad(query); }

AnalyticNarrowBandSdfProvider::AnalyticNarrowBandSdfProvider(std::string provider_name,
                                                             std::string backend_name,
                                                             ReferenceGeometry geometry,
                                                             double voxel_size,
                                                             double narrow_band)
    : provider_name_(std::move(provider_name)),
      backend_name_(std::move(backend_name)),
      geometry_(std::move(geometry)),
      voxel_size_(voxel_size),
      narrow_band_(narrow_band),
      world_aabb_(computeAnalyticWorldAabb(geometry_, narrowBandWorld())) {}

std::string AnalyticNarrowBandSdfProvider::name() const { return provider_name_; }

std::string AnalyticNarrowBandSdfProvider::backendName() const { return backend_name_; }

Vec3 AnalyticNarrowBandSdfProvider::referencePoint() const { return geometry_.center; }

double AnalyticNarrowBandSdfProvider::voxelSize() const { return voxel_size_; }

double AnalyticNarrowBandSdfProvider::narrowBand() const { return narrow_band_; }

double AnalyticNarrowBandSdfProvider::boundingRadius() const { return geometry_.boundingRadius(); }

Aabb3 AnalyticNarrowBandSdfProvider::worldAabb() const { return world_aabb_; }

bool AnalyticNarrowBandSdfProvider::isNarrowBand(const Vec3& query) const {
  if (!world_aabb_.valid) {
    return true;
  }
  return world_aabb_.contains(query, voxel_size_);
}

double AnalyticNarrowBandSdfProvider::samplePhi(const Vec3& query) const { return geometry_.signedDistance(query); }

Vec3 AnalyticNarrowBandSdfProvider::sampleGrad(const Vec3& query) const {
  const double h = (voxel_size_ > 0.0) ? std::max(1e-4, voxel_size_ * 0.5) : 1e-3;
  const Vec3 dx{h, 0.0, 0.0};
  const Vec3 dy{0.0, h, 0.0};
  const Vec3 dz{0.0, 0.0, h};
  const double gx = (samplePhi(query + dx) - samplePhi(query - dx)) / (2.0 * h);
  const double gy = (samplePhi(query + dy) - samplePhi(query - dy)) / (2.0 * h);
  const double gz = (samplePhi(query + dz) - samplePhi(query - dz)) / (2.0 * h);
  return normalized({gx, gy, gz}, geometry_.normalAt(query));
}

SdfSample AnalyticNarrowBandSdfProvider::samplePhiGrad(const Vec3& query) const {
  return {samplePhi(query), sampleGrad(query), isNarrowBand(query)};
}

const ReferenceGeometry& AnalyticNarrowBandSdfProvider::geometry() const { return geometry_; }

double AnalyticNarrowBandSdfProvider::narrowBandWorld() const { return voxel_size_ * narrow_band_; }

}  // namespace baseline
