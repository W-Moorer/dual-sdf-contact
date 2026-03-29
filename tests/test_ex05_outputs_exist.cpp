#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "baseline/core/io_utils.h"

namespace {

std::filesystem::path findExecutable(const std::string& name) {
  const auto root = baseline::binaryDir();
#ifdef _WIN32
  const std::string executable_name = name + ".exe";
#else
  const std::string executable_name = name;
#endif

  const std::vector<std::filesystem::path> candidates = {
      root / "bin" / "Release" / executable_name,
      root / "bin" / "Debug" / executable_name,
      root / "bin" / executable_name,
  };
  for (const auto& candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  throw std::runtime_error("Could not locate example executable: " + name);
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("Could not read file: " + path.string());
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

}  // namespace

int main() {
  const auto executable = findExecutable("ex05_compare_backends");
  const std::string command = "\"" + executable.string() + "\"";
  if (std::system(command.c_str()) != 0) {
    std::cerr << "Failed to run ex05_compare_backends.\n";
    return 1;
  }

  const auto output_root = baseline::sourceDir() / "outputs" / "ex05_compare_backends";
  const auto summary_csv = output_root / "summary.csv";
  const auto summary_json = output_root / "summary.json";
  const auto report_md = output_root / "report.md";

  if (!std::filesystem::exists(summary_csv) || !std::filesystem::exists(summary_json) ||
      !std::filesystem::exists(report_md)) {
    std::cerr << "Expected ex05 outputs were not produced.\n";
    return 1;
  }

  const std::string json_content = readFile(summary_json);
  const std::string report_content = readFile(report_md);
  if (json_content.find("\"cases\"") == std::string::npos ||
      json_content.find("\"selected_reference_backend\"") == std::string::npos ||
      report_content.find("Case Summary") == std::string::npos) {
    std::cerr << "ex05 outputs are missing expected content.\n";
    return 1;
  }
  return 0;
}
