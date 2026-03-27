#pragma once

#include <string>
#include <vector>

#include "baseline/core/math.h"

namespace baseline {

struct CandidateObject {
  std::string id;
  Vec3 reference_point{0.0, 0.0, 0.0};
  double broadphase_radius{0.0};
};

struct CandidatePair {
  std::string id_a;
  std::string id_b;
  double center_distance{0.0};
  double broadphase_gap{0.0};
};

class CandidatePairManager {
 public:
  std::vector<CandidatePair> buildPairs(const std::vector<CandidateObject>& objects,
                                        double margin = 0.0) const;
};

}  // namespace baseline
