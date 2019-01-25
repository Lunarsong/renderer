#pragma once

#include <renderer/renderer.h>
#include <vector>
#include "generational/generational.h"

using RenderGraphResource = Generational::HandleType;

struct RenderGraphResourceTextures {
  std::vector<RenderGraphResource> textures;
};

struct RenderGraphFramebuffer {
  RenderAPI::RenderPass pass;
  RenderAPI::Framebuffer framebuffer;

  std::vector<RenderAPI::ClearValue> clear_values;
};

struct RenderGraphTextureDesc {
  uint32_t width;
  uint32_t height;

  RenderAPI::TextureFormat format;
  RenderAPI::ImageLayout layout =
      RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
  RenderAPI::AttachmentLoadOp load_op = RenderAPI::AttachmentLoadOp::kDontCare;
  RenderAPI::ClearValue clear_values;
};

struct RenderGraphFramebufferDesc {
  std::vector<RenderGraphTextureDesc> textures;
};