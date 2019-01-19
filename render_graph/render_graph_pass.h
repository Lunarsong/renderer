#pragma once

#include <renderer/renderer.h>
#include <cstdint>
#include <functional>

class RenderGraphBuilder;
struct RenderContext {
  Renderer::CommandBuffer cmd;
  Renderer::RenderPass pass;
  Renderer::Framebuffer framebuffer;
};

using RenderGraphRenderFn = std::function<void(RenderContext* context)>;
using RenderGraphSetupFn = std::function<void(RenderGraphBuilder&)>;

struct RenderGraphPassInfo {
  Renderer::RenderPassCreateInfo pass;
};

struct RenderGraphPass {
  std::string name;
  RenderGraphRenderFn fn;
  RenderGraphSetupFn setup;

  Renderer::CommandBuffer cmd = Renderer::kInvalidHandle;

  Renderer::RenderPass render_pass = Renderer::kInvalidHandle;
  Renderer::Framebuffer framebuffer = Renderer::kInvalidHandle;
  std::vector<Renderer::Semaphore> wait_semaphores;
  std::vector<Renderer::Semaphore> signal_semaphores;
};