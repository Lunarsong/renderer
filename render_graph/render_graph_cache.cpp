#include "render_graph_cache.h"

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
  cmd_index_ = 0;
  semaphore_index_ = 0;
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