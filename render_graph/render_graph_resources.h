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
  std::vector<Renderer::ClearValue> clear_values;
};

struct RenderGraphTexture {
  Renderer::Image image;
  Renderer::ImageView image_view;
};

struct RenderGraphTextureCreateInfo {
  uint32_t width;
  uint32_t height;

  Renderer::TextureFormat format;
  Renderer::AttachmentLoadOp load_op = Renderer::AttachmentLoadOp::kDontCare;

  Renderer::ClearValue clear_values;
};
