#pragma once

#include <renderer/renderer.h>
#include <unordered_map>
#include "generational/generational.h"
#include "render_graph_resources.h"

class RenderGraphCache {
 public:
  RenderGraphMutableResource AddFramebuffer(RenderGraphFramebuffer buffer);
  RenderGraphResource AddTexture(RenderGraphTexture texture);

  RenderGraphMutableResource CreateTransientFramebuffer(
      const RenderGraphTextureCreateInfo& info);

  RenderGraphFramebuffer* GetFrameBuffer(RenderGraphMutableResource);

  Renderer::CommandBuffer AllocateCommand();
  Renderer::Semaphore AllocateSemaphore();

  void SetRenderObjects(Renderer::Device device, Renderer::CommandPool pool);
  void Reset();

  void Destroy();

 private:
  Renderer::Device device_;
  Renderer::CommandPool pool_;
  Generational::Manager mutable_handles_;
  Generational::Manager read_handles_;

  std::unordered_map<RenderGraphMutableResource, RenderGraphFramebuffer>
      framebuffers_;
  std::unordered_map<RenderGraphResource, RenderGraphTexture> textures_;

  size_t cmd_index_ = 0;
  std::vector<Renderer::CommandBuffer> cmds_;
  size_t semaphore_index_ = 0;
  std::vector<Renderer::Semaphore> semaphores_;

  struct TransientFramebuffer {
    RenderGraphTextureCreateInfo info;

    RenderGraphFramebuffer resources;
    std::vector<RenderGraphTexture> textures;

    uint8_t frames_since_use = 0;
  };
  std::vector<TransientFramebuffer> transient_buffers_;
};