#pragma once

#include <RenderAPI/RenderAPI.h>
#include <unordered_map>

namespace RenderUtils {

struct TextureCreateInfo {
  RenderAPI::ImageViewType type = RenderAPI::ImageViewType::Texture2D;
  RenderAPI::TextureFormat format;
  RenderAPI::Extent3D extent;
  uint32_t mips = 1;
  uint32_t array_layers = 1;
  RenderAPI::ImageCreateFlags flags = 0;
  RenderAPI::Swizzle swizzle;
};

class TextureManager {
 public:
  TextureManager(RenderAPI::Device device);
  ~TextureManager();

  RenderAPI::ImageView Create(
      TextureCreateInfo info, const void* data = nullptr, uint64_t size = 0,
      uint32_t num_regions = 0,
      const RenderAPI::BufferImageCopy* regions = nullptr);

  void AddRef(RenderAPI::ImageView texture);
  void Release(RenderAPI::ImageView texture);

  static TextureManager* Get();

 private:
  RenderAPI::Device device_;
  RenderAPI::CommandPool pool_;

  struct CachedTexture {
    uint32_t refs = 1;
    RenderAPI::Image image;
    TextureCreateInfo info;
  };
  std::unordered_map<RenderAPI::ImageView, CachedTexture> cache_;
};

}  // namespace RenderUtils