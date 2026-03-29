#include "baseline/sdf/nanovdb_sdf_provider.h"

#include <utility>

#include "baseline/core/build_config.h"

namespace baseline {

bool NanoVdbSdfProvider::realBackendAvailable() { return BASELINE_REAL_NANOVDB_AVAILABLE != 0; }

std::string NanoVdbSdfProvider::availabilitySummary() {
  if (realBackendAvailable()) {
    return "NanoVDB headers detected. The current implementation keeps NanoVDB in optional/read-only status and falls back to the analytic provider.";
  }
  return "NanoVDB headers not detected; provider falls back to the analytic narrow-band implementation.";
}

NanoVdbSdfProvider::NanoVdbSdfProvider(std::string provider_name,
                                       ReferenceGeometry geometry,
                                       double voxel_size,
                                       double narrow_band)
    : provider_name_(std::move(provider_name)),
      backend_name_(realBackendAvailable() ? "nanovdb-readonly-skeleton" : "nanovdb-fallback-analytic"),
      geometry_(std::move(geometry)),
      fallback_(provider_name_, "analytic-fallback", geometry_, voxel_size, narrow_band) {}

NanoVdbSdfProvider NanoVdbSdfProvider::makeSphere(std::string provider_name,
                                                  const Vec3& center,
                                                  double radius,
                                                  double voxel_size,
                                                  double narrow_band) {
  return NanoVdbSdfProvider(
      std::move(provider_name), ReferenceGeometry::makeSphere("nanovdb-sphere", center, radius), voxel_size, narrow_band);
}

NanoVdbSdfProvider NanoVdbSdfProvider::makeBox(std::string provider_name,
                                               const Vec3& center,
                                               const Vec3& half_extents,
                                               double voxel_size,
                                               double narrow_band) {
  return NanoVdbSdfProvider(
      std::move(provider_name), ReferenceGeometry::makeBox("nanovdb-box", center, half_extents), voxel_size, narrow_band);
}

std::string NanoVdbSdfProvider::name() const { return provider_name_; }

std::string NanoVdbSdfProvider::backendName() const { return backend_name_; }

Vec3 NanoVdbSdfProvider::referencePoint() const { return geometry_.center; }

double NanoVdbSdfProvider::voxelSize() const { return fallback_.voxelSize(); }

double NanoVdbSdfProvider::narrowBand() const { return fallback_.narrowBand(); }

double NanoVdbSdfProvider::boundingRadius() const { return fallback_.boundingRadius(); }

Aabb3 NanoVdbSdfProvider::worldAabb() const { return fallback_.worldAabb(); }

bool NanoVdbSdfProvider::isNarrowBand(const Vec3& query) const { return fallback_.isNarrowBand(query); }

double NanoVdbSdfProvider::samplePhi(const Vec3& query) const { return fallback_.samplePhi(query); }

Vec3 NanoVdbSdfProvider::sampleGrad(const Vec3& query) const { return fallback_.sampleGrad(query); }

SdfSample NanoVdbSdfProvider::samplePhiGrad(const Vec3& query) const { return fallback_.samplePhiGrad(query); }

const ReferenceGeometry& NanoVdbSdfProvider::geometry() const { return geometry_; }

}  // namespace baseline
