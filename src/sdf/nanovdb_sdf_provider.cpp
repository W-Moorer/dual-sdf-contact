#include "baseline/sdf/nanovdb_sdf_provider.h"

namespace baseline {

bool NanoVdbSdfProvider::realBackendAvailable() { return BASELINE_REAL_NANOVDB_AVAILABLE != 0; }

std::string NanoVdbSdfProvider::availabilitySummary() {
  if (realBackendAvailable()) {
    return "NanoVDB dependency detected; adapter skeleton is available and currently delegates to analytic sampling.";
  }
  return "NanoVDB dependency not detected; this provider remains a compile-time skeleton only.";
}

NanoVdbSdfProvider::NanoVdbSdfProvider(std::string provider_name,
                                       ReferenceGeometry geometry,
                                       double voxel_size,
                                       double narrow_band)
    : AnalyticNarrowBandSdfProvider(
          std::move(provider_name),
          realBackendAvailable() ? "nanovdb-adapter-skeleton" : "nanovdb-unavailable-skeleton",
          std::move(geometry),
          voxel_size,
          narrow_band) {}

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

}  // namespace baseline
