#include <iostream>

#include "baseline/core/math.h"

int main() {
  using namespace baseline;

  const ContactFrame frame = makeContactFrame(normalized({0.3, 1.0, -0.2}));
  if (!frame.isOrthonormal()) {
    std::cerr << "Contact frame is not orthonormal.\n";
    return 1;
  }
  return 0;
}
