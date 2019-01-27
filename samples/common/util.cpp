#include "util.h"

#include <ktx.h>
#include <gli/gli.hpp>

namespace util {
std::vector<char> ReadFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file " + filename);
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

void LoadCubemap(const char* data, size_t size, RenderAPI::Device device,
                 RenderAPI::CommandPool pool, RenderAPI::Image& image,
                 RenderAPI::ImageView& view) {
  gli::texture_cube texCube(gli::load(data, size));
  assert(!texCube.empty());

  RenderAPI::ImageCreateInfo image_info;
  image_info.type = RenderAPI::TextureType::Texture2D;
  image_info.format = static_cast<RenderAPI::TextureFormat>(texCube.format());
  image_info.extent =
      RenderAPI::Extent3D(texCube.extent().x, texCube.extent().y, 1);
  image_info.mips = texCube.levels();
  image_info.array_layers = 6;
  image_info.flags = RenderAPI::ImageCreateFlagBits::kCubeCompatibleBit;

  image = RenderAPI::CreateImage(device, image_info);

  std::vector<RenderAPI::BufferImageCopy> copy_regions(6 * image_info.mips);

  uint32_t i = 0;
  size_t offset = 0;
  for (int face = 0; face < 6; ++face) {
    for (int mip = 0; mip < image_info.mips; ++mip) {
      RenderAPI::BufferImageCopy& region = copy_regions[i];

      region.image_subsurface.aspect_mask =
          RenderAPI::ImageAspectFlagBits::kColorBit;
      region.image_subsurface.mip_level = mip;
      region.image_subsurface.base_array_layer = face;
      region.image_extent.width = texCube[face][mip].extent().x;
      region.image_extent.height = texCube[face][mip].extent().y;
      region.image_extent.depth = 1;
      region.buffer_offset = offset;

      // Increase offset into staging buffer for next level / face.
      offset += texCube[face][mip].size();

      ++i;
    }
  }

  RenderAPI::StageCopyDataToImage(pool, image, texCube.data(), texCube.size(),
                                  copy_regions.size(), copy_regions.data());

  RenderAPI::ImageViewCreateInfo image_view_info;
  image_view_info.image = image;
  image_view_info.type = RenderAPI::ImageViewType::Cube;
  image_view_info.format = image_info.format;
  image_view_info.subresource_range.layer_count = 6;
  image_view_info.subresource_range.level_count = image_info.mips;
  image_view_info.subresource_range.aspect_mask =
      RenderAPI::ImageAspectFlagBits::kColorBit;
  view = RenderAPI::CreateImageView(device, image_view_info);
}

bool LoadTextureFromKtx(const std::string& filename, RenderAPI::Device device,
                        RenderAPI::CommandPool pool, RenderAPI::Image& image,
                        RenderAPI::ImageView& view) {
  std::vector<char> file = ReadFile(filename);
  if (file.empty()) {
    return false;
  }

  const ktx::KtxHeader* header =
      reinterpret_cast<const ktx::KtxHeader*>(file.data());

  if (strncmp(reinterpret_cast<const char*>(header->identifier),
              reinterpret_cast<const char*>(ktx::kKtxIdentifier), 12) != 0) {
    throw std::runtime_error(filename + " doesn't have Ktx identifier!");
    return false;
  }

  if (header->number_of_faces == 6) {
    // Handle cubemap.
    LoadCubemap(file.data(), file.size(), device, pool, image, view);
  } else {
    // Handle non cubemap.
  }

  return true;
}
}  // namespace util