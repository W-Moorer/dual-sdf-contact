#include <iostream>
#include <vector>

#include "baseline/runtime/backend_factory.h"
#include "example_utils.h"

int main(int argc, char** argv) {
  using namespace baseline;

  try {
    const std::string example_name = "ex01_nanovdb_hello";
    const auto options = apps::parseBackendOptions(argc, argv);
    if (options.help_requested) {
      apps::printUsage(example_name);
      return 0;
    }

    const auto availability = queryBackendAvailability();
    const auto sdf_backend = resolveSdfBackend(options.sdf_backend);
    const auto output_dir = apps::outputDir(example_name);

    auto provider = makeSdfProvider(
        sdf_backend,
        {
            "sphere_level_set",
            ReferenceGeometry::makeSphere("sphere_level_set", {0.0, 0.0, 0.0}, 1.0),
            0.1,
            3.0,
        });

    const std::vector<Vec3> samples = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {1.5, 0.0, 0.0},
        {0.4, 0.3, 0.2},
    };

    std::vector<std::vector<std::string>> rows;
    for (const auto& point : samples) {
      const SdfSample sample = provider->sample(point);
      rows.push_back({
          formatDouble(point.x),
          formatDouble(point.y),
          formatDouble(point.z),
          formatDouble(sample.signed_distance),
          formatDouble(sample.gradient.x),
          formatDouble(sample.gradient.y),
          formatDouble(sample.gradient.z),
      });
    }

    apps::writeCsv(
        output_dir / "samples.csv",
        {"x", "y", "z", "signed_distance", "grad_x", "grad_y", "grad_z"},
        rows);

    std::ostringstream summary;
    summary << "{\n"
            << "  \"example\": " << quoteJson(example_name) << ",\n"
            << "  \"selected_sdf_backend\": " << quoteJson(sdf_backend.selected_name) << ",\n"
            << "  \"provider_backend_name\": " << quoteJson(provider->backendName()) << ",\n"
            << "  \"backend_note\": " << quoteJson(sdf_backend.note) << ",\n"
            << "  \"reference_point\": " << apps::vec3Json(provider->referencePoint()) << ",\n"
            << "  \"real_backends_available\": " << apps::backendListJson(availability) << ",\n"
            << "  \"build_summary\": " << quoteJson(buildConfigurationSummary()) << ",\n"
            << "  \"output_csv\": " << quoteJson((output_dir / "samples.csv").string()) << "\n"
            << "}\n";
    writeTextFile(output_dir / "summary.json", summary.str());

    std::cout << example_name << ": sdf_backend=" << sdf_backend.selected_name
              << ", provider=" << provider->backendName()
              << ", note=" << sdf_backend.note << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex01_nanovdb_hello failed: " << error.what() << "\n";
    return 1;
  }
}
