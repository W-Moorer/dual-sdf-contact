#pragma once

#include <memory>

#include "baseline/sdf/sdf_provider.h"

namespace baseline {

class OpenVdbSdfProvider final : public SdfProvider {
 public:
  OpenVdbSdfProvider(std::string provider_name, ReferenceGeometry geometry, double voxel_size, double narrow_band);

  static bool realBackendAvailable();
  static std::string availabilitySummary();

  static OpenVdbSdfProvider makeSphere(std::string provider_name,
                                       const Vec3& center,
                                       double radius,
                                       double voxel_size,
                                       double narrow_band);

  static OpenVdbSdfProvider makeBox(std::string provider_name,
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
  class Impl;

  std::string provider_name_;
  std::string backend_name_;
  ReferenceGeometry geometry_;
  AnalyticNarrowBandSdfProvider fallback_;
  double voxel_size_{0.1};
  double narrow_band_{2.0};
  Aabb3 world_aabb_;
  std::shared_ptr<Impl> impl_;
};

}  // namespace baseline
