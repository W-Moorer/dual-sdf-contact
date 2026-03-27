#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "baseline/contact/candidate_pair_manager.h"
#include "baseline/contact/reference_geometry.h"
#include "example_utils.h"

int main() {
  using namespace baseline;

  const std::string example_name = "ex02_hppfcl_distance";
  const auto output_dir = apps::outputDir(example_name);

  const auto engine = makeDefaultReferenceGeometryQueryEngine();
  const ReferenceGeometry sphere = ReferenceGeometry::makeSphere("sphere", {0.0, 0.0, 0.0}, 0.75);
  const ReferenceGeometry box_far = ReferenceGeometry::makeBox("box_far", {1.8, 0.2, 0.0}, {0.4, 0.6, 0.5});
  const ReferenceGeometry box_near = ReferenceGeometry::makeBox("box_near", {0.9, 0.0, 0.0}, {0.4, 0.6, 0.5});

  const DistanceQueryResult far_result = engine->distance(sphere, box_far);
  const DistanceQueryResult near_result = engine->distance(sphere, box_near);

  CandidatePairManager pair_manager;
  const auto pairs = pair_manager.buildPairs(
      {
          {sphere.name, sphere.center, sphere.boundingRadius()},
          {box_far.name, box_far.center, box_far.boundingRadius()},
          {box_near.name, box_near.center, box_near.boundingRadius()},
      },
      0.25);

  apps::writeCsv(
      output_dir / "distance_cases.csv",
      {"case", "backend", "collision", "distance", "normal_x", "normal_y", "normal_z"},
      {
          {"separated",
           far_result.backend_name,
           boolToString(far_result.collision),
           formatDouble(far_result.distance),
           formatDouble(far_result.normal.x),
           formatDouble(far_result.normal.y),
           formatDouble(far_result.normal.z)},
          {"penetrating",
           near_result.backend_name,
           boolToString(near_result.collision),
           formatDouble(near_result.distance),
           formatDouble(near_result.normal.x),
           formatDouble(near_result.normal.y),
           formatDouble(near_result.normal.z)},
      });

  std::ostringstream summary;
  summary << "{\n"
          << "  \"example\": " << quoteJson(example_name) << ",\n"
          << "  \"engine\": " << quoteJson(engine->name()) << ",\n"
          << "  \"candidate_pair_count\": " << pairs.size() << ",\n"
          << "  \"closest_point_separated_a\": " << apps::vec3Json(far_result.closest_point_a) << ",\n"
          << "  \"closest_point_separated_b\": " << apps::vec3Json(far_result.closest_point_b) << "\n"
          << "}\n";
  writeTextFile(output_dir / "summary.json", summary.str());

  std::cout << example_name << ": engine=" << engine->name()
            << ", separated_distance=" << far_result.distance
            << ", penetrating_collision=" << boolToString(near_result.collision) << "\n";
  return 0;
}
