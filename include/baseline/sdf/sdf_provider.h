#pragma once

#include <string>

#include "baseline/contact/reference_geometry.h"

namespace baseline {

struct SdfSample {
  double signed_distance{0.0};
  Vec3 gradient{1.0, 0.0, 0.0};
  bool in_narrow_band{true};
};

class SdfProvider {
 public:
  virtual ~SdfProvider() = default;
  virtual std::string name() const = 0;
  virtual std::string backendName() const = 0;
  virtual Vec3 referencePoint() const = 0;
  virtual double voxelSize() const = 0;
  virtual double narrowBand() const = 0;
  virtual double boundingRadius() const = 0;
  virtual Aabb3 worldAabb() const = 0;
  virtual bool isNarrowBand(const Vec3& query) const = 0;
  virtual double samplePhi(const Vec3& query) const = 0;
  virtual Vec3 sampleGrad(const Vec3& query) const = 0;
  virtual SdfSample samplePhiGrad(const Vec3& query) const;
  virtual SdfSample sample(const Vec3& query) const;
};

class AnalyticNarrowBandSdfProvider : public SdfProvider {
 public:
  AnalyticNarrowBandSdfProvider(std::string provider_name,
                                std::string backend_name,
                                ReferenceGeometry geometry,
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
  double narrowBandWorld() const;

 private:
  std::string provider_name_;
  std::string backend_name_;
  ReferenceGeometry geometry_;
  double voxel_size_{0.1};
  double narrow_band_{2.0};
  Aabb3 world_aabb_;
};

}  // namespace baseline
