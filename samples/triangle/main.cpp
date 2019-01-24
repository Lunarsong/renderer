#include <chrono>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <renderer/renderer.h>
#include "samples/common/util.h"

void CreateVkSurfance(Renderer::Instance instance, GLFWwindow* window);
Renderer::GraphicsPipeline CreatePipeline(Renderer::Device device,
                                          Renderer::RenderPass pass,
                                          Renderer::PipelineLayout layout);
GLFWwindow* InitWindow();
void Shutdown(GLFWwindow* window);

void Run() {
  std::cout << "Hello Vulkan" << std::endl;

  // Create a window.
  GLFWwindow* window = InitWindow();

  // Create the renderer instance (in this case Vulkan).
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  Renderer::Instance instance =
      Renderer::Create(glfwExtensions, glfwExtensionCount);

  // Hack. Figure out how to hide this. Create a surface.
  CreateVkSurfance(instance, window);

  // Create a device and swapchain.
  Renderer::Device device = Renderer::CreateDevice(instance);
  Renderer::SwapChain swapchain = Renderer::CreateSwapChain(device, 1980, 1200);

  // Create the render pass and frame buffers.
  Renderer::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(1);
  pass_info.attachments[0].format =
      Renderer::GetSwapChainImageFormat(swapchain);
  pass_info.attachments[0].load_op = Renderer::AttachmentLoadOp::kClear;
  pass_info.attachments[0].store_op = Renderer::AttachmentStoreOp::kStore;
  pass_info.attachments[0].final_layout =
      Renderer::ImageLayout::kPresentSrcKHR;
  Renderer::RenderPass pass = Renderer::CreateRenderPass(device, pass_info);
  const uint32_t swapchain_length = Renderer::GetSwapChainLength(swapchain);
  std::vector<Renderer::Framebuffer> framebuffers(swapchain_length);
  for (uint32_t i = 0; i < swapchain_length; ++i) {
    Renderer::FramebufferCreateInfo info;
    info.pass = pass;
    info.width = 1980;
    info.height = 1200;
    info.attachments.push_back(Renderer::GetSwapChainImageView(swapchain, i));
    framebuffers[i] = Renderer::CreateFramebuffer(device, info);
  }

  Renderer::DescriptorSetLayoutCreateInfo descriptor_layout_info = {
      {{Renderer::DescriptorType::kUniformBuffer, 1, Renderer::ShaderStageFlagBits::kVertexBit},
       {Renderer::DescriptorType::kUniformBuffer, 1, Renderer::ShaderStageFlagBits::kFragmentBit},
       {Renderer::DescriptorType::kCombinedImageSampler, 1,
        Renderer::ShaderStageFlagBits::kFragmentBit}}};
  Renderer::DescriptorSetLayout descriptor_layout =
      Renderer::CreateDescriptorSetLayout(device, descriptor_layout_info);

  // Create a pipeline.
  Renderer::PipelineLayout pipeline_layout =
      Renderer::CreatePipelineLayout(device, {{descriptor_layout}});
  Renderer::GraphicsPipeline pipeline =
      CreatePipeline(device, pass, pipeline_layout);

  Renderer::CreateDescriptorSetPoolCreateInfo pool_info = {
      {{Renderer::DescriptorType::kUniformBuffer,
        static_cast<uint32_t>(framebuffers.size()) * 2},
       {Renderer::DescriptorType::kCombinedImageSampler,
        static_cast<uint32_t>(framebuffers.size())}},
      /*max_sets=*/static_cast<uint32_t>(framebuffers.size())};
  Renderer::DescriptorSetPool set_pool =
      Renderer::CreateDescriptorSetPool(device, pool_info);

  std::vector<Renderer::DescriptorSet> descriptor_sets(framebuffers.size());
  Renderer::AllocateDescriptorSets(set_pool,
                                   std::vector<Renderer::DescriptorSetLayout>(
                                       framebuffers.size(), descriptor_layout),
                                   descriptor_sets.data());

  // Create a command pool.
  Renderer::CommandPool command_pool = Renderer::CreateCommandPool(device);

  // Create the vertex and index buffers. Copy the data using a staging buffer.

  std::vector<float> quad = {-0.5, -0.5, 0.0, 0.0, 1.0, 0.0, 0.0,  //
                             0.5,  0.5,  1.0, 1.0, 0.0, 0.0, 1.0,  //
                             -0.5, 0.5,  0.0, 1.0, 0.0, 1.0, 0.0,  //
                             0.5,  -0.5, 1.0, 0.0, 1.0, 1.0, 1.0};
  Renderer::Buffer vertex_buffer = Renderer::CreateBuffer(
      device, Renderer::BufferType::kVertex, sizeof(float) * 4 * 7,
      Renderer::MemoryUsage::kGpu);
  Renderer::StageCopyDataToBuffer(command_pool, vertex_buffer, quad.data(),
                                  sizeof(float) * 4 * 7);

  // Create the index buffer in GPU memory and copy the data.
  const uint32_t indices[] = {0, 1, 2, 3, 1, 0};
  Renderer::Buffer index_buffer =
      Renderer::CreateBuffer(device, Renderer::BufferType::kIndex,
                             sizeof(uint32_t) * 6, Renderer::MemoryUsage::kGpu);
  Renderer::StageCopyDataToBuffer(command_pool, index_buffer, indices,
                                  sizeof(uint32_t) * 6);

  float offsets[] = {0.5f, 0.0f, 0.0f, 0.0f};
  Renderer::Buffer uniform_buffer_offset = Renderer::CreateBuffer(
      device, Renderer::BufferType::kUniform, sizeof(float) * 4);
  memcpy(Renderer::MapBuffer(uniform_buffer_offset), offsets,
         sizeof(float) * 4);
  Renderer::UnmapBuffer(uniform_buffer_offset);

  float color[] = {0.5f, 0.5f, 0.5f, 1.0f};
  Renderer::Buffer uniform_buffer_color = Renderer::CreateBuffer(
      device, Renderer::BufferType::kUniform, sizeof(float) * 4);
  memcpy(Renderer::MapBuffer(uniform_buffer_color), color, sizeof(float) * 4);
  Renderer::UnmapBuffer(uniform_buffer_color);

  Renderer::Image image =
      Renderer::CreateImage(device, {Renderer::TextureType::Texture2D,
                                     Renderer::TextureFormat::kR8G8B8A8_UNORM,
                                     Renderer::Extent3D(2, 2, 1)});
  unsigned char pixels[] = {255, 255, 255, 255, 0,   0,   0,   255,
                            0,   0,   0,   255, 255, 255, 255, 255};
  Renderer::BufferImageCopy image_copy(0, 0, 0, 2, 2, 1);
  Renderer::StageCopyDataToImage(command_pool, image, pixels, 2 * 2 * 4,
                                 image_copy);
  Renderer::ImageViewCreateInfo image_view_info(
      image, Renderer::ImageViewType::Texture2D,
      Renderer::TextureFormat::kR8G8B8A8_UNORM,
      Renderer::ImageSubresourceRange(
          Renderer::ImageAspectFlagBits::kColorBit));
  Renderer::ImageView image_view =
      Renderer::CreateImageView(device, image_view_info);
  Renderer::Sampler sampler = Renderer::CreateSampler(device);

  for (auto it : descriptor_sets) {
    Renderer::WriteDescriptorSet write[3];
    Renderer::DescriptorBufferInfo offset_buffer;
    offset_buffer.buffer = uniform_buffer_offset;
    offset_buffer.range = sizeof(float) * 4;
    write[0].set = it;
    write[0].binding = 0;
    write[0].buffers = &offset_buffer;
    write[0].descriptor_count = 1;
    write[0].type = Renderer::DescriptorType::kUniformBuffer;

    Renderer::DescriptorBufferInfo color_buffer;
    color_buffer.buffer = uniform_buffer_color;
    color_buffer.range = sizeof(float) * 4;
    write[1].set = it;
    write[1].binding = 1;
    write[1].buffers = &color_buffer;
    write[1].descriptor_count = 1;
    write[1].type = Renderer::DescriptorType::kUniformBuffer;

    Renderer::DescriptorImageInfo image_info;
    image_info.image_view = image_view;
    image_info.sampler = sampler;
    write[2].set = it;
    write[2].binding = 2;
    write[2].descriptor_count = 1;
    write[2].type = Renderer::DescriptorType::kCombinedImageSampler;
    write[2].images = &image_info;

    Renderer::UpdateDescriptorSets(device, 3, write);
  }

  // Command buffers for every image in the swapchain.
  std::vector<Renderer::CommandBuffer> cmd_buffers(swapchain_length);
  int i = 0;
  for (auto& it : cmd_buffers) {
    it = Renderer::CreateCommandBuffer(command_pool);
    Renderer::CmdBegin(it);
    Renderer::ClearValue clear_value;
    Renderer::CmdBeginRenderPass(
        it,
        Renderer::BeginRenderPassInfo(pass, framebuffers[i], 1, &clear_value));
    Renderer::CmdBindPipeline(it, pipeline);
    Renderer::CmdBindVertexBuffers(it, 0, 1, &vertex_buffer);
    Renderer::CmdBindIndexBuffer(it, index_buffer,
                                 Renderer::IndexType::kUInt32);
    Renderer::CmdBindDescriptorSets(it, 0, pipeline_layout, 0, 1,
                                    &descriptor_sets[i]);
    Renderer::CmdDrawIndexed(it, 6);
    Renderer::CmdEndRenderPass(it);
    Renderer::CmdEnd(it);
    ++i;
  }

  // Allow drawing 2 frames at once, synchronized by their own semaphores and
  // fences.
  static constexpr size_t kMaxFramesInFlight = 2;
  std::vector<Renderer::Semaphore> image_available_semaphores(
      kMaxFramesInFlight);
  for (auto& it : image_available_semaphores) {
    it = Renderer::CreateSemaphore(device);
  }
  std::vector<Renderer::Semaphore> render_finished_semaphores(
      kMaxFramesInFlight);
  for (auto& it : render_finished_semaphores) {
    it = Renderer::CreateSemaphore(device);
  }
  std::vector<Renderer::Fence> in_flight_fences(kMaxFramesInFlight);
  for (auto& it : in_flight_fences) {
    it = Renderer::CreateFence(device, true);
  }

  // Main loop.
  size_t current_frame = 0;
  std::chrono::high_resolution_clock::time_point time =
      std::chrono::high_resolution_clock::now();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Get the delta time.
    std::chrono::high_resolution_clock::time_point current_time =
        std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = current_time - time;
    time = current_time;
    double delta_seconds = elapsed_time.count();

    // If the CPU is ahead, wait on fences.
    Renderer::WaitForFences(&in_flight_fences[current_frame], 1, true,
                            std::numeric_limits<uint64_t>::max());
    Renderer::ResetFences(&in_flight_fences[current_frame], 1);

    // Prepare the next swapchain frame buffer for rendering.
    uint32_t imageIndex;
    Renderer::AcquireNextImage(swapchain, std::numeric_limits<uint64_t>::max(),
                               image_available_semaphores[current_frame],
                               &imageIndex);

    // Set the semaphores and command buffer for the current frame buffer.
    Renderer::SubmitInfo submit_info;
    submit_info.wait_semaphores = &image_available_semaphores[current_frame];
    submit_info.wait_semaphores_count = 1;
    submit_info.command_buffers = &cmd_buffers[imageIndex];
    submit_info.command_buffers_count = 1;
    submit_info.signal_semaphores = &render_finished_semaphores[current_frame];
    submit_info.signal_semaphores_count = 1;
    Renderer::QueueSubmit(device, submit_info, in_flight_fences[current_frame]);

    // Present.
    Renderer::PresentInfo present_info;
    present_info.wait_semaphores = &render_finished_semaphores[current_frame];
    present_info.wait_semaphores_count = 1;
    Renderer::QueuePresent(swapchain, imageIndex, present_info);
    current_frame = (current_frame + 1) % kMaxFramesInFlight;
  }

  // Shutdown: Destroy everything.
  Renderer::DeviceWaitIdle(device);
  Renderer::DestroyCommandPool(command_pool);
  for (auto it : framebuffers) {
    Renderer::DestroyFramebuffer(it);
  }

  for (auto& it : in_flight_fences) {
    Renderer::DestroyFence(it);
  }
  for (auto it : image_available_semaphores) {
    Renderer::DestroySemaphore(it);
  }
  for (auto it : render_finished_semaphores) {
    Renderer::DestroySemaphore(it);
  }
  Renderer::DestroyDescriptorSetPool(set_pool);
  Renderer::DestroyImageView(device, image_view);
  Renderer::DestroyImage(image);
  Renderer::DestroyBuffer(uniform_buffer_color);
  Renderer::DestroyBuffer(uniform_buffer_offset);
  Renderer::DestroyBuffer(index_buffer);
  Renderer::DestroyBuffer(vertex_buffer);
  Renderer::DestroySampler(device, sampler);
  Renderer::DestroyGraphicsPipeline(pipeline);
  Renderer::DestroyPipelineLayout(device, pipeline_layout);
  Renderer::DestroyDescriptorSetLayout(descriptor_layout);
  Renderer::DestroyRenderPass(pass);
  Renderer::DestroySwapChain(swapchain);
  Renderer::DestroyDevice(device);
  Renderer::Destroy(instance);
  Shutdown(window);
}

