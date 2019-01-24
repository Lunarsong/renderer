#include "render_graph_cache.h"

bool operator==(const RenderGraphTextureDesc& a,
                const RenderGraphTextureDesc& b) {
  return (a.format == b.format && a.width == b.width && a.height == b.height &&
          a.load_op == b.load_op);
}

bool operator==(const RenderGraphFramebufferDesc& a,
                const RenderGraphFramebufferDesc& b) {
  return a.textures == b.textures;
}

void RenderGraphCache::SetRenderObjects(Renderer::Device device,
                                        Renderer::CommandPool pool) {
  device_ = device;
  pool_ = pool;
}

void RenderGraphCache::Reset() {
  cmd_index_ = 0;
  semaphore_index_ = 0;

  for (auto& it : transient_buffers_) {
    ++it.frames_since_use;
  }
  for (auto& it : transient_textures_) {
    ++it.frames_since_use;
  }
}

Renderer::CommandBuffer RenderGraphCache::AllocateCommand() {
  if (cmd_index_ == cmds_.size()) {
    cmds_.emplace_back(Renderer::CreateCommandBuffer(pool_));
  }
  return cmds_[cmd_index_++];
}

Renderer::Semaphore RenderGraphCache::AllocateSemaphore() {
  if (semaphore_index_ == semaphores_.size()) {
    semaphores_.emplace_back(Renderer::CreateSemaphore(device_));
  }
  return semaphores_[semaphore_index_++];
}

void RenderGraphCache::Destroy() {
  for (auto it : semaphores_) {
    Renderer::DestroySemaphore(it);
  }
  semaphores_.clear();

  for (auto& it : transient_buffers_) {
    Renderer::DestroyFramebuffer(it.resources.framebuffer);
    Renderer::DestroyRenderPass(it.resources.pass);
  }

  for (auto& it : transient_textures_) {
    Renderer::DestroyImageView(device_, it.image_view);
    Renderer::DestroyImage(it.image);
  }
}

Renderer::ImageAspectFlags GetAspectFlagBits(Renderer::TextureFormat format) {
  if (format >= Renderer::TextureFormat::kD16_UNORM &&
      format <= Renderer::TextureFormat::kD32_SFLOAT) {
    return Renderer::ImageAspectFlagBits::kDepthBit;
  }

  if (format == Renderer::TextureFormat::kS8_UINT) {
    return Renderer::ImageAspectFlagBits::kStencilBit;
  }

  if (format >= Renderer::TextureFormat::kD16_UNORM_S8_UINT &&
      format <= Renderer::TextureFormat::kD32_SFLOAT_S8_UINT) {
    return Renderer::ImageAspectFlagBits::kDepthBit |
           Renderer::ImageAspectFlagBits::kStencilBit;
  }
  return Renderer::ImageAspectFlagBits::kColorBit;
}

Renderer::ImageUsageFlags GetImageUsageFlagsForFormat(
    Renderer::TextureFormat format) {
  if (format >= Renderer::TextureFormat::kD16_UNORM &&
      format <= Renderer::TextureFormat::kD32_SFLOAT_S8_UINT) {
    return Renderer::ImageUsageFlagBits::kSampledBit |
           Renderer::ImageUsageFlagBits::kDepthStencilAttachmentBit;
  }
  return Renderer::ImageUsageFlagBits::kSampledBit |
         Renderer::ImageUsageFlagBits::kColorAttachmentBit;
}

Renderer::ImageView RenderGraphCache::CreateTransientTexture(
    const RenderGraphTextureDesc& info) {
  for (const auto& it : transient_textures_) {
    if (it.frames_since_use > 0 && it.info == info) {
      return it.image_view;
    }
  }

  // Create a new transient texture.
  TransientTexture texture;
  Renderer::ImageCreateInfo image_create_info(
      Renderer::TextureType::Texture2D, info.format,
      Renderer::Extent3D(info.width, info.height, 1),
      GetImageUsageFlagsForFormat(info.format));
  texture.image = Renderer::CreateImage(device_, image_create_info);
  const Renderer::ImageAspectFlags aspect_bits = GetAspectFlagBits(info.format);
  Renderer::ImageViewCreateInfo image_view_info(
      texture.image, Renderer::ImageViewType::Texture2D, info.format,
      Renderer::ImageSubresourceRange(aspect_bits));
  texture.image_view = Renderer::CreateImageView(device_, image_view_info);
  texture.info = info;
  transient_textures_.emplace_back(std::move(texture));
  return transient_textures_.back().image_view;
}

const RenderGraphFramebuffer& RenderGraphCache::CreateTransientFramebuffer(
    const RenderGraphFramebufferDesc& info,
    const std::vector<Renderer::ImageView>& textures) {
  for (TransientFramebuffer& it : transient_buffers_) {
    if (it.frames_since_use > 0 && it.info == info && textures == it.textures) {
      it.frames_since_use = 0;
      return it.resources;
    }
  }

  TransientFramebuffer buffer;

  Renderer::RenderPassCreateInfo render_pass_info;
  for (const auto& texture_desc : info.textures) {
    const Renderer::ImageAspectFlags aspect_bits =
        GetAspectFlagBits(texture_desc.format);

    Renderer::AttachmentDescription attachment;
    attachment.final_layout = texture_desc.layout;
    attachment.format = texture_desc.format;
    attachment.load_op = texture_desc.load_op;
    attachment.store_op = Renderer::AttachmentStoreOp::kStore;
    if (aspect_bits & Renderer::ImageAspectFlagBits::kDepthBit ||
        aspect_bits & Renderer::ImageAspectFlagBits::kStencilBit) {
      attachment.load_op = texture_desc.load_op;
      attachment.store_op = Renderer::AttachmentStoreOp::kStore;
      attachment.stencil_load_op = texture_desc.load_op;
      attachment.stencil_store_op = Renderer::AttachmentStoreOp::kStore;
    }
    render_pass_info.attachments.emplace_back(std::move(attachment));
  }

  buffer.resources.pass = Renderer::CreateRenderPass(device_, render_pass_info);

  Renderer::FramebufferCreateInfo fb_info;
  fb_info.pass = buffer.resources.pass;
  fb_info.width = info.textures[0].width;
  fb_info.height = info.textures[0].height;
  fb_info.attachments = textures;
  buffer.resources.framebuffer = Renderer::CreateFramebuffer(device_, fb_info);

  for (const auto& texture_desc : info.textures) {
    buffer.resources.clear_values.emplace_back(texture_desc.clear_values);
  }

  buffer.info = info;
  transient_buffers_.emplace_back(std::move(buffer));
  return transient_buffers_.back().resources;
}
