#include "render_graph.h"

#include <iostream>

RenderGraph::RenderGraph(Renderer::Device device) : device_(device) {
  command_pool_ = Renderer::CreateCommandPool(
      device_, Renderer::CommandPoolCreateFlags::kResetCommand);
}

RenderGraph::~RenderGraph() { Destroy(); }

void RenderGraph::Destroy() {
  if (device_ == Renderer::kInvalidHandle) {
    return;
  }
  Renderer::DeviceWaitIdle(device_);

  for (auto& pass : render_passes_) {
    DestroyRenderPass(pass.get());
  }

  if (swapchain_ != Renderer::kInvalidHandle) {
    Renderer::DestroySwapChain(swapchain_);
    swapchain_ = Renderer::kInvalidHandle;
  }
  if (command_pool_ != Renderer::kInvalidHandle) {
    Renderer::DestroyCommandPool(command_pool_);
    command_pool_ = Renderer::kInvalidHandle;
  }
  DestroySyncObjects();
  device_ = Renderer::kInvalidHandle;
}

void RenderGraph::BuildSwapChain(uint32_t width, uint32_t height) {
  Renderer::DeviceWaitIdle(device_);
  DestroySyncObjects();
  if (swapchain_ != Renderer::kInvalidHandle) {
    Renderer::DestroySwapChain(swapchain_);
    swapchain_ = Renderer::kInvalidHandle;
  }

  swapchain_ = Renderer::CreateSwapChain(device_, width, height);
  max_frames_in_flight = Renderer::GetSwapChainLength(swapchain_);

  swapchain_width_ = width;
  swapchain_height_ = height;

  CreateSyncObjects();
}

void RenderGraph::Render() {
  bool has_work = !render_passes_.empty();
  if (!has_work) {
    return;
  }

  // If the CPU is ahead, wait on fences.
  Renderer::WaitForFences(&backbuffer_fences_[current_frame_], 1, true,
                          std::numeric_limits<uint64_t>::max());
  Renderer::ResetFences(&backbuffer_fences_[current_frame_], 1);

  // Prepare the next swapchain frame buffer for rendering.
  uint32_t image_index;
  Renderer::AcquireNextImage(swapchain_, std::numeric_limits<uint64_t>::max(),
                             present_semaphores_[current_frame_], &image_index);
  current_backbuffer_image_ = image_index;

  // Submit work from the renderer passes.
  Renderer::Semaphore render_complete_semaphore =
      ExecuteRenderPasses(present_semaphores_[current_frame_]);

  // Present.
  Renderer::PresentInfo present_info;
  if (render_complete_semaphore != Renderer::kInvalidHandle) {
    present_info.wait_semaphores = &render_complete_semaphore;
    //&rendering_complete_semaphores_[current_frame_];
    present_info.wait_semaphores_count = 1;
  } else {
    present_info.wait_semaphores_count = 0;
  }

  Renderer::QueuePresent(swapchain_, image_index, present_info);

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight;
}

void RenderGraph::Submit() {}

void RenderGraph::CreateSyncObjects() {
  present_semaphores_.resize(max_frames_in_flight);
  // rendering_complete_semaphores_.resize(max_frames_in_flight);
  backbuffer_fences_.resize(max_frames_in_flight);

  for (auto& it : present_semaphores_) {
    it = Renderer::CreateSemaphore(device_);
  }
  /*for (auto& it : rendering_complete_semaphores_) {
    it = Renderer::CreateSemaphore(device_);
  }*/
  for (auto& it : backbuffer_fences_) {
    it = Renderer::CreateFence(device_, true);
  }
}

void RenderGraph::DestroySyncObjects() {
  for (auto it : present_semaphores_) {
    Renderer::DestroySemaphore(it);
  }
  /*for (auto it : rendering_complete_semaphores_) {
    Renderer::DestroySemaphore(it);
  }*/
  for (auto it : backbuffer_fences_) {
    Renderer::DestroyFence(it);
  }
  present_semaphores_.clear();
  // rendering_complete_semaphores_.clear();
  backbuffer_fences_.clear();
}

