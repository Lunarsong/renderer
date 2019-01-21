#pragma once

#include <unordered_map>
#include <vector>
#include "render_graph_cache.h"
#include "render_graph_pass.h"
#include "render_graph_resources.h"

using RenderGraphPassHandle = size_t;

class RenderGraphBuilder {
 public:
  RenderGraphBuilder(RenderGraphCache* cache);

  const RenderGraphFramebuffer& UseRenderTarget(RenderGraphResource resource);
  const RenderGraphFramebuffer& CreateRenderTarget(
      const RenderGraphTextureCreateInfo& info);

  void Write(RenderGraphResource resource);
  void Read(RenderGraphResource resource);

  // For use only by the render graph.
  std::vector<RenderGraphNode> Build(std::vector<RenderGraphPass>& passes);
  void SetCurrentPass(RenderGraphPassHandle pass);
  void Reset();

 private:
  RenderGraphCache* cache_;
  RenderGraphPassHandle current_pass_;

  std::unordered_map<RenderGraphMutableResource,
                     std::vector<RenderGraphPassHandle>>
      reads_;
  std::unordered_map<RenderGraphMutableResource,
                     std::vector<RenderGraphPassHandle>>
      writes_;
  std::unordered_map<RenderGraphMutableResource,
                     std::vector<RenderGraphPassHandle>>
      render_targets_;

  // Debug info.
  uint8_t debug_current_pass_render_targets_ = 0;
};