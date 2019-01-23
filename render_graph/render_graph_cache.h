#pragma once

#include <renderer/renderer.h>
#include <unordered_map>
#include "generational/generational.h"
#include "render_graph_resources.h"

class RenderGraphCache {
 public:
  Renderer::ImageView CreateTransientTexture(
      const RenderGraphTextureDesc& info);
  const RenderGraphFramebuffer& CreateTransientFramebuffer(
      const RenderGraphFramebufferDesc& info,
      const std::vector<Renderer::ImageView>& textures);

  Renderer::CommandBuffer AllocateCommand();
  Renderer::Semaphore AllocateSemaphore();

  void SetRenderObjects(Renderer::Device device, Renderer::CommandPool pool);
  void Reset();

  void Destroy();

 private:
  Renderer::Device device_;
  Renderer::CommandPool pool_;

  size_t cmd_index_ = 0;
  std::vector<Renderer::CommandBuffer> cmds_;
  size_t semaphore_index_ = 0;
  std::vector<Renderer::Semaphore> semaphores_;

  struct TransientTexture {
    RenderGraphTextureDesc info;

    Renderer::ImageView image_view;
    Renderer::Image image;

    uint8_t frames_since_use = 0;
  };
  struct TransientFramebuffer {
    RenderGraphFramebufferDesc info;

    RenderGraphFramebuffer resources;
    std::vector<Renderer::ImageView> textures;

    uint8_t frames_since_use = 0;
  };
  std::vector<TransientFramebuffer> transient_buffers_;
  std::vector<TransientTexture> transient_textures_;
};