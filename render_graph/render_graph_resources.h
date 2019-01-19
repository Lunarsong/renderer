#pragma once

#include <renderer/renderer.h>
#include "generational/generational.h"

using RenderGraphMutableResource = Generational::HandleType;
using RenderGraphResource = Generational::HandleType;

struct RenderGraphFramebuffer {
  Renderer::RenderPass pass;
  Renderer::Framebuffer framebuffer;
};
