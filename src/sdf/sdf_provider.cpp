#include "baseline/sdf/sdf_provider.h"

namespace baseline {

AnalyticNarrowBandSdfProvider::AnalyticNarrowBandSdfProvider(std::string provider_name,
                                                             std::string backend_name,
                                                             ReferenceGeometry geometry,
                                                             double voxel_size,
                                                             double narrow_band)
    : provider_name_(std::move(provider_name)),
      backend_name_(std::move(backend_name)),
      geometry_(std::move(geometry)),
      voxel_size_(voxel_size),
      narrow_band_(narrow_band) {}

std::string AnalyticNarrowBandSdfProvider::name() const { return provider_name_; }

std::string AnalyticNarrowBandSdfProvider::backendName() const { return backend_name_; }

Vec3 AnalyticNarrowBandSdfProvider::referencePoint() const { return geometry_.center; }

double AnalyticNarrowBandSdfProvider::narrowBand() const { return narrow_band_; }

double AnalyticNarrowBandSdfProvider::boundingRadius() const { return geometry_.boundingRadius(); }

SdfSample AnalyticNarrowBandSdfProvider::sample(const Vec3& query) const {
  const double signed_distance = geometry_.signedDistance(query);
  const double h = (voxel_size_ > 0.0) ? voxel_size_ * 0.25 : 1e-3;
  const Vec3 dx{h, 0.0, 0.0};
  const Vec3 dy{0.0, h, 0.0};
  const Vec3 dz{0.0, 0.0, h};
  const double gx = (geometry_.signedDistance(query + dx) - geometry_.signedDistance(query - dx)) / (2.0 * h);
  const double gy = (geometry_.signedDistance(query + dy) - geometry_.signedDistance(query - dy)) / (2.0 * h);
  const double gz = (geometry_.signedDistance(query + dz) - geometry_.signedDistance(query - dz)) / (2.0 * h);
  return {signed_distance, normalized({gx, gy, gz}, geometry_.normalAt(query))};
}

const ReferenceGeometry& AnalyticNarrowBandSdfProvider::geometry() const { return geometry_; }

double AnalyticNarrowBandSdfProvider::voxelSize() const { return voxel_size_; }

}  // namespace baseline
