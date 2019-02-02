#include "render_graph_cache.h"

bool operator==(const RenderGraphTextureDesc& a,
                const RenderGraphTextureDesc& b) {
  return (a.format == b.format && a.width == b.width && a.height == b.height &&
          a.load_op == b.load_op && a.layout == b.layout);
}

bool operator==(const RenderGraphFramebufferDesc& a,
                const RenderGraphFramebufferDesc& b) {
  return a.textures == b.textures;
}

void RenderGraphCache::PrepareBufferedResources(uint32_t size) {
  if (size > buffered_resources_.size()) {
    buffered_resources_.resize(size);
  }
}

void RenderGraphCache::SetRenderObjects(RenderAPI::Device device,
                                        RenderAPI::CommandPool pool) {
  device_ = device;
  pool_ = pool;
}

void RenderGraphCache::Reset() {
  resources_index_ = (resources_index_ + 1) % buffered_resources_.size();
  buffered_resources_[resources_index_].cmd_index = 0;
  buffered_resources_[resources_index_].semaphore_index = 0;

  for (auto& it : transient_buffers_) {
    ++it.frames_since_use;
  }
  for (auto& it : transient_textures_) {
    ++it.frames_since_use;
  }
}

RenderAPI::CommandBuffer RenderGraphCache::AllocateCommand() {
  if (buffered_resources_[resources_index_].cmd_index ==
      buffered_resources_[resources_index_].cmds.size()) {
    buffered_resources_[resources_index_].cmds.emplace_back(
        RenderAPI::CreateCommandBuffer(pool_));
  }
  return buffered_resources_[resources_index_]
      .cmds[buffered_resources_[resources_index_].cmd_index++];
}

RenderAPI::Semaphore RenderGraphCache::AllocateSemaphore() {
  if (buffered_resources_[resources_index_].semaphore_index ==
      buffered_resources_[resources_index_].semaphores.size()) {
    buffered_resources_[resources_index_].semaphores.emplace_back(
        RenderAPI::CreateSemaphore(device_));
  }
  return buffered_resources_[resources_index_]
      .semaphores[buffered_resources_[resources_index_].semaphore_index++];
}

void RenderGraphCache::Destroy() {
  for (auto& it : buffered_resources_) {
    for (auto semaphore : it.semaphores) {
      RenderAPI::DestroySemaphore(semaphore);
    }
    it.semaphores.clear();
  }

  for (auto& it : transient_buffers_) {
    RenderAPI::DestroyFramebuffer(it.resources.framebuffer);
    RenderAPI::DestroyRenderPass(it.resources.pass);
  }

  for (auto& it : transient_textures_) {
    RenderAPI::DestroyImageView(device_, it.image_view);
    RenderAPI::DestroyImage(it.image);
  }
}

RenderAPI::ImageAspectFlags GetAspectFlagBits(RenderAPI::TextureFormat format) {
  if (format >= RenderAPI::TextureFormat::kD16_UNORM &&
      format <= RenderAPI::TextureFormat::kD32_SFLOAT) {
    return RenderAPI::ImageAspectFlagBits::kDepthBit;
  }

  if (format == RenderAPI::TextureFormat::kS8_UINT) {
    return RenderAPI::ImageAspectFlagBits::kStencilBit;
  }

  if (format >= RenderAPI::TextureFormat::kD16_UNORM_S8_UINT &&
      format <= RenderAPI::TextureFormat::kD32_SFLOAT_S8_UINT) {
    return RenderAPI::ImageAspectFlagBits::kDepthBit |
           RenderAPI::ImageAspectFlagBits::kStencilBit;
  }
  return RenderAPI::ImageAspectFlagBits::kColorBit;
}

