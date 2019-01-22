#include "render_graph_cache.h"

bool operator==(const RenderGraphTextureCreateInfo& a,
                const RenderGraphTextureCreateInfo& b) {
  return (a.format == b.format && a.width == b.width && a.height == b.height &&
          a.load_op == b.load_op);
}

void RenderGraphCache::SetRenderObjects(Renderer::Device device,
                                        Renderer::CommandPool pool) {
  device_ = device;
  pool_ = pool;
}

RenderGraphMutableResource RenderGraphCache::AddFramebuffer(
    RenderGraphFramebuffer buffer) {
  RenderGraphMutableResource handle = mutable_handles_.Create();
  framebuffers_[handle] = std::move(buffer);
  return handle;
}

void RenderGraphCache::Reset() {
  framebuffers_.clear();
  mutable_handles_.Reset();
  cmd_index_ = 0;
  semaphore_index_ = 0;

  for (auto& it : transient_buffers_) {
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
    for (auto& texture : it.textures) {
      Renderer::DestroyImageView(device_, texture.image_view);
      Renderer::DestroyImage(texture.image);
    }
    Renderer::DestroyFramebuffer(it.resources.framebuffer);
    Renderer::DestroyRenderPass(it.resources.pass);
  }
}

RenderGraphFramebuffer* RenderGraphCache::GetFrameBuffer(
    RenderGraphMutableResource handle) {
  auto& it = framebuffers_.find(handle);
  if (it == framebuffers_.end()) {
    return nullptr;
  }
  return &it->second;
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

RenderGraphMutableResource RenderGraphCache::CreateTransientFramebuffer(
    const RenderGraphTextureCreateInfo& info) {
  for (TransientFramebuffer& it : transient_buffers_) {
    if (it.frames_since_use > 0 && it.info == info) {
      it.frames_since_use = 0;
      return AddFramebuffer(it.resources);
    }
  }

  TransientFramebuffer buffer;
  buffer.textures.resize(1);
  Renderer::ImageCreateInfo image_create_info(
      Renderer::TextureType::Texture2D, info.format,
      Renderer::Extent3D(info.width, info.height, 1),
      GetImageUsageFlagsForFormat(info.format));
  buffer.textures[0].image = Renderer::CreateImage(device_, image_create_info);
  const Renderer::ImageAspectFlags aspect_bits = GetAspectFlagBits(info.format);
  Renderer::ImageViewCreateInfo image_view_info(
      buffer.textures[0].image, Renderer::ImageViewType::Texture2D, info.format,
      Renderer::ImageSubresourceRange(aspect_bits));
  buffer.textures[0].image_view =
      Renderer::CreateImageView(device_, image_view_info);

  Renderer::RenderPassCreateInfo render_pass_info;
  Renderer::AttachmentDescription attachment;
  attachment.final_layout = Renderer::ImageLayout::kShaderReadOnlyOptimal;
  attachment.format = info.format;
  if (aspect_bits & Renderer::ImageAspectFlagBits::kColorBit) {
    attachment.load_op = info.load_op;
    attachment.store_op = Renderer::AttachmentStoreOp::kStore;
    render_pass_info.color_attachments.emplace_back(std::move(attachment));
  } else {
    attachment.stencil_load_op = info.load_op;
    attachment.stencil_store_op = Renderer::AttachmentStoreOp::kStore;
    render_pass_info.depth_stencil_attachments.emplace_back(
        std::move(attachment));
  }
  buffer.resources.pass = Renderer::CreateRenderPass(device_, render_pass_info);

  Renderer::FramebufferCreateInfo fb_info;
  fb_info.pass = buffer.resources.pass;
  fb_info.width = info.width;
  fb_info.height = info.height;
  fb_info.attachments.push_back(buffer.textures[0].image_view);
  buffer.resources.framebuffer = Renderer::CreateFramebuffer(device_, fb_info);

  for (const auto& texture : buffer.textures) {
    buffer.resources.textures.emplace_back(AddTexture(texture));
  }
  buffer.resources.clear_values.emplace_back(info.clear_values);

  buffer.info = std::move(info);
  transient_buffers_.emplace_back(std::move(buffer));
  return AddFramebuffer(transient_buffers_.back().resources);
}

RenderGraphResource RenderGraphCache::AddTexture(RenderGraphTexture texture) {
  RenderGraphMutableResource handle = read_handles_.Create();
  textures_[handle] = std::move(texture);
  return handle;
}

RenderGraphTexture RenderGraphCache::GetTexture(
    RenderGraphResource texture) const {
  return textures_.find(texture)->second;
}