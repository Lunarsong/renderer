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

class Scope {
 public:
  Renderer::ImageView GetTexture(RenderGraphResource resource) const;

 private:
  friend class RenderGraphBuilder;
  std::unordered_map<RenderGraphResource, Renderer::ImageView> textures_;
};

using RenderGraphRenderFn =
    std::function<void(RenderContext* context, const Scope& scope)>;
using RenderGraphSetupFn = std::function<void(RenderGraphBuilder&)>;

struct RenderGraphPassInfo {
  Renderer::RenderPassCreateInfo pass;
};

struct RenderGraphPass {
  std::string name;
  RenderGraphRenderFn fn;
  RenderGraphSetupFn setup;

  Scope scope;

  uint8_t ref_count = 0;
};

struct RenderGraphCombinedRenderPasses {
  std::vector<RenderGraphPass*> passes;

  RenderGraphFramebuffer framebuffer;
};

struct RenderGraphNode {
  std::vector<RenderGraphCombinedRenderPasses> render_passes;

  Renderer::CommandBuffer render_cmd = Renderer::kInvalidHandle;
  std::vector<Renderer::Semaphore> wait_semaphores;
  std::vector<Renderer::Semaphore> signal_semaphores;
};
