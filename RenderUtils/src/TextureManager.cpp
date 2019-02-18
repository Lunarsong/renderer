#include <RenderUtils/TextureManager.h>

#include <cassert>

namespace RenderUtils {
TextureManager::TextureManager(RenderAPI::Device device) : device_(device) {
  pool_ = RenderAPI::CreateCommandPool(
      device_, RenderAPI::CommandPoolCreateFlag::kTransient |
                   RenderAPI::CommandPoolCreateFlag::kResetCommand);
}

TextureManager::~TextureManager() {
  for (auto& it : cache_) {
    RenderAPI::DestroyImageView(device_, it.first);
    RenderAPI::DestroyImage(it.second.image);
  }
  RenderAPI::DestroyCommandPool(pool_);
}

RenderAPI::ImageView TextureManager::Create(
    TextureCreateInfo info, const void* data, uint64_t size,
    uint32_t num_regions, const RenderAPI::BufferImageCopy* regions) {
  // Create the image.
  RenderAPI::ImageCreateInfo image_info;
  image_info.format = info.format;
  image_info.extent = info.extent;
  image_info.mips = info.mips;
  image_info.array_layers = 6;
  image_info.flags = info.flags;

  if (info.type == RenderAPI::ImageViewType::Cube) {
    image_info.type = RenderAPI::TextureType::Texture2D;
    image_info.flags |= RenderAPI::ImageCreateFlagBits::kCubeCompatibleBit;
  } else if (info.type == RenderAPI::ImageViewType::Texture1D) {
    image_info.type = RenderAPI::TextureType::Texture1D;
  } else if (info.type == RenderAPI::ImageViewType::Texture2D) {
    image_info.type = RenderAPI::TextureType::Texture2D;
  } else if (info.type == RenderAPI::ImageViewType::Texture3D) {
    image_info.type = RenderAPI::TextureType::Texture3D;
  }
  RenderAPI::Image image = RenderAPI::CreateImage(device_, image_info);

  // Copy data.
  if (data) {
    assert(size);
    if (num_regions == 0) {
      RenderAPI::BufferImageCopy region(0, 0, 0, info.extent.width,
                                        info.extent.height, info.extent.depth);
      RenderAPI::StageCopyDataToImage(pool_, image, data, size, 1, &region);
    } else {
      RenderAPI::StageCopyDataToImage(pool_, image, data, size, num_regions,
                                      regions);
    }
  }

  // Create the image view.
  RenderAPI::ImageViewCreateInfo image_view_info;
  image_view_info.image = image;
  image_view_info.type = info.type;
  image_view_info.format = image_info.format;
  image_view_info.subresource_range.layer_count = info.array_layers;
  image_view_info.subresource_range.level_count = info.mips;
  image_view_info.subresource_range.aspect_mask =
      RenderAPI::ImageAspectFlagBits::kColorBit;
  RenderAPI::ImageView view =
      RenderAPI::CreateImageView(device_, image_view_info);

  auto& cached = cache_[view];
  cached.image = image;
  cached.info = std::move(info);
  cached.refs = 1;

  return view;
}

void TextureManager::AddRef(RenderAPI::ImageView texture) {
  auto& it = cache_.find(texture);
  assert(it != cache_.end());
  ++it->second.refs;
}
void TextureManager::Release(RenderAPI::ImageView texture) {
  auto& it = cache_.find(texture);
  assert(it != cache_.end());
  if (--it->second.refs == 0) {
    RenderAPI::DestroyImageView(device_, texture);
    RenderAPI::DestroyImage(it->second.image);
    cache_.erase(it);
  }
}
}  // namespace RenderUtils