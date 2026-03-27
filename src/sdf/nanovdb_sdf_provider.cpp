#include "baseline/sdf/nanovdb_sdf_provider.h"

namespace baseline {

NanoVdbSdfProvider::NanoVdbSdfProvider(std::string provider_name,
                                       ReferenceGeometry geometry,
                                       double voxel_size,
                                       double narrow_band)
    : AnalyticNarrowBandSdfProvider(
          std::move(provider_name),
          BASELINE_HAS_NANOVDB ? "nanovdb-adapter-skeleton" : "mock-nanovdb-analytic",
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
