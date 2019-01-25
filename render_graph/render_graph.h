#pragma once

#include <renderer/renderer.h>
#include <cstdint>
#include <memory>
#include <string>
#include "render_graph_builder.h"
#include "render_graph_cache.h"
#include "render_graph_pass.h"

struct RenderGraphPass;
class RenderGraph {
 public:
  RenderGraph(Renderer::Device device);
  ~RenderGraph();

  void BuildSwapChain(uint32_t width, uint32_t height);
  void Destroy();

  void BeginFrame();
  void Render();

  void AddPass(const std::string& name, RenderGraphSetupFn setup_fn,
               RenderGraphRenderFn render_fn);

  // Resources.
  RenderGraphResource ImportTexture(RenderGraphTextureDesc desc,
                                    Renderer::ImageView texture);
  void MoveSubresource(RenderGraphResource from, RenderGraphResource to);

  // Swapchain
  Renderer::SwapChain GetSwapChain() const;
  RenderGraphResource GetBackbufferResource() const;
  const RenderGraphTextureDesc& GetSwapChainDescription() const;

 private:
  Renderer::Device device_;
  Renderer::SwapChain swapchain_ = Renderer::kInvalidHandle;
  Renderer::CommandPool command_pool_ = Renderer::kInvalidHandle;

  RenderGraphBuilder builder_;
  RenderGraphCache cache_;

  // Current frame and how many frames are allowed to be drawn at the same time.
  uint32_t current_frame_ = 0;
  uint32_t max_frames_in_flight = 1;

  // Swapchain information (should probably be moved elsewhere).
  uint32_t current_backbuffer_image_;
  Renderer::RenderPass backbuffer_render_pass_ = Renderer::kInvalidHandle;
  std::vector<Renderer::Semaphore> present_semaphores_;
  std::vector<Renderer::Semaphore> backbuffer_complete_semaphores_;
  std::vector<Renderer::Fence> backbuffer_fences_;
  RenderGraphResource backbuffer_resource_;
  RenderGraphTextureDesc swapchain_desc_;

  void CreateSyncObjects();
  void DestroySyncObjects();
  void AcquireBackbuffer();
  void DestroySwapChain();

  // Passes.
  std::vector<RenderGraphPass> passes_;
  std::vector<RenderGraphNode> Compile();
  Renderer::Semaphore ExecuteRenderPasses(std::vector<RenderGraphNode>& nodes,
                                          Renderer::Semaphore semaphore);
};