int main() {
  try {
    Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return 0;
}

// Support functions.
namespace Renderer {
extern VkInstance GetVkInstance(Instance instance);
void SetSurface(Instance instance, VkSurfaceKHR surface);
}  // namespace Renderer

void CreateVkSurfance(Renderer::Instance instance, GLFWwindow* window) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(Renderer::GetVkInstance(instance), window,
                              nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
  Renderer::SetSurface(instance, surface);
}

Renderer::GraphicsPipeline CreatePipeline(Renderer::Device device,
                                          Renderer::RenderPass pass,
                                          Renderer::PipelineLayout layout) {
  Renderer::GraphicsPipeline pipeline = Renderer::kInvalidHandle;

  Renderer::GraphicsPipelineCreateInfo info;
  auto vert = util::ReadFile("samples/triangle/data/triangle.vert.spv");
  auto frag = util::ReadFile("samples/triangle/data/triangle.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input.resize(1);
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec2, 0});
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec2, sizeof(float) * 2});
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec3, sizeof(float) * 4});
  info.layout = layout;

  info.states.viewport.viewports.emplace_back(
      Renderer::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));
  info.states.blend.attachments.resize(1);
  info.states.rasterization.front_face = Renderer::FrontFace::kClockwise;
  pipeline = Renderer::CreateGraphicsPipeline(device, pass, info);

  return pipeline;
}

GLFWwindow* InitWindow() {
  if (!glfwInit()) {
    return nullptr;
  }
  if (!glfwVulkanSupported()) {
    std::cout << "Vulkan is not supported." << std::endl;
    glfwTerminate();
    return nullptr;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow* window =
      glfwCreateWindow(1980, 1200, "Vulkan window", nullptr, nullptr);
  return window;
}

void Shutdown(GLFWwindow* window) {
  glfwDestroyWindow(window);
  glfwTerminate();
}