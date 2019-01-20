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
  textures_.clear();
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
}

RenderGraphFramebuffer* RenderGraphCache::GetFrameBuffer(
    RenderGraphMutableResource handle) {
  auto& it = framebuffers_.find(handle);
  if (it == framebuffers_.end()) {
    return nullptr;
  }
  return &it->second;
}

RenderGraphMutableResource RenderGraphCache::CreateTransientFramebuffer(
    const RenderGraphTextureCreateInfo& info) {
  for (TransientFramebuffer& it : transient_buffers_) {
    if (it.frames_since_use > 0 && it.info == info) {
      ++it.frames_since_use;
      return AddFramebuffer(it.resources);
    }
  }

  TransientFramebuffer buffer;
  transient_buffers_.emplace_back(std::move(buffer));
  return AddFramebuffer(transient_buffers_.back().resources);
}

RenderGraphResource RenderGraphCache::AddTexture(RenderGraphTexture texture) {
  RenderGraphMutableResource handle = read_handles_.Create();
  textures_[handle] = std::move(texture);
  return handle;
}