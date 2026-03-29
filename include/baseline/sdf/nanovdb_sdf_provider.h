#pragma once

#include "baseline/sdf/sdf_provider.h"

namespace baseline {

class NanoVdbSdfProvider final : public SdfProvider {
 public:
  NanoVdbSdfProvider(std::string provider_name, ReferenceGeometry geometry, double voxel_size, double narrow_band);

  static bool realBackendAvailable();
  static std::string availabilitySummary();

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

  std::string name() const override;
  std::string backendName() const override;
  Vec3 referencePoint() const override;
  double voxelSize() const override;
  double narrowBand() const override;
  double boundingRadius() const override;
  Aabb3 worldAabb() const override;
  bool isNarrowBand(const Vec3& query) const override;
  double samplePhi(const Vec3& query) const override;
  Vec3 sampleGrad(const Vec3& query) const override;
  SdfSample samplePhiGrad(const Vec3& query) const override;

  const ReferenceGeometry& geometry() const;

 private:
  std::string provider_name_;
  std::string backend_name_;
  ReferenceGeometry geometry_;
  AnalyticNarrowBandSdfProvider fallback_;
};

}  // namespace baseline
