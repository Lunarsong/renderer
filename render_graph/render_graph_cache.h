#pragma once

#include <renderer/renderer.h>
#include <unordered_map>
#include "generational/generational.h"
#include "render_graph_resources.h"

class RenderGraphCache {
 public:
  RenderAPI::ImageView CreateTransientTexture(
      const RenderGraphTextureDesc& info);
  const RenderGraphFramebuffer& CreateTransientFramebuffer(
      const RenderGraphFramebufferDesc& info,
      const std::vector<RenderAPI::ImageView>& textures);

  RenderAPI::CommandBuffer AllocateCommand();
  RenderAPI::Semaphore AllocateSemaphore();

  void SetRenderObjects(RenderAPI::Device device, RenderAPI::CommandPool pool);
  void Reset();

  void Destroy();

 private:
  RenderAPI::Device device_;
  RenderAPI::CommandPool pool_;

  size_t cmd_index_ = 0;
  std::vector<RenderAPI::CommandBuffer> cmds_;
  size_t semaphore_index_ = 0;
  std::vector<RenderAPI::Semaphore> semaphores_;

  struct TransientTexture {
    RenderGraphTextureDesc info;

    RenderAPI::ImageView image_view;
    RenderAPI::Image image;

    uint8_t frames_since_use = 0;
  };
  struct TransientFramebuffer {
    RenderGraphFramebufferDesc info;

    RenderGraphFramebuffer resources;
    std::vector<RenderAPI::ImageView> textures;

    uint8_t frames_since_use = 0;
  };
  std::vector<TransientFramebuffer> transient_buffers_;
  std::vector<TransientTexture> transient_textures_;
};