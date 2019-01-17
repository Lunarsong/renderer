#pragma once

#include <renderer/renderer.h>
#include <cstdint>
#include <memory>
#include "render_graph_pass.h"

struct RenderGraphPass;
class RenderGraph {
 public:
  RenderGraph(Renderer::Device device);
  ~RenderGraph();

  void BuildSwapChain(uint32_t width, uint32_t height);
  void Destroy();

  void Render();

  void CreateRenderPass(RenderGraphCompileFn compile_fn,
                        RenderGraphRenderFn render_fn);

 private:
  Renderer::Device device_;
  Renderer::SwapChain swapchain_ = Renderer::kInvalidHandle;
  Renderer::CommandPool command_pool_ = Renderer::kInvalidHandle;

  uint32_t swapchain_width_;
  uint32_t swapchain_height_;
  uint32_t max_frames_in_flight = 1;
  uint32_t current_frame_ = 0;
  uint32_t current_backbuffer_image_;
  std::vector<Renderer::Semaphore> present_semaphores_;
  std::vector<Renderer::Semaphore> rendering_complete_semaphores_;
  std::vector<Renderer::Fence> backbuffer_fences_;
  Renderer::Semaphore last_semaphore_ = Renderer::kInvalidHandle;

  void Submit();
  void CreateSyncObjects();
  void DestroySyncObjects();

  // Render passes.
  std::vector<std::unique_ptr<RenderGraphPass>> render_passes_;
  void CompileRenderPass(RenderGraphPass* pass);
  void DestroyRenderPass(RenderGraphPass* pass);
  Renderer::Semaphore ExecuteRenderPasses(Renderer::Semaphore semaphore);
  void CreateRenderContextForPass(RenderContext* context,
                                  const RenderGraphPass* pass);
};

struct RenderGraphPass {
  Renderer::RenderPass pass;
  std::vector<Renderer::Framebuffer> frame_buffers;
  std::vector<Renderer::CommandBuffer> command_buffers;
  std::vector<Renderer::Semaphore> semaphores;
  RenderGraphRenderFn fn;
};