#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "baseline/core/io_utils.h"

namespace {

std::filesystem::path findExecutable(const std::string& name) {
  const auto root = baseline::binaryDir();
#ifdef _WIN32
  const std::string executable_name = name + ".exe";
#else
  const std::string executable_name = name;
#endif
  for (const auto& candidate : {root / "bin" / "Release" / executable_name, root / "bin" / executable_name}) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  throw std::runtime_error("Could not locate example executable: " + name);
}

}  // namespace

int main() {
  using namespace baseline;

  const auto executable = findExecutable("ex05_compare_backends");
  const auto config_path = sourceDir() / "configs" / "benchmarks" / "mesh_smoke.json";
  const auto output_root = sourceDir() / "outputs" / "test_reference_quality_labels_present";
  const std::string command = "\"" + executable.string() + "\" --config \"" + config_path.string() +
                              "\" --output-dir \"" + output_root.string() + "\"";
  if (std::system(command.c_str()) != 0) {
    std::cerr << "Failed to run ex05_compare_backends.\n";
    return 1;
  }

  const auto samples_csv = output_root / "mesh_smoke" / "samples.csv";
  const auto summary_csv = output_root / "mesh_smoke" / "summary.csv";
  const std::string samples = readTextFile(samples_csv);
  const std::string summary = readTextFile(summary_csv);
  if (samples.find("reference_quality") == std::string::npos || samples.find("reference_mode") == std::string::npos ||
      samples.find("reference_source_label") == std::string::npos || summary.find("reference_quality") == std::string::npos) {
    std::cerr << "Reference quality labels are missing from benchmark outputs.\n";
    return 1;
  }
  return 0;
}
