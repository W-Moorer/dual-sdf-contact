#include "baseline/contact/candidate_pair_manager.h"

namespace baseline {

std::vector<CandidatePair> CandidatePairManager::buildPairs(const std::vector<CandidateObject>& objects,
                                                            double margin) const {
  std::vector<CandidatePair> pairs;
  for (std::size_t i = 0; i < objects.size(); ++i) {
    for (std::size_t j = i + 1; j < objects.size(); ++j) {
      const double center_distance = norm(objects[j].reference_point - objects[i].reference_point);
      const double broadphase_gap =
          center_distance - (objects[i].broadphase_radius + objects[j].broadphase_radius);
      if (broadphase_gap <= margin) {
        pairs.push_back({objects[i].id, objects[j].id, center_distance, broadphase_gap});
      }
    }
  }
  return pairs;
}

}  // namespace baseline
