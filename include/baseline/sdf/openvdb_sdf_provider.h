#pragma once

#include "baseline/sdf/sdf_provider.h"

namespace baseline {

class OpenVdbSdfProvider final : public AnalyticNarrowBandSdfProvider {
 public:
  OpenVdbSdfProvider(std::string provider_name, ReferenceGeometry geometry, double voxel_size, double narrow_band);

  static OpenVdbSdfProvider makeSphere(std::string provider_name,
                                       const Vec3& center,
                                       double radius,
                                       double voxel_size,
                                       double narrow_band);
};

}  // namespace baseline
