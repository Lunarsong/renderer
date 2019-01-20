#pragma once

#include <renderer/renderer.h>
#include <vector>
#include "generational/generational.h"

using RenderGraphMutableResource = Generational::HandleType;
using RenderGraphResource = Generational::HandleType;

struct RenderGraphFramebuffer {
  Renderer::RenderPass pass;
  Renderer::Framebuffer framebuffer;

  std::vector<RenderGraphResource> textures;
};

struct RenderGraphTexture {
  Renderer::Image image;
  Renderer::ImageView image_view;
};

struct RenderGraphClearValues {
  float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float depth = 1.0f;
  uint32_t stencil = 0;
};

struct RenderGraphTextureCreateInfo {
  uint32_t width;
  uint32_t height;

  Renderer::TextureFormat format;
  Renderer::AttachmentLoadOp load_op;

  RenderGraphClearValues clear_values;
};
