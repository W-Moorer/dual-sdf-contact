#include <cmath>
#include <iostream>

#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"

int main() {
  using namespace baseline;

  const NanoVdbSdfProvider a = NanoVdbSdfProvider::makeSphere("a", {0.0, 0.0, 0.0}, 1.0, 0.05, 2.0);
  const NanoVdbSdfProvider b = NanoVdbSdfProvider::makeSphere("b", {1.6, 0.0, 0.0}, 0.7, 0.05, 2.0);

  const DualSdfContactCalculator calculator;
  const ContactGeometry ab = calculator.compute(a, b);
  const ContactGeometry ba = calculator.compute(b, a);

  if (std::abs(ab.signed_gap - ba.signed_gap) > 1e-8) {
    std::cerr << "Signed gap lost symmetry.\n";
    return 1;
  }
  if (std::abs(dot(ab.normal, ba.normal) + 1.0) > 1e-6) {
    std::cerr << "Normals are not opposite.\n";
    return 1;
  }
  if (norm(ab.contact_point - ba.contact_point) > 1e-8) {
    std::cerr << "Contact point lost symmetry.\n";
    return 1;
  }
  return 0;
}
