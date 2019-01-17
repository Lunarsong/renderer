#pragma once

#include <renderer/renderer.h>
#include <cstdint>
#include <functional>

struct RenderContext {
  Renderer::CommandBuffer cmd;
  Renderer::RenderPass pass;
  Renderer::Framebuffer framebuffer;
};
using RenderGraphRenderFn = std::function<void(RenderContext* context)>;
using RenderGraphCompileFn = std::function<void(Renderer::RenderPass)>;

struct RenderGraphPassInfo {
  Renderer::RenderPassCreateInfo pass;
};
