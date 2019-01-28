#include "render_graph.h"

#include <cassert>
#include <iostream>

RenderGraph::RenderGraph(RenderAPI::Device device)
    : device_(device), builder_(&cache_) {
  command_pool_ = RenderAPI::CreateCommandPool(
      device_, RenderAPI::CommandPoolCreateFlags::kResetCommand);
  cache_.SetRenderObjects(device_, command_pool_);
}

RenderGraph::~RenderGraph() { Destroy(); }

void RenderGraph::Destroy() {
  if (device_ == RenderAPI::kInvalidHandle) {
    return;
  }
  RenderAPI::DeviceWaitIdle(device_);

  cache_.Destroy();

  if (swapchain_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroySwapChain(swapchain_);
    swapchain_ = RenderAPI::kInvalidHandle;
  }
  if (command_pool_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyCommandPool(command_pool_);
    command_pool_ = RenderAPI::kInvalidHandle;
  }
  DestroySwapChain();
  device_ = RenderAPI::kInvalidHandle;
}

void RenderGraph::DestroySwapChain() {
  RenderAPI::DeviceWaitIdle(device_);
  DestroySyncObjects();

  if (backbuffer_render_pass_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyRenderPass(backbuffer_render_pass_);
    backbuffer_render_pass_ = RenderAPI::kInvalidHandle;
  }

  if (swapchain_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroySwapChain(swapchain_);
    swapchain_ = RenderAPI::kInvalidHandle;
  }
}

void RenderGraph::BuildSwapChain(uint32_t width, uint32_t height) {
  DestroySwapChain();

  swapchain_ = RenderAPI::CreateSwapChain(device_, width, height);
  const uint32_t swapchain_length = RenderAPI::GetSwapChainLength(swapchain_);
  max_frames_in_flight = swapchain_length;
  cache_.PrepareBufferedResources(max_frames_in_flight);

  swapchain_desc_.width = width;
  swapchain_desc_.height = height;
  swapchain_desc_.format = RenderAPI::GetSwapChainImageFormat(swapchain_);
  swapchain_desc_.load_op = RenderAPI::AttachmentLoadOp::kClear;
  swapchain_desc_.layout = RenderAPI::ImageLayout::kPresentSrcKHR;

  CreateSyncObjects();
}

std::vector<RenderGraphNode> RenderGraph::Compile() {
  AddPass("Present",
          [this](RenderGraphBuilder& builder) {
            builder.Read(backbuffer_resource_);
          },
          [this](RenderContext* context, const Scope& scope) {});

  return builder_.Build(passes_);
}

void RenderGraph::BeginFrame() {
  cache_.Reset();
  builder_.Reset();
  passes_.clear();
  AcquireBackbuffer();
}

