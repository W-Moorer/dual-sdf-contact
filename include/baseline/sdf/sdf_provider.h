#pragma once

#include <string>

#include "baseline/contact/reference_geometry.h"

namespace baseline {

struct SdfSample {
  double signed_distance{0.0};
  Vec3 gradient{1.0, 0.0, 0.0};
};

class SdfProvider {
 public:
  virtual ~SdfProvider() = default;
  virtual std::string name() const = 0;
  virtual std::string backendName() const = 0;
  virtual Vec3 referencePoint() const = 0;
  virtual double narrowBand() const = 0;
  virtual double boundingRadius() const = 0;
  virtual SdfSample sample(const Vec3& query) const = 0;
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
  double narrowBand() const override;
  double boundingRadius() const override;
  SdfSample sample(const Vec3& query) const override;

  const ReferenceGeometry& geometry() const;
  double voxelSize() const;

 private:
  std::string provider_name_;
  std::string backend_name_;
  ReferenceGeometry geometry_;
  double voxel_size_{0.1};
  double narrow_band_{2.0};
};

}  // namespace baseline
