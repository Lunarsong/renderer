#include "render_graph.h"

#include <iostream>

RenderGraph::RenderGraph(Renderer::Device device)
    : device_(device), builder_(&cache_) {
  command_pool_ = Renderer::CreateCommandPool(
      device_, Renderer::CommandPoolCreateFlags::kResetCommand);
  cache_.SetRenderObjects(device_, command_pool_);
}

RenderGraph::~RenderGraph() { Destroy(); }

void RenderGraph::Destroy() {
  if (device_ == Renderer::kInvalidHandle) {
    return;
  }
  Renderer::DeviceWaitIdle(device_);

  cache_.Destroy();

  if (swapchain_ != Renderer::kInvalidHandle) {
    Renderer::DestroySwapChain(swapchain_);
    swapchain_ = Renderer::kInvalidHandle;
  }
  if (command_pool_ != Renderer::kInvalidHandle) {
    Renderer::DestroyCommandPool(command_pool_);
    command_pool_ = Renderer::kInvalidHandle;
  }
  DestroySwapChain();
  device_ = Renderer::kInvalidHandle;
}

void RenderGraph::DestroySwapChain() {
  Renderer::DeviceWaitIdle(device_);
  DestroySyncObjects();

  if (backbuffer_render_pass_ != Renderer::kInvalidHandle) {
    Renderer::DestroyRenderPass(backbuffer_render_pass_);
    backbuffer_render_pass_ = Renderer::kInvalidHandle;
  }
  for (auto it : backbuffer_framebuffers_) {
    Renderer::DestroyFramebuffer(it);
  }
  backbuffer_framebuffers_.clear();

  if (swapchain_ != Renderer::kInvalidHandle) {
    Renderer::DestroySwapChain(swapchain_);
    swapchain_ = Renderer::kInvalidHandle;
  }
}

void RenderGraph::BuildSwapChain(uint32_t width, uint32_t height) {
  DestroySwapChain();

  swapchain_ = Renderer::CreateSwapChain(device_, width, height);
  const uint32_t swapchain_length = Renderer::GetSwapChainLength(swapchain_);
  max_frames_in_flight = 1;  // swapchain_length;
  swapchain_width_ = width;
  swapchain_height_ = height;

  // Create a render pass for the swapchain.
  Renderer::RenderPassCreateInfo render_pass_info;
  render_pass_info.color_attachments.resize(1);
  render_pass_info.color_attachments[0].format =
      Renderer::GetSwapChainImageFormat(swapchain_);
  render_pass_info.color_attachments[0].load_op =
      Renderer::AttachmentLoadOp::kDontCare;
  render_pass_info.color_attachments[0].store_op =
      Renderer::AttachmentStoreOp::kStore;
  render_pass_info.color_attachments[0].final_layout =
      Renderer::ImageLayout::kPresentSrcKHR;
  backbuffer_render_pass_ =
      Renderer::CreateRenderPass(device_, render_pass_info);

  // Create framebuffers for the swapchain.
  backbuffer_framebuffers_.resize(swapchain_length);
  uint32_t i = 0;
  for (auto& it : backbuffer_framebuffers_) {
    Renderer::FramebufferCreateInfo info;
    info.pass = backbuffer_render_pass_;
    info.width = swapchain_width_;
    info.height = swapchain_height_;
    info.attachments.push_back(Renderer::GetSwapChainImageView(swapchain_, i));
    it = Renderer::CreateFramebuffer(device_, info);
    ++i;
  }

  CreateSyncObjects();
}

void RenderGraph::Compile() {
  AddPass("Present",
          [this](RenderGraphBuilder& builder) {
            builder.Read(mutable_backbuffer_);
            builder.UseRenderTarget(mutable_backbuffer_);
          },
          [this](RenderContext* context) {});

  builder_.Build(passes_);
}

void RenderGraph::BeginFrame() {
  cache_.Reset();
  builder_.Reset();
  passes_.clear();
  AquireBackbuffer();
}

void RenderGraph::Render() {
  // (This should be removed) If the CPU is ahead, wait on fences.
  Renderer::WaitForFences(&backbuffer_fences_[current_frame_], 1, true,
                          std::numeric_limits<uint64_t>::max());
  Renderer::ResetFences(&backbuffer_fences_[current_frame_], 1);

  // Submit work from the renderer passes.
  Renderer::Semaphore render_complete_semaphore =
      ExecuteRenderPasses(present_semaphores_[current_frame_]);

  // Present.
  Renderer::PresentInfo present_info;
  if (render_complete_semaphore != Renderer::kInvalidHandle) {
    present_info.wait_semaphores = &render_complete_semaphore;
    present_info.wait_semaphores_count = 1;
  } else {
    present_info.wait_semaphores_count = 0;
  }

  Renderer::QueuePresent(swapchain_, current_backbuffer_image_, present_info);

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight;
}

