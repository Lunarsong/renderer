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

  const RenderGraphResourceTextures& UseRenderTarget(
      RenderGraphResource resource);
  const RenderGraphResourceTextures& CreateRenderTarget(
      RenderGraphTextureDesc info);
  const RenderGraphResourceTextures& CreateRenderTarget(
      RenderGraphFramebufferDesc info);

  void Write(RenderGraphResource resource);
  void Read(RenderGraphResource resource);

  // For use only by the render graph.
  std::vector<RenderGraphNode> Build(std::vector<RenderGraphPass>& passes);
  void SetCurrentPass(RenderGraphPassHandle pass);
  void Reset();

 private:
  friend class RenderGraph;

  RenderGraphCache* cache_;
  RenderGraphPassHandle current_pass_;

  Generational::Manager handles_;

  // Resources by use.
  std::unordered_map<RenderGraphResource, std::vector<RenderGraphPassHandle>>
      reads_;
  std::unordered_map<RenderGraphResource, std::vector<RenderGraphPassHandle>>
      writes_;
  std::unordered_map<RenderGraphResource, std::vector<RenderGraphPassHandle>>
      render_targets_;

  // Resources.
  struct RenderGraphFramebufferResource {
    RenderGraphFramebufferDesc desc;
    RenderGraphFramebuffer framebuffer;
    RenderGraphResourceTextures textures;

    bool transient = true;
    uint8_t ref_count = 0;
  };

  struct RenderGraphTextureResource {
    RenderGraphTextureDesc desc;
    RenderAPI::ImageView texture;

    bool transient = true;
    uint8_t ref_count = 0;
  };

  std::unordered_map<RenderGraphResource, RenderGraphFramebufferResource>
      framebuffers_;
  std::unordered_map<RenderGraphResource, RenderGraphTextureResource> textures_;

  // Aliasing.
  std::unordered_map<RenderGraphResource, RenderGraphResource> aliases_;
  RenderGraphResource GetAlised(RenderGraphResource handle) const;

  // Debug info.
  uint8_t debug_current_pass_render_targets_ = 0;

  RenderGraphResource CreateTexture(RenderGraphTextureDesc info);
};