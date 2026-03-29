#include "baseline/sdf/openvdb_sdf_provider.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "baseline/core/build_config.h"

#if BASELINE_REAL_OPENVDB_AVAILABLE
#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/MeshToVolume.h>
#endif

namespace baseline {

class OpenVdbSdfProvider::Impl {
 public:
#if BASELINE_REAL_OPENVDB_AVAILABLE
  openvdb::FloatGrid::Ptr grid;
#endif
};

namespace {

Vec3 finiteDifferenceGradient(const SdfProvider& provider, const ReferenceGeometry& geometry, const Vec3& query) {
  const double h = std::max(1e-4, provider.voxelSize() * 0.5);
  const Vec3 dx{h, 0.0, 0.0};
  const Vec3 dy{0.0, h, 0.0};
  const Vec3 dz{0.0, 0.0, h};
  const double gx = (provider.samplePhi(query + dx) - provider.samplePhi(query - dx)) / (2.0 * h);
  const double gy = (provider.samplePhi(query + dy) - provider.samplePhi(query - dy)) / (2.0 * h);
  const double gz = (provider.samplePhi(query + dz) - provider.samplePhi(query - dz)) / (2.0 * h);
  return normalized({gx, gy, gz}, geometry.normalAt(query));
}

#if BASELINE_REAL_OPENVDB_AVAILABLE

openvdb::Vec3d toOpenVdb(const Vec3& value) { return {value.x, value.y, value.z}; }

Vec3 fromOpenVdb(const openvdb::Vec3d& value) { return {value.x(), value.y(), value.z()}; }

void ensureOpenVdbInitialized() {
  static const bool initialized = []() {
    openvdb::initialize();
    return true;
  }();
  (void)initialized;
}

Aabb3 computeGridWorldAabb(const openvdb::FloatGrid& grid) {
  const openvdb::CoordBBox bbox = grid.evalActiveVoxelBoundingBox();
  if (bbox.empty()) {
    return {};
  }

  const openvdb::Vec3d lower_world = grid.indexToWorld(bbox.min().asVec3d());
  const openvdb::Vec3d upper_world =
      grid.indexToWorld((bbox.max() + openvdb::Coord(1, 1, 1)).asVec3d());
  return {fromOpenVdb(lower_world), fromOpenVdb(upper_world), true};
}

openvdb::FloatGrid::Ptr buildOpenVdbGrid(const ReferenceGeometry& geometry,
                                         double voxel_size,
                                         double narrow_band) {
  ensureOpenVdbInitialized();

  if (geometry.type == ShapeType::Sphere) {
    return openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(
        static_cast<float>(geometry.radius),
        openvdb::Vec3f(static_cast<float>(geometry.center.x),
                       static_cast<float>(geometry.center.y),
                       static_cast<float>(geometry.center.z)),
        static_cast<float>(voxel_size),
        static_cast<float>(narrow_band));
  }

  if (geometry.type == ShapeType::Box) {
    auto transform = openvdb::math::Transform::createLinearTransform(voxel_size);
    const openvdb::math::BBox<openvdb::Vec3d> bbox(
        toOpenVdb(geometry.center - geometry.half_extents),
        toOpenVdb(geometry.center + geometry.half_extents));
    return openvdb::tools::createLevelSetBox<openvdb::FloatGrid>(
        bbox, *transform, static_cast<float>(narrow_band));
  }

  return nullptr;
}

double sampleOpenVdbPhi(const openvdb::FloatGrid& grid, const Vec3& query) {
  using Accessor = openvdb::FloatGrid::ConstAccessor;
  using Sampler = openvdb::tools::GridSampler<Accessor, openvdb::tools::BoxSampler>;

  Accessor accessor = grid.getConstAccessor();
  const Sampler sampler(accessor, grid.transform());
  return static_cast<double>(sampler.wsSample(toOpenVdb(query)));
}

bool isInsideOpenVdbBand(const openvdb::FloatGrid& grid,
                         const Aabb3& world_aabb,
                         const Vec3& query,
                         double narrow_band_world,
                         double voxel_size) {
  if (!world_aabb.valid || !world_aabb.contains(query, voxel_size)) {
    return false;
  }
  const double raw_phi = std::abs(sampleOpenVdbPhi(grid, query));
  const double trusted_limit = std::max(0.0, narrow_band_world - 0.5 * voxel_size);
  return raw_phi < trusted_limit;
}

#endif

}  // namespace

bool OpenVdbSdfProvider::realBackendAvailable() { return BASELINE_REAL_OPENVDB_AVAILABLE != 0; }

std::string OpenVdbSdfProvider::availabilitySummary() {
  if (realBackendAvailable()) {
    return "OpenVDB detected and compiled in. Sphere/box level sets use real grid generation and world-space sampling inside the trusted narrow band, with analytic extension outside that band for current primitive-backed examples.";
  }
  return "OpenVDB not detected; provider falls back to the analytic narrow-band implementation.";
}

OpenVdbSdfProvider::OpenVdbSdfProvider(std::string provider_name,
                                       ReferenceGeometry geometry,
                                       double voxel_size,
                                       double narrow_band)
    : provider_name_(std::move(provider_name)),
      backend_name_(realBackendAvailable() ? "openvdb-real" : "openvdb-fallback-analytic"),
      geometry_(std::move(geometry)),
      fallback_(provider_name_, "analytic-fallback", geometry_, voxel_size, narrow_band),
      voxel_size_(voxel_size),
      narrow_band_(narrow_band),
      world_aabb_(fallback_.worldAabb()),
      impl_(std::make_shared<Impl>()) {
#if BASELINE_REAL_OPENVDB_AVAILABLE
  try {
    impl_->grid = buildOpenVdbGrid(geometry_, voxel_size_, narrow_band_);
    if (impl_->grid) {
      world_aabb_ = computeGridWorldAabb(*impl_->grid);
    } else {
      backend_name_ = "openvdb-fallback-analytic";
    }
  } catch (...) {
    impl_->grid.reset();
    backend_name_ = "openvdb-fallback-analytic";
    world_aabb_ = fallback_.worldAabb();
  }
#else
  impl_.reset();
#endif
}

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

std::string OpenVdbSdfProvider::name() const { return provider_name_; }

std::string OpenVdbSdfProvider::backendName() const { return backend_name_; }

Vec3 OpenVdbSdfProvider::referencePoint() const { return geometry_.center; }

double OpenVdbSdfProvider::voxelSize() const { return voxel_size_; }

double OpenVdbSdfProvider::narrowBand() const { return narrow_band_; }

double OpenVdbSdfProvider::boundingRadius() const { return geometry_.boundingRadius(); }

Aabb3 OpenVdbSdfProvider::worldAabb() const { return world_aabb_; }

bool OpenVdbSdfProvider::isNarrowBand(const Vec3& query) const {
  const double narrow_band_world = voxel_size_ * narrow_band_;
#if BASELINE_REAL_OPENVDB_AVAILABLE
  if (impl_ && impl_->grid) {
    return isInsideOpenVdbBand(*impl_->grid, world_aabb_, query, narrow_band_world, voxel_size_);
  }
#endif
  if (!world_aabb_.valid) {
    return true;
  }
  return world_aabb_.contains(query, voxel_size_) &&
         std::abs(fallback_.samplePhi(query)) <= (narrow_band_world + voxel_size_);
}

double OpenVdbSdfProvider::samplePhi(const Vec3& query) const {
#if BASELINE_REAL_OPENVDB_AVAILABLE
  if (impl_ && impl_->grid) {
    if (isInsideOpenVdbBand(*impl_->grid, world_aabb_, query, voxel_size_ * narrow_band_, voxel_size_)) {
      return sampleOpenVdbPhi(*impl_->grid, query);
    }
    return fallback_.samplePhi(query);
  }
#endif
  return fallback_.samplePhi(query);
}

Vec3 OpenVdbSdfProvider::sampleGrad(const Vec3& query) const {
#if BASELINE_REAL_OPENVDB_AVAILABLE
  if (impl_ && impl_->grid) {
    if (isInsideOpenVdbBand(*impl_->grid, world_aabb_, query, voxel_size_ * narrow_band_, voxel_size_)) {
      return finiteDifferenceGradient(*this, geometry_, query);
    }
    return fallback_.sampleGrad(query);
  }
#endif
  return fallback_.sampleGrad(query);
}

SdfSample OpenVdbSdfProvider::samplePhiGrad(const Vec3& query) const {
  return {samplePhi(query), sampleGrad(query), isNarrowBand(query)};
}

const ReferenceGeometry& OpenVdbSdfProvider::geometry() const { return geometry_; }

}  // namespace baseline
