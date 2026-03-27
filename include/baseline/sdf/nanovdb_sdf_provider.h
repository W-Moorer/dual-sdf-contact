#pragma once

#include "baseline/sdf/sdf_provider.h"

namespace baseline {

class NanoVdbSdfProvider final : public AnalyticNarrowBandSdfProvider {
 public:
  NanoVdbSdfProvider(std::string provider_name, ReferenceGeometry geometry, double voxel_size, double narrow_band);

  static NanoVdbSdfProvider makeSphere(std::string provider_name,
                                       const Vec3& center,
                                       double radius,
                                       double voxel_size,
                                       double narrow_band);

  static NanoVdbSdfProvider makeBox(std::string provider_name,
                                    const Vec3& center,
                                    const Vec3& half_extents,
                                    double voxel_size,
                                    double narrow_band);
};

}  // namespace baseline
