#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace baseline {

std::filesystem::path sourceDir();
std::filesystem::path binaryDir();
std::filesystem::path outputsDir();
std::filesystem::path ensureExampleOutputDir(std::string_view example_name);

void writeTextFile(const std::filesystem::path& path, const std::string& content);
std::string quoteJson(std::string_view text);
std::string formatDouble(double value, int precision = 6);
std::string boolToString(bool value);

}  // namespace baseline