void RenderGraph::CreateRenderPass(RenderGraphCompileFn compile_fn,
                                   RenderGraphRenderFn render_fn) {
  std::unique_ptr<RenderGraphPass> pass = std::make_unique<RenderGraphPass>();
  pass->fn = std::move(render_fn);

  CompileRenderPass(pass.get());
  if (compile_fn) {
    compile_fn(pass->pass);
  }
  render_passes_.emplace_back(std::move(pass));
}

void RenderGraph::CompileRenderPass(RenderGraphPass* pass) {
  static bool first = true;
  Renderer::RenderPassCreateInfo render_pass_info;
  render_pass_info.color_attachments.resize(1);
  render_pass_info.color_attachments[0].format =
      Renderer::GetSwapChainImageFormat(swapchain_);
  if (first) {
    render_pass_info.color_attachments[0].load_op =
        Renderer::AttachmentLoadOp::kClear;
  } else {
    render_pass_info.color_attachments[0].load_op =
        Renderer::AttachmentLoadOp::kLoad;
  }
  render_pass_info.color_attachments[0].store_op =
      Renderer::AttachmentStoreOp::kStore;
  render_pass_info.color_attachments[0].final_layout =
      Renderer::ImageLayout::kPresentSrcKHR;
  pass->pass = Renderer::CreateRenderPass(device_, render_pass_info);

  first = false;

  const uint32_t swapchain_length = Renderer::GetSwapChainLength(swapchain_);
  pass->frame_buffers.resize(swapchain_length);
  for (uint32_t i = 0; i < swapchain_length; ++i) {
    Renderer::FramebufferCreateInfo info;
    info.pass = pass->pass;
    info.width = swapchain_width_;
    info.height = swapchain_height_;
    info.attachments.push_back(Renderer::GetSwapChainImageView(swapchain_, i));
    pass->frame_buffers[i] = Renderer::CreateFramebuffer(device_, info);
  }

  pass->command_buffers.resize(swapchain_length);
  for (auto& it : pass->command_buffers) {
    it = Renderer::CreateCommandBuffer(command_pool_);
  }

  pass->semaphores.resize(swapchain_length);
  for (auto& it : pass->semaphores) {
    it = Renderer::CreateSemaphore(device_);
  }
}

void RenderGraph::DestroyRenderPass(RenderGraphPass* pass) {
  for (auto& it : pass->command_buffers) {
    Renderer::DestroyCommandBuffer(it);
  }
  for (auto& it : pass->semaphores) {
    Renderer::DestroySemaphore(it);
  }
  for (auto& it : pass->frame_buffers) {
    Renderer::DestroyFramebuffer(it);
  }
  Renderer::DestroyRenderPass(pass->pass);
}

Renderer::Semaphore RenderGraph::ExecuteRenderPasses(
    Renderer::Semaphore semaphore) {
  // Set the semaphores and command buffer for the current frame buffer.
  Renderer::SubmitInfo submit_info;
  submit_info.wait_semaphores_count = 1;
  submit_info.signal_semaphores_count = 1;

  RenderContext context;
  size_t count = 0;
  for (auto& pass : render_passes_) {
    submit_info.wait_semaphores = &semaphore;

    CreateRenderContextForPass(&context, pass.get());
    Renderer::CmdBegin(context.cmd);
    Renderer::CmdBeginRenderPass(context.cmd, context.pass,
                                 context.framebuffer);
    pass->fn(&context);
    Renderer::CmdEndRenderPass(context.cmd);
    Renderer::CmdEnd(context.cmd);

    Renderer::Semaphore signal_semaphore =
        pass->semaphores[current_frame_ % pass->semaphores.size()];
    submit_info.signal_semaphores = &signal_semaphore;
    submit_info.command_buffers = &context.cmd;
    submit_info.command_buffers_count = 1;

    ++count;
    Renderer::Fence fence = (count == render_passes_.size())
                                ? backbuffer_fences_[current_frame_]
                                : Renderer::kInvalidHandle;
    Renderer::QueueSubmit(device_, submit_info, fence);
    semaphore = signal_semaphore;
  }
  return semaphore;
}

void RenderGraph::CreateRenderContextForPass(RenderContext* context,
                                             const RenderGraphPass* pass) {
  context->cmd = pass->command_buffers[current_backbuffer_image_ %
                                       pass->command_buffers.size()];
  context->framebuffer = pass->frame_buffers[current_backbuffer_image_ %
                                             pass->frame_buffers.size()];
  context->pass = pass->pass;
}
