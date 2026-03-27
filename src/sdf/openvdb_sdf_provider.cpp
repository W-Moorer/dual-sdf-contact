#include "baseline/sdf/openvdb_sdf_provider.h"

namespace baseline {

OpenVdbSdfProvider::OpenVdbSdfProvider(std::string provider_name,
                                       ReferenceGeometry geometry,
                                       double voxel_size,
                                       double narrow_band)
    : AnalyticNarrowBandSdfProvider(
          std::move(provider_name),
          BASELINE_HAS_OPENVDB ? "openvdb-adapter-skeleton" : "mock-openvdb-analytic",
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

}  // namespace baseline
