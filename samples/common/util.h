#pragma once

#include <RenderAPI/RenderAPI.h>
#include <fstream>
#include <string>
#include <vector>

namespace util {
std::vector<char> ReadFile(const std::string& filename);
bool LoadTexture(const std::string& filename, RenderAPI::Device device,
                 RenderAPI::CommandPool pool, RenderAPI::Image& image,
                 RenderAPI::ImageView& view);
}  // namespace util