void RenderGraph::Render() {
  // Compile the pass (should this be exposed?)
  std::vector<RenderGraphNode> nodes = Compile();

  // (This should be removed) If the CPU is ahead, wait on fences.
  RenderAPI::WaitForFences(&backbuffer_fences_[current_frame_], 1, true,
                           std::numeric_limits<uint64_t>::max());
  RenderAPI::ResetFences(&backbuffer_fences_[current_frame_], 1);

  // Submit work from the renderer passes.
  RenderAPI::Semaphore render_complete_semaphore =
      ExecuteRenderPasses(nodes, present_semaphores_[current_frame_]);

  // Present.
  RenderAPI::PresentInfo present_info;
  if (render_complete_semaphore != RenderAPI::kInvalidHandle) {
    present_info.wait_semaphores = &render_complete_semaphore;
    present_info.wait_semaphores_count = 1;
  } else {
    present_info.wait_semaphores_count = 0;
  }

  RenderAPI::QueuePresent(swapchain_, current_backbuffer_image_, present_info);

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

RenderAPI::Semaphore RenderGraph::ExecuteRenderPasses(
    std::vector<RenderGraphNode>& nodes, RenderAPI::Semaphore semaphore) {
  RenderAPI::Semaphore signal_semaphore = RenderAPI::kInvalidHandle;

  RenderAPI::SubmitInfo submit_info;
  RenderContext context;
  size_t count = 0;
  for (auto& node : nodes) {
    if (node.render_cmd == RenderAPI::kInvalidHandle) {
      continue;
    }

    context.cmd = node.render_cmd;
    RenderAPI::CmdBegin(context.cmd);

    for (auto& render_pass : node.render_passes) {
      if (render_pass.framebuffer.pass != RenderAPI::kInvalidHandle &&
          render_pass.framebuffer.framebuffer != RenderAPI::kInvalidHandle) {
        context.framebuffer = render_pass.framebuffer.framebuffer;
        context.pass = render_pass.framebuffer.pass;
        RenderAPI::CmdBeginRenderPass(
            context.cmd, RenderAPI::BeginRenderPassInfo(
                             context.pass, context.framebuffer,
                             static_cast<uint32_t>(
                                 render_pass.framebuffer.clear_values.size()),
                             render_pass.framebuffer.clear_values.data()));
        for (auto& pass : render_pass.passes) {
          pass->fn(&context, pass->scope);
        }
        RenderAPI::CmdEndRenderPass(context.cmd);
      }
    }
    RenderAPI::CmdEnd(context.cmd);

    // If this is the first node, add the wait semaphore.
    if (semaphore != RenderAPI::kInvalidHandle) {
      node.wait_semaphores.emplace_back(semaphore);
      semaphore = RenderAPI::kInvalidHandle;
    }

    // If this is the last pass, add a fence and semaphore.
    ++count;
    RenderAPI::Fence fence = RenderAPI::kInvalidHandle;
    if (count == nodes.size()) {
      fence = backbuffer_fences_[current_frame_];
      signal_semaphore = cache_.AllocateSemaphore();
      node.signal_semaphores.emplace_back(signal_semaphore);
    }

    // Submit the pass.
    submit_info.signal_semaphores = node.signal_semaphores.data();
    submit_info.signal_semaphores_count = node.signal_semaphores.size();
    submit_info.wait_semaphores = node.wait_semaphores.data();
    submit_info.wait_semaphores_count = node.wait_semaphores.size();
    submit_info.command_buffers = &context.cmd;
    submit_info.command_buffers_count = 1;
    RenderAPI::QueueSubmit(device_, submit_info, fence);
  }
  return signal_semaphore;
}

RenderAPI::SwapChain RenderGraph::GetSwapChain() const { return swapchain_; }

void RenderGraph::AcquireBackbuffer() {
  // Prepare the next swapchain frame buffer for rendering.
  uint32_t image_index;
  RenderAPI::AcquireNextImage(swapchain_, std::numeric_limits<uint64_t>::max(),
                              present_semaphores_[current_frame_],
                              &image_index);
  current_backbuffer_image_ = image_index;
  backbuffer_resource_ =
      ImportTexture(swapchain_desc_,
                    RenderAPI::GetSwapChainImageView(swapchain_, image_index));
}

RenderGraphResource RenderGraph::GetBackbufferResource() const {
  return backbuffer_resource_;
}

void RenderGraph::CreateSyncObjects() {
  present_semaphores_.resize(max_frames_in_flight);
  backbuffer_fences_.resize(max_frames_in_flight);

  for (auto& it : present_semaphores_) {
    it = RenderAPI::CreateSemaphore(device_);
  }
  for (auto& it : backbuffer_fences_) {
    it = RenderAPI::CreateFence(device_, true);
  }
}

void RenderGraph::DestroySyncObjects() {
  for (auto it : present_semaphores_) {
    RenderAPI::DestroySemaphore(it);
  }
  for (auto it : backbuffer_fences_) {
    RenderAPI::DestroyFence(it);
  }
  present_semaphores_.clear();
  backbuffer_fences_.clear();
}

RenderGraphResource RenderGraph::ImportTexture(RenderGraphTextureDesc desc,
                                               RenderAPI::ImageView texture) {
  RenderGraphBuilder::RenderGraphTextureResource resource;
  resource.transient = false;
  resource.desc = std::move(desc);
  resource.texture = texture;
  auto handle = builder_.handles_.Create();
  builder_.textures_[handle] = std::move(resource);
  return handle;
}

void RenderGraph::MoveSubresource(RenderGraphResource from,
                                  RenderGraphResource to) {
  builder_.aliases_[from] = to;
}

const RenderGraphTextureDesc& RenderGraph::GetSwapChainDescription() const {
  return swapchain_desc_;
}

RenderAPI::ImageView Scope::GetTexture(RenderGraphResource resource) const {
  const auto& it = textures_.find(resource);
  assert(it != textures_.cend());
  return it->second;
}