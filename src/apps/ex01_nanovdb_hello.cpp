#include <iostream>
#include <vector>

#include "baseline/core/io_utils.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "example_utils.h"

int main() {
  using namespace baseline;

  const std::string example_name = "ex01_nanovdb_hello";
  const auto output_dir = apps::outputDir(example_name);

  const NanoVdbSdfProvider provider =
      NanoVdbSdfProvider::makeSphere("sphere_level_set", {0.0, 0.0, 0.0}, 1.0, 0.1, 3.0);

  const std::vector<Vec3> samples = {
      {0.0, 0.0, 0.0},
      {1.0, 0.0, 0.0},
      {1.5, 0.0, 0.0},
      {0.4, 0.3, 0.2},
  };

  std::vector<std::vector<std::string>> rows;
  for (const auto& point : samples) {
    const SdfSample sample = provider.sample(point);
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
          << "  \"provider\": " << quoteJson(provider.name()) << ",\n"
          << "  \"backend\": " << quoteJson(provider.backendName()) << ",\n"
          << "  \"reference_point\": " << apps::vec3Json(provider.referencePoint()) << ",\n"
          << "  \"output_csv\": " << quoteJson((output_dir / "samples.csv").string()) << "\n"
          << "}\n";
  writeTextFile(output_dir / "summary.json", summary.str());

  std::cout << example_name << ": backend=" << provider.backendName()
            << ", output=" << (output_dir / "summary.json").string() << "\n";
  return 0;
}
