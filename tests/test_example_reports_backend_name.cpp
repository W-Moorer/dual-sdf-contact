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

}  // namespace

int main() {
  const auto executable = findExecutable("ex01_nanovdb_hello");
  const std::string command = "\"" + executable.string() + "\"";
  if (std::system(command.c_str()) != 0) {
    std::cerr << "Failed to run ex01_nanovdb_hello.\n";
    return 1;
  }

  const auto summary_path = baseline::sourceDir() / "outputs" / "ex01_nanovdb_hello" / "summary.json";
  std::ifstream stream(summary_path, std::ios::binary);
  if (!stream) {
    std::cerr << "Could not read summary file.\n";
    return 1;
  }

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const std::string content = buffer.str();
  if (content.find("\"selected_sdf_backend\"") == std::string::npos ||
      content.find("\"provider_backend_name\"") == std::string::npos) {
    std::cerr << "Summary file does not report backend names.\n";
    return 1;
  }
  return 0;
}
