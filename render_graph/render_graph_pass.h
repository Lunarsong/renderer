#pragma once

#include <renderer/renderer.h>
#include <cstdint>
#include <functional>

class RenderGraphBuilder;
struct RenderContext {
  RenderAPI::CommandBuffer cmd;
  RenderAPI::RenderPass pass;
  RenderAPI::Framebuffer framebuffer;
};

class Scope {
 public:
  RenderAPI::ImageView GetTexture(RenderGraphResource resource) const;

 private:
  friend class RenderGraphBuilder;
  std::unordered_map<RenderGraphResource, RenderAPI::ImageView> textures_;
};

using RenderGraphRenderFn =
    std::function<void(RenderContext* context, const Scope& scope)>;
using RenderGraphSetupFn = std::function<void(RenderGraphBuilder&)>;

struct RenderGraphPassInfo {
  RenderAPI::RenderPassCreateInfo pass;
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

  RenderAPI::CommandBuffer render_cmd = RenderAPI::kInvalidHandle;
  std::vector<RenderAPI::Semaphore> wait_semaphores;
  std::vector<RenderAPI::Semaphore> signal_semaphores;
};
