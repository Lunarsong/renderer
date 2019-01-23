#pragma once

#include <renderer/renderer.h>
#include <vector>
#include "generational/generational.h"

using RenderGraphResource = Generational::HandleType;

struct RenderGraphResourceTextures {
  std::vector<RenderGraphResource> textures;
};

struct RenderGraphFramebuffer {
  Renderer::RenderPass pass;
  Renderer::Framebuffer framebuffer;

  std::vector<Renderer::ClearValue> clear_values;
};

struct RenderGraphTextureDesc {
  uint32_t width;
  uint32_t height;

  Renderer::TextureFormat format;
  Renderer::ImageLayout layout = Renderer::ImageLayout::kShaderReadOnlyOptimal;
  Renderer::AttachmentLoadOp load_op = Renderer::AttachmentLoadOp::kDontCare;
  Renderer::ClearValue clear_values;
};

struct RenderGraphFramebufferDesc {
  std::vector<RenderGraphTextureDesc> textures;
};