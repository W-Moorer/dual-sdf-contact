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
    const ReferenceGeometry sphere_b = ReferenceGeometry::makeSphere("sphere_b", {1.7, 0.0, 0.0}, 0.55);
    const ReferenceGeometry box_far = ReferenceGeometry::makeBox("box_far", {1.8, 0.2, 0.0}, {0.4, 0.6, 0.5});
    const ReferenceGeometry box_near = ReferenceGeometry::makeBox("box_near", {0.9, 0.0, 0.0}, {0.4, 0.6, 0.5});

    const DistanceQueryResult sphere_sphere = engine->distance(sphere, sphere_b);
    const DistanceQueryResult far_result = engine->distance(sphere, box_far);
    const DistanceQueryResult near_result = engine->distance(sphere, box_near);

    CandidatePairManager pair_manager;
    const auto pairs = pair_manager.buildPairs(
        {
            {sphere.name, sphere.center, sphere.boundingRadius()},
            {sphere_b.name, sphere_b.center, sphere_b.boundingRadius()},
            {box_far.name, box_far.center, box_far.boundingRadius()},
            {box_near.name, box_near.center, box_near.boundingRadius()},
        },
        0.25);

    apps::writeCsv(
        output_dir / "distance_cases.csv",
        {"case", "backend", "collision", "distance", "signed_distance", "normal_x", "normal_y", "normal_z",
         "point_a_x", "point_a_y", "point_a_z", "point_b_x", "point_b_y", "point_b_z"},
        {
            {"sphere_sphere",
             sphere_sphere.backend_name,
             boolToString(sphere_sphere.collision),
             formatDouble(sphere_sphere.distance),
             formatDouble(sphere_sphere.signed_distance),
             formatDouble(sphere_sphere.normal.x),
             formatDouble(sphere_sphere.normal.y),
             formatDouble(sphere_sphere.normal.z),
             formatDouble(sphere_sphere.closest_point_a.x),
             formatDouble(sphere_sphere.closest_point_a.y),
             formatDouble(sphere_sphere.closest_point_a.z),
             formatDouble(sphere_sphere.closest_point_b.x),
             formatDouble(sphere_sphere.closest_point_b.y),
             formatDouble(sphere_sphere.closest_point_b.z)},
            {"sphere_box_separated",
             far_result.backend_name,
             boolToString(far_result.collision),
             formatDouble(far_result.distance),
             formatDouble(far_result.signed_distance),
             formatDouble(far_result.normal.x),
             formatDouble(far_result.normal.y),
             formatDouble(far_result.normal.z),
             formatDouble(far_result.closest_point_a.x),
             formatDouble(far_result.closest_point_a.y),
             formatDouble(far_result.closest_point_a.z),
             formatDouble(far_result.closest_point_b.x),
             formatDouble(far_result.closest_point_b.y),
             formatDouble(far_result.closest_point_b.z)},
            {"sphere_box_penetrating",
             near_result.backend_name,
             boolToString(near_result.collision),
             formatDouble(near_result.distance),
             formatDouble(near_result.signed_distance),
             formatDouble(near_result.normal.x),
             formatDouble(near_result.normal.y),
             formatDouble(near_result.normal.z),
             formatDouble(near_result.closest_point_a.x),
             formatDouble(near_result.closest_point_a.y),
             formatDouble(near_result.closest_point_a.z),
             formatDouble(near_result.closest_point_b.x),
             formatDouble(near_result.closest_point_b.y),
             formatDouble(near_result.closest_point_b.z)},
        });

    std::ostringstream summary;
    summary << "{\n"
            << "  \"example\": " << quoteJson(example_name) << ",\n"
            << "  \"selected_reference_backend\": " << quoteJson(reference_backend.selected_name) << ",\n"
            << "  \"engine_name\": " << quoteJson(engine->name()) << ",\n"
            << "  \"backend_note\": " << quoteJson(reference_backend.note) << ",\n"
            << "  \"candidate_pair_count\": " << pairs.size() << ",\n"
            << "  \"sphere_box_separated\": {\n"
            << "    \"collision\": " << boolToString(far_result.collision) << ",\n"
            << "    \"distance\": " << formatDouble(far_result.distance) << ",\n"
            << "    \"signed_distance\": " << formatDouble(far_result.signed_distance) << ",\n"
            << "    \"normal\": " << apps::vec3Json(far_result.normal) << ",\n"
            << "    \"closest_point_a\": " << apps::vec3Json(far_result.closest_point_a) << ",\n"
            << "    \"closest_point_b\": " << apps::vec3Json(far_result.closest_point_b) << "\n"
            << "  },\n"
            << "  \"sphere_box_penetrating\": {\n"
            << "    \"collision\": " << boolToString(near_result.collision) << ",\n"
            << "    \"distance\": " << formatDouble(near_result.distance) << ",\n"
            << "    \"signed_distance\": " << formatDouble(near_result.signed_distance) << ",\n"
            << "    \"normal\": " << apps::vec3Json(near_result.normal) << ",\n"
            << "    \"closest_point_a\": " << apps::vec3Json(near_result.closest_point_a) << ",\n"
            << "    \"closest_point_b\": " << apps::vec3Json(near_result.closest_point_b) << "\n"
            << "  },\n"
            << "  \"fallback_execution\": " << boolToString(reference_backend.uses_fallback_execution) << ",\n"
            << "  \"real_backends_available\": " << apps::backendListJson(availability) << "\n"
            << "}\n";
    writeTextFile(output_dir / "summary.json", summary.str());

    std::cout << example_name << ": reference_backend=" << reference_backend.selected_name
              << ", engine=" << engine->name()
              << ", signed_distance=" << far_result.signed_distance << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex02_hppfcl_distance failed: " << error.what() << "\n";
    return 1;
  }
}
