#include "baseline/contact/dual_sdf_contact_calculator.h"

#include <algorithm>
#include <cmath>

namespace baseline {

namespace {

bool finiteVec3(const Vec3& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

ContactKinematicsResult buildContactResult(const Vec3& reference_a,
                                           const Vec3& reference_b,
                                           double phi_a_at_b,
                                           const Vec3& grad_a_at_b,
                                           bool a_in_narrow_band,
                                           double narrow_band_world_a,
                                           double voxel_size_a,
                                           double phi_b_at_a,
                                           const Vec3& grad_b_at_a,
                                           bool b_in_narrow_band,
                                           double narrow_band_world_b,
                                           double voxel_size_b,
                                           std::string object_a,
                                           std::string object_b,
                                           std::string method) {
  const Vec3 fallback_normal = normalized(reference_b - reference_a, {1.0, 0.0, 0.0});
  const Vec3 outward_a = normalized(grad_a_at_b, fallback_normal);
  const Vec3 outward_b = normalized(grad_b_at_a, -fallback_normal);
  const Vec3 normal_seed = outward_a - outward_b;
  const bool used_fallback_normal = norm(normal_seed) <= 1e-8;
  const Vec3 normal = normalized(normal_seed, fallback_normal);

  const ContactFrame frame = makeContactFrame(normal);
  const Vec3 point_on_a = reference_b - outward_a * phi_a_at_b;
  const Vec3 point_on_b = reference_a - outward_b * phi_b_at_a;
  const double signed_gap = dot(point_on_b - point_on_a, normal);
  const double gradient_alignment = dot(outward_a, -outward_b);
  const double tangent_orthogonality = contactFrameOrthogonalityResidual(frame);

  std::uint32_t valid_flags = 0;
  valid_flags |= ContactValidNormal;
  if (frame.isOrthonormal()) {
    valid_flags |= ContactValidTangents;
  }
  if (a_in_narrow_band) {
    valid_flags |= ContactQueryAInNarrowBand;
  }
  if (b_in_narrow_band) {
    valid_flags |= ContactQueryBInNarrowBand;
  }
  if (gradient_alignment > 0.7) {
    valid_flags |= ContactGradientsConsistent;
  }
  if (used_fallback_normal) {
    valid_flags |= ContactUsedFallbackNormal;
  }
  if (finiteVec3(point_on_a) && finiteVec3(point_on_b) && std::isfinite(signed_gap)) {
    valid_flags |= ContactSupportPointsValid;
  }

  const double band_margin_a = std::abs(std::abs(phi_a_at_b) - narrow_band_world_a);
  const double band_margin_b = std::abs(std::abs(phi_b_at_a) - narrow_band_world_b);
  const double band_boundary_tolerance = std::max({voxel_size_a, voxel_size_b, 1e-6});
  if (band_margin_a <= band_boundary_tolerance || band_margin_b <= band_boundary_tolerance) {
    valid_flags |= ContactNearNarrowBandBoundary;
  }

  ContactKinematicsResult result;
  result.signed_gap = signed_gap;
  result.contact_point = (point_on_a + point_on_b) * 0.5;
  result.normal = frame.normal;
  result.tangent1 = frame.tangent_u;
  result.tangent2 = frame.tangent_v;
  result.point_on_a = point_on_a;
  result.point_on_b = point_on_b;
  result.phi_a_at_query_b = phi_a_at_b;
  result.phi_b_at_query_a = phi_b_at_a;
  result.tangent_orthogonality = tangent_orthogonality;
  result.gradient_alignment = gradient_alignment;
  result.valid_flags = valid_flags;
  result.method = std::move(method);
  result.object_a = std::move(object_a);
  result.object_b = std::move(object_b);
  return result;
}

double symmetryResidual(const ContactKinematicsResult& ab, const ContactKinematicsResult& ba) {
  return std::abs(ab.signed_gap - ba.signed_gap) + norm(ab.contact_point - ba.contact_point) +
         std::abs(dot(ab.normal, ba.normal) + 1.0);
}

}  // namespace

ContactKinematicsResult DualSdfContactCalculator::compute(const SdfProvider& a, const SdfProvider& b) const {
  const Vec3 ref_a = a.referencePoint();
  const Vec3 ref_b = b.referencePoint();
  const SdfSample sample_a = a.samplePhiGrad(ref_b);
  const SdfSample sample_b = b.samplePhiGrad(ref_a);

  ContactKinematicsResult forward = buildContactResult(ref_a,
                                                       ref_b,
                                                       sample_a.signed_distance,
                                                       sample_a.gradient,
                                                       sample_a.in_narrow_band,
                                                       a.voxelSize() * a.narrowBand(),
                                                       a.voxelSize(),
                                                       sample_b.signed_distance,
                                                       sample_b.gradient,
                                                       sample_b.in_narrow_band,
                                                       b.voxelSize() * b.narrowBand(),
                                                       b.voxelSize(),
                                                       a.name(),
                                                       b.name(),
                                                       "dual-sdf");
  const ContactKinematicsResult reverse = buildContactResult(ref_b,
                                                             ref_a,
                                                             sample_b.signed_distance,
                                                             sample_b.gradient,
                                                             sample_b.in_narrow_band,
                                                             b.voxelSize() * b.narrowBand(),
                                                             b.voxelSize(),
                                                             sample_a.signed_distance,
                                                             sample_a.gradient,
                                                             sample_a.in_narrow_band,
                                                             a.voxelSize() * a.narrowBand(),
                                                             a.voxelSize(),
                                                             b.name(),
                                                             a.name(),
                                                             "dual-sdf");
  forward.symmetry_residual = symmetryResidual(forward, reverse);
  return forward;
}

ContactKinematicsResult DualSdfContactCalculator::compute(const SdfProvider& a, const ReferenceGeometry& b) const {
  const Vec3 ref_a = a.referencePoint();
  const Vec3 ref_b = b.center;
  const SdfSample sample_a = a.samplePhiGrad(ref_b);
  const double phi_b = b.signedDistance(ref_a);
  const Vec3 grad_b = b.normalAt(ref_a);

  ContactKinematicsResult forward = buildContactResult(ref_a,
                                                       ref_b,
                                                       sample_a.signed_distance,
                                                       sample_a.gradient,
                                                       sample_a.in_narrow_band,
                                                       a.voxelSize() * a.narrowBand(),
                                                       a.voxelSize(),
                                                       phi_b,
                                                       grad_b,
                                                       true,
                                                       std::max(1.0, b.boundingRadius()),
                                                       a.voxelSize(),
                                                       a.name(),
                                                       b.name,
                                                       "sdf-plus-reference-geometry");
  const ContactKinematicsResult reverse = buildContactResult(ref_b,
                                                             ref_a,
                                                             phi_b,
                                                             grad_b,
                                                             true,
                                                             std::max(1.0, b.boundingRadius()),
                                                             a.voxelSize(),
                                                             sample_a.signed_distance,
                                                             sample_a.gradient,
                                                             sample_a.in_narrow_band,
                                                             a.voxelSize() * a.narrowBand(),
                                                             a.voxelSize(),
                                                             b.name,
                                                             a.name(),
                                                             "reference-plus-sdf");
  forward.symmetry_residual = symmetryResidual(forward, reverse);
  return forward;
}

}  // namespace baseline