RenderAPI::ImageUsageFlags GetImageUsageFlagsForFormat(
    RenderAPI::TextureFormat format) {
  if (format >= RenderAPI::TextureFormat::kD16_UNORM &&
      format <= RenderAPI::TextureFormat::kD32_SFLOAT_S8_UINT) {
    return RenderAPI::ImageUsageFlagBits::kSampledBit |
           RenderAPI::ImageUsageFlagBits::kDepthStencilAttachmentBit;
  }
  return RenderAPI::ImageUsageFlagBits::kSampledBit |
         RenderAPI::ImageUsageFlagBits::kColorAttachmentBit;
}

RenderAPI::ImageView RenderGraphCache::CreateTransientTexture(
    const RenderGraphTextureDesc& info) {
  for (auto& it : transient_textures_) {
    if (it.frames_since_use > 0 && it.info == info) {
      it.frames_since_use = 0;
      return it.image_view;
    }
  }

  // Create a new transient texture.
  TransientTexture texture;
  RenderAPI::ImageCreateInfo image_create_info(
      RenderAPI::TextureType::Texture2D, info.format,
      RenderAPI::Extent3D(info.width, info.height, 1),
      GetImageUsageFlagsForFormat(info.format));
  texture.image = RenderAPI::CreateImage(device_, image_create_info);
  const RenderAPI::ImageAspectFlags aspect_bits =
      GetAspectFlagBits(info.format);
  RenderAPI::ImageViewCreateInfo image_view_info(
      texture.image, RenderAPI::ImageViewType::Texture2D, info.format,
      RenderAPI::ImageSubresourceRange(aspect_bits));
  texture.image_view = RenderAPI::CreateImageView(device_, image_view_info);
  texture.info = info;
  transient_textures_.emplace_back(std::move(texture));
  return transient_textures_.back().image_view;
}

const RenderGraphFramebuffer& RenderGraphCache::CreateTransientFramebuffer(
    const RenderGraphFramebufferDesc& info,
    const std::vector<RenderAPI::ImageView>& textures) {
  for (TransientFramebuffer& it : transient_buffers_) {
    if (it.frames_since_use > 0 && it.info == info && textures == it.textures) {
      it.frames_since_use = 0;
      return it.resources;
    }
  }

  TransientFramebuffer buffer;
  buffer.textures = textures;

  RenderAPI::RenderPassCreateInfo render_pass_info;
  for (const auto& texture_desc : info.textures) {
    const RenderAPI::ImageAspectFlags aspect_bits =
        GetAspectFlagBits(texture_desc.format);

    RenderAPI::AttachmentDescription attachment;
    attachment.final_layout = texture_desc.layout;
    attachment.format = texture_desc.format;
    attachment.load_op = texture_desc.load_op;
    attachment.store_op = RenderAPI::AttachmentStoreOp::kStore;
    if (aspect_bits & RenderAPI::ImageAspectFlagBits::kDepthBit ||
        aspect_bits & RenderAPI::ImageAspectFlagBits::kStencilBit) {
      attachment.load_op = texture_desc.load_op;
      attachment.store_op = RenderAPI::AttachmentStoreOp::kStore;
      attachment.stencil_load_op = texture_desc.load_op;
      attachment.stencil_store_op = RenderAPI::AttachmentStoreOp::kStore;
    }
    render_pass_info.attachments.emplace_back(std::move(attachment));
  }

  buffer.resources.pass =
      RenderAPI::CreateRenderPass(device_, render_pass_info);

  RenderAPI::FramebufferCreateInfo fb_info;
  fb_info.pass = buffer.resources.pass;
  fb_info.width = info.textures[0].width;
  fb_info.height = info.textures[0].height;
  fb_info.attachments = textures;
  buffer.resources.framebuffer = RenderAPI::CreateFramebuffer(device_, fb_info);

  for (const auto& texture_desc : info.textures) {
    buffer.resources.clear_values.emplace_back(texture_desc.clear_values);
  }
  buffer.resources.render_area = {0, 0, fb_info.width, fb_info.height};

  buffer.info = info;
  transient_buffers_.emplace_back(std::move(buffer));
  return transient_buffers_.back().resources;
}
