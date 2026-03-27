#include "baseline/core/io_utils.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "baseline/core/build_config.h"

namespace baseline {

namespace {

std::string readEnvironmentVariable(const char* name) {
#ifdef _WIN32
  char* buffer = nullptr;
  std::size_t size = 0;
  if (_dupenv_s(&buffer, &size, name) != 0 || buffer == nullptr) {
    return {};
  }
  std::string value(buffer);
  std::free(buffer);
  return value;
#else
  const char* value = std::getenv(name);
  return value ? std::string(value) : std::string{};
#endif
}

}  // namespace

std::filesystem::path sourceDir() { return std::filesystem::path(BASELINE_SOURCE_DIR); }

std::filesystem::path binaryDir() { return std::filesystem::path(BASELINE_BINARY_DIR); }

std::filesystem::path outputsDir() {
  const std::string env_output_root = readEnvironmentVariable("BASELINE_OUTPUT_ROOT");
  if (!env_output_root.empty()) {
    return std::filesystem::path(env_output_root);
  }
  return sourceDir() / "outputs";
}

std::filesystem::path ensureExampleOutputDir(std::string_view example_name) {
  const auto directory = outputsDir() / std::filesystem::path(example_name);
  std::filesystem::create_directories(directory);
  return directory;
}

void writeTextFile(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("Failed to open file for writing: " + path.string());
  }
  stream << content;
}

std::string quoteJson(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size() + 2);
  escaped.push_back('"');
  for (const char ch : text) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  escaped.push_back('"');
  return escaped;
}

std::string formatDouble(double value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

std::string boolToString(bool value) { return value ? "true" : "false"; }

}  // namespace baseline
