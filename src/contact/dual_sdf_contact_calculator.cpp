#include "baseline/contact/dual_sdf_contact_calculator.h"

namespace baseline {

namespace {

ContactGeometry finalizeGeometry(const Vec3& reference_a,
                                 const Vec3& reference_b,
                                 double phi_a_at_b,
                                 const Vec3& grad_a_at_b,
                                 double phi_b_at_a,
                                 const Vec3& grad_b_at_a,
                                 std::string object_a,
                                 std::string object_b,
                                 std::string method) {
  const Vec3 fallback_normal = normalized(reference_b - reference_a, {1.0, 0.0, 0.0});
  const Vec3 outward_a = normalized(grad_a_at_b, fallback_normal);
  const Vec3 outward_b = normalized(grad_b_at_a, -fallback_normal);
  const Vec3 normal_ab = normalized(outward_a - outward_b, fallback_normal);
  const Vec3 support_a = reference_b - outward_a * phi_a_at_b;
  const Vec3 support_b = reference_a - outward_b * phi_b_at_a;
  const double signed_gap = dot(support_b - support_a, normal_ab);
  return {
      signed_gap,
      makeContactFrame(normal_ab),
      (support_a + support_b) * 0.5,
      support_a,
      support_b,
      std::move(method),
      std::move(object_a),
      std::move(object_b),
  };
}

}  // namespace

ContactGeometry DualSdfContactCalculator::compute(const SdfProvider& a, const SdfProvider& b) const {
  const Vec3 ref_a = a.referencePoint();
  const Vec3 ref_b = b.referencePoint();
  const SdfSample sample_a = a.sample(ref_b);
  const SdfSample sample_b = b.sample(ref_a);
  return finalizeGeometry(ref_a,
                          ref_b,
                          sample_a.signed_distance,
                          sample_a.gradient,
                          sample_b.signed_distance,
                          sample_b.gradient,
                          a.name(),
                          b.name(),
                          "dual-sdf");
}

ContactGeometry DualSdfContactCalculator::compute(const SdfProvider& a, const ReferenceGeometry& b) const {
  const Vec3 ref_a = a.referencePoint();
  const Vec3 ref_b = b.center;
  const SdfSample sample_a = a.sample(ref_b);
  return finalizeGeometry(ref_a,
                          ref_b,
                          sample_a.signed_distance,
                          sample_a.gradient,
                          b.signedDistance(ref_a),
                          b.normalAt(ref_a),
                          a.name(),
                          b.name,
                          "sdf-plus-reference-geometry");
}

}  // namespace baseline
