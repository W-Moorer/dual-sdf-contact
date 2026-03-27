#include "baseline/sdf/openvdb_sdf_provider.h"

namespace baseline {

bool OpenVdbSdfProvider::realBackendAvailable() { return BASELINE_REAL_OPENVDB_AVAILABLE != 0; }

std::string OpenVdbSdfProvider::availabilitySummary() {
  if (realBackendAvailable()) {
    return "OpenVDB dependency detected; adapter skeleton is available and currently delegates to analytic sampling.";
  }
  return "OpenVDB dependency not detected; this provider remains a compile-time skeleton only.";
}

OpenVdbSdfProvider::OpenVdbSdfProvider(std::string provider_name,
                                       ReferenceGeometry geometry,
                                       double voxel_size,
                                       double narrow_band)
    : AnalyticNarrowBandSdfProvider(
          std::move(provider_name),
          realBackendAvailable() ? "openvdb-adapter-skeleton" : "openvdb-unavailable-skeleton",
          std::move(geometry),
          voxel_size,
          narrow_band) {}

OpenVdbSdfProvider OpenVdbSdfProvider::makeSphere(std::string provider_name,
                                                  const Vec3& center,
                                                  double radius,
                                                  double voxel_size,
                                                  double narrow_band) {
  return OpenVdbSdfProvider(
      std::move(provider_name), ReferenceGeometry::makeSphere("openvdb-sphere", center, radius), voxel_size, narrow_band);
}

OpenVdbSdfProvider OpenVdbSdfProvider::makeBox(std::string provider_name,
                                               const Vec3& center,
                                               const Vec3& half_extents,
                                               double voxel_size,
                                               double narrow_band) {
  return OpenVdbSdfProvider(
      std::move(provider_name), ReferenceGeometry::makeBox("openvdb-box", center, half_extents), voxel_size, narrow_band);
}

}  // namespace baseline
