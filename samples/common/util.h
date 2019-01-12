#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace util {
std::vector<char> ReadFile(const std::string& filename);
}  // namespace util