void RenderGraph::AddPass(const std::string& name, RenderGraphSetupFn setup_fn,
                          RenderGraphRenderFn render_fn) {
  RenderGraphPass pass;
  pass.name = name;
  pass.fn = std::move(render_fn);
  pass.setup = std::move(setup_fn);

  builder_.SetCurrentPass(passes_.size());
  if (pass.setup) {
    pass.setup(builder_);
  }
  passes_.emplace_back(std::move(pass));
}

Renderer::Semaphore RenderGraph::ExecuteRenderPasses(
    Renderer::Semaphore semaphore) {
  Renderer::Semaphore signal_semaphore = Renderer::kInvalidHandle;

  Renderer::SubmitInfo submit_info;
  RenderContext context;
  size_t count = 0;
  for (auto& pass : passes_) {
    if (pass.cmd == Renderer::kInvalidHandle) {
      continue;
    }

    CreateRenderContextForPass(&context, &pass);
    Renderer::CmdBegin(context.cmd);
    if (pass.render_pass != Renderer::kInvalidHandle &&
        pass.framebuffer != Renderer::kInvalidHandle) {
      Renderer::CmdBeginRenderPass(context.cmd, context.pass,
                                   context.framebuffer);
      pass.fn(&context);
      Renderer::CmdEndRenderPass(context.cmd);
    } else {
      pass.fn(&context);
    }
    Renderer::CmdEnd(context.cmd);

    // If this is the first pass, add the wait semaphore.
    if (semaphore != Renderer::kInvalidHandle) {
      pass.wait_semaphores.emplace_back(semaphore);
      semaphore = Renderer::kInvalidHandle;
    }

    // If this is the last pass, add a fence and semaphore.
    ++count;
    Renderer::Fence fence = Renderer::kInvalidHandle;
    if (count == passes_.size()) {
      fence = backbuffer_fences_[current_frame_];
      signal_semaphore = cache_.AllocateSemaphore();
      pass.signal_semaphores.emplace_back(signal_semaphore);
    }

    // Submit the pass.
    submit_info.signal_semaphores = pass.signal_semaphores.data();
    submit_info.signal_semaphores_count = pass.signal_semaphores.size();
    submit_info.wait_semaphores = pass.wait_semaphores.data();
    submit_info.wait_semaphores_count = pass.wait_semaphores.size();
    submit_info.command_buffers = &context.cmd;
    submit_info.command_buffers_count = 1;
    Renderer::QueueSubmit(device_, submit_info, fence);
  }
  return signal_semaphore;
}

void RenderGraph::CreateRenderContextForPass(RenderContext* context,
                                             const RenderGraphPass* pass) {
  context->cmd = pass->cmd;
  context->framebuffer = pass->framebuffer;
  context->pass = pass->render_pass;
}

Renderer::SwapChain RenderGraph::GetSwapChain() const { return swapchain_; }

void RenderGraph::AquireBackbuffer() {
  // Prepare the next swapchain frame buffer for rendering.
  uint32_t image_index;
  Renderer::AcquireNextImage(swapchain_, std::numeric_limits<uint64_t>::max(),
                             present_semaphores_[current_frame_], &image_index);
  current_backbuffer_image_ = image_index;

  RenderGraphFramebuffer buffer;
  buffer.pass = backbuffer_render_pass_;
  buffer.framebuffer = backbuffer_framebuffers_[image_index];
  mutable_backbuffer_ = cache_.AddFramebuffer(buffer);
}

RenderGraphMutableResource RenderGraph::GetBackbufferFramebuffer() const {
  return mutable_backbuffer_;
}

void RenderGraph::CreateSyncObjects() {
  present_semaphores_.resize(max_frames_in_flight);
  backbuffer_fences_.resize(max_frames_in_flight);

  for (auto& it : present_semaphores_) {
    it = Renderer::CreateSemaphore(device_);
  }
  for (auto& it : backbuffer_fences_) {
    it = Renderer::CreateFence(device_, true);
  }
}

void RenderGraph::DestroySyncObjects() {
  for (auto it : present_semaphores_) {
    Renderer::DestroySemaphore(it);
  }
  for (auto it : backbuffer_fences_) {
    Renderer::DestroyFence(it);
  }
  present_semaphores_.clear();
  backbuffer_fences_.clear();
}

/*void RenderGraph::CompileRenderPass(RenderGraphPass* pass) {
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
*/