#include <iostream>
#include <sstream>
#include <vector>

#include "baseline/contact/candidate_pair_manager.h"
#include "baseline/runtime/backend_factory.h"
#include "example_utils.h"

int main(int argc, char** argv) {
  using namespace baseline;

  try {
    const std::string example_name = "ex02_hppfcl_distance";
    const auto options = apps::parseBackendOptions(argc, argv);
    if (options.help_requested) {
      apps::printUsage(example_name);
      return 0;
    }

    const auto availability = queryBackendAvailability();
    const auto reference_backend = resolveReferenceBackend(options.reference_backend);
    const auto output_dir = apps::outputDir(example_name);

    const auto engine = makeReferenceBackend(reference_backend);
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
            << "  \"selected_reference_backend\": " << quoteJson(reference_backend.selected_name) << ",\n"
            << "  \"engine_name\": " << quoteJson(engine->name()) << ",\n"
            << "  \"backend_note\": " << quoteJson(reference_backend.note) << ",\n"
            << "  \"candidate_pair_count\": " << pairs.size() << ",\n"
            << "  \"closest_point_separated_a\": " << apps::vec3Json(far_result.closest_point_a) << ",\n"
            << "  \"closest_point_separated_b\": " << apps::vec3Json(far_result.closest_point_b) << ",\n"
            << "  \"real_backends_available\": " << apps::backendListJson(availability) << "\n"
            << "}\n";
    writeTextFile(output_dir / "summary.json", summary.str());

    std::cout << example_name << ": reference_backend=" << reference_backend.selected_name
              << ", engine=" << engine->name()
              << ", note=" << reference_backend.note << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex02_hppfcl_distance failed: " << error.what() << "\n";
    return 1;
  }
}
