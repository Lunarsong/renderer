#pragma once

namespace MetallicRoughnessBits {
constexpr uint32_t kHasBaseColorTexture = 1 << 0;
constexpr uint32_t kHasMetallicRoughnessTexture = 1 << 1;
constexpr uint32_t kHasOcclusionTexture = 1 << 2;
constexpr uint32_t kHasNormalsTexture = 1 << 3;
constexpr uint32_t kHashEmissiveTexture = 1 << 4;
}  // namespace MetallicRoughnessBits