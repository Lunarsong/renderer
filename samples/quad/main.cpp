#include <chrono>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <renderer/renderer.h>
#include "samples/common/util.h"

void CreateVkSurfance(RenderAPI::Instance instance, GLFWwindow* window);
RenderAPI::GraphicsPipeline CreatePipeline(RenderAPI::Device device,
                                           RenderAPI::RenderPass pass,
                                           RenderAPI::PipelineLayout layout);
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
  RenderAPI::Instance instance =
      RenderAPI::Create(glfwExtensions, glfwExtensionCount);

  // Hack. Figure out how to hide this. Create a surface.
  CreateVkSurfance(instance, window);

  // Create a device and swapchain.
  RenderAPI::Device device = RenderAPI::CreateDevice(instance);
  RenderAPI::SwapChain swapchain =
      RenderAPI::CreateSwapChain(device, 1980, 1200);

  // Create the render pass and frame buffers.
  RenderAPI::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(1);
  pass_info.attachments[0].format =
      RenderAPI::GetSwapChainImageFormat(swapchain);
  pass_info.attachments[0].load_op = RenderAPI::AttachmentLoadOp::kClear;
  pass_info.attachments[0].store_op = RenderAPI::AttachmentStoreOp::kStore;
  pass_info.attachments[0].final_layout =
      RenderAPI::ImageLayout::kPresentSrcKHR;
  RenderAPI::RenderPass pass = RenderAPI::CreateRenderPass(device, pass_info);
  const uint32_t swapchain_length = RenderAPI::GetSwapChainLength(swapchain);
  std::vector<RenderAPI::Framebuffer> framebuffers(swapchain_length);
  for (uint32_t i = 0; i < swapchain_length; ++i) {
    RenderAPI::FramebufferCreateInfo info;
    info.pass = pass;
    info.width = 1980;
    info.height = 1200;
    info.attachments.push_back(RenderAPI::GetSwapChainImageView(swapchain, i));
    framebuffers[i] = RenderAPI::CreateFramebuffer(device, info);
  }

  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_layout_info = {
      {{RenderAPI::DescriptorType::kUniformBuffer, 1,
        RenderAPI::ShaderStageFlagBits::kVertexBit},
       {RenderAPI::DescriptorType::kUniformBuffer, 1,
        RenderAPI::ShaderStageFlagBits::kFragmentBit},
       {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
        RenderAPI::ShaderStageFlagBits::kFragmentBit}}};
  RenderAPI::DescriptorSetLayout descriptor_layout =
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info);

  // Create a pipeline.
  RenderAPI::PipelineLayout pipeline_layout =
      RenderAPI::CreatePipelineLayout(device, {{descriptor_layout}});
  RenderAPI::GraphicsPipeline pipeline =
      CreatePipeline(device, pass, pipeline_layout);

  RenderAPI::CreateDescriptorSetPoolCreateInfo pool_info = {
      {{RenderAPI::DescriptorType::kUniformBuffer,
        static_cast<uint32_t>(framebuffers.size()) * 2},
       {RenderAPI::DescriptorType::kCombinedImageSampler,
        static_cast<uint32_t>(framebuffers.size())}},
      /*max_sets=*/static_cast<uint32_t>(framebuffers.size())};
  RenderAPI::DescriptorSetPool set_pool =
      RenderAPI::CreateDescriptorSetPool(device, pool_info);

  std::vector<RenderAPI::DescriptorSet> descriptor_sets(framebuffers.size());
  RenderAPI::AllocateDescriptorSets(set_pool,
                                    std::vector<RenderAPI::DescriptorSetLayout>(
                                        framebuffers.size(), descriptor_layout),
                                    descriptor_sets.data());

  // Create a command pool.
  RenderAPI::CommandPool command_pool = RenderAPI::CreateCommandPool(device);

  // Create the vertex and index buffers. Copy the data using a staging buffer.

  std::vector<float> quad = {-0.5, -0.5, 0.0, 0.0, 1.0, 0.0, 0.0,  //
                             0.5,  0.5,  1.0, 1.0, 0.0, 0.0, 1.0,  //
                             -0.5, 0.5,  0.0, 1.0, 0.0, 1.0, 0.0,  //
                             0.5,  -0.5, 1.0, 0.0, 1.0, 1.0, 1.0};
  RenderAPI::Buffer vertex_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kVertex, sizeof(float) * 4 * 7,
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, vertex_buffer, quad.data(),
                                   sizeof(float) * 4 * 7);

  // Create the index buffer in GPU memory and copy the data.
  const uint32_t indices[] = {0, 1, 2, 3, 1, 0};
  RenderAPI::Buffer index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kIndex, sizeof(uint32_t) * 6,
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, index_buffer, indices,
                                   sizeof(uint32_t) * 6);

  float offsets[] = {0.5f, 0.0f, 0.0f, 0.0f};
  RenderAPI::Buffer uniform_buffer_offset = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kUniform, sizeof(float) * 4);
  memcpy(RenderAPI::MapBuffer(uniform_buffer_offset), offsets,
         sizeof(float) * 4);
  RenderAPI::UnmapBuffer(uniform_buffer_offset);

  float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
  RenderAPI::Buffer uniform_buffer_color = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kUniform, sizeof(float) * 4);
  memcpy(RenderAPI::MapBuffer(uniform_buffer_color), color, sizeof(float) * 4);
  RenderAPI::UnmapBuffer(uniform_buffer_color);

  RenderAPI::Image image =
      RenderAPI::CreateImage(device, {RenderAPI::TextureType::Texture2D,
                                      RenderAPI::TextureFormat::kR8G8B8A8_UNORM,
                                      RenderAPI::Extent3D(2, 2, 1)});
  unsigned char pixels[] = {255, 0, 0,   255, 0,   255, 0,   255,
                            0,   0, 255, 255, 255, 255, 255, 255};
  RenderAPI::BufferImageCopy image_copy(0, 0, 0, 2, 2, 1);
  RenderAPI::StageCopyDataToImage(command_pool, image, pixels, 2 * 2 * 4,
                                  image_copy);
  RenderAPI::ImageViewCreateInfo image_view_info(
      image, RenderAPI::ImageViewType::Texture2D,
      RenderAPI::TextureFormat::kR8G8B8A8_UNORM,
      RenderAPI::ImageSubresourceRange(
          RenderAPI::ImageAspectFlagBits::kColorBit));
  RenderAPI::ImageView image_view =
      RenderAPI::CreateImageView(device, image_view_info);
  RenderAPI::Sampler sampler = RenderAPI::CreateSampler(device);

  for (auto it : descriptor_sets) {
    RenderAPI::WriteDescriptorSet write[3];
    RenderAPI::DescriptorBufferInfo offset_buffer;
    offset_buffer.buffer = uniform_buffer_offset;
    offset_buffer.range = sizeof(float) * 4;
    write[0].set = it;
    write[0].binding = 0;
    write[0].buffers = &offset_buffer;
    write[0].descriptor_count = 1;
    write[0].type = RenderAPI::DescriptorType::kUniformBuffer;

    RenderAPI::DescriptorBufferInfo color_buffer;
    color_buffer.buffer = uniform_buffer_color;
    color_buffer.range = sizeof(float) * 4;
    write[1].set = it;
    write[1].binding = 1;
    write[1].buffers = &color_buffer;
    write[1].descriptor_count = 1;
    write[1].type = RenderAPI::DescriptorType::kUniformBuffer;

    RenderAPI::DescriptorImageInfo image_info;
    image_info.image_view = image_view;
    image_info.sampler = sampler;
    write[2].set = it;
    write[2].binding = 2;
    write[2].descriptor_count = 1;
    write[2].type = RenderAPI::DescriptorType::kCombinedImageSampler;
    write[2].images = &image_info;

    RenderAPI::UpdateDescriptorSets(device, 3, write);
  }

  // Command buffers for every image in the swapchain.
  std::vector<RenderAPI::CommandBuffer> cmd_buffers(swapchain_length);
  int i = 0;
  for (auto& it : cmd_buffers) {
    it = RenderAPI::CreateCommandBuffer(command_pool);
    RenderAPI::CmdBegin(it);
    RenderAPI::ClearValue clear_value;
    RenderAPI::CmdBeginRenderPass(
        it,
        RenderAPI::BeginRenderPassInfo(pass, framebuffers[i], 1, &clear_value));
    RenderAPI::CmdBindPipeline(it, pipeline);
    RenderAPI::CmdBindVertexBuffers(it, 0, 1, &vertex_buffer);
    RenderAPI::CmdBindIndexBuffer(it, index_buffer,
                                  RenderAPI::IndexType::kUInt32);
    RenderAPI::CmdBindDescriptorSets(it, 0, pipeline_layout, 0, 1,
                                     &descriptor_sets[i]);
    RenderAPI::CmdDrawIndexed(it, 6);
    RenderAPI::CmdEndRenderPass(it);
    RenderAPI::CmdEnd(it);
    ++i;
  }

  // Allow drawing 2 frames at once, synchronized by their own semaphores and
  // fences.
  static constexpr size_t kMaxFramesInFlight = 2;
  std::vector<RenderAPI::Semaphore> image_available_semaphores(
      kMaxFramesInFlight);
  for (auto& it : image_available_semaphores) {
    it = RenderAPI::CreateSemaphore(device);
  }
  std::vector<RenderAPI::Semaphore> render_finished_semaphores(
      kMaxFramesInFlight);
  for (auto& it : render_finished_semaphores) {
    it = RenderAPI::CreateSemaphore(device);
  }
  std::vector<RenderAPI::Fence> in_flight_fences(kMaxFramesInFlight);
  for (auto& it : in_flight_fences) {
    it = RenderAPI::CreateFence(device, true);
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
    RenderAPI::WaitForFences(&in_flight_fences[current_frame], 1, true,
                             std::numeric_limits<uint64_t>::max());
    RenderAPI::ResetFences(&in_flight_fences[current_frame], 1);

    // Prepare the next swapchain frame buffer for rendering.
    uint32_t imageIndex;
    RenderAPI::AcquireNextImage(swapchain, std::numeric_limits<uint64_t>::max(),
                                image_available_semaphores[current_frame],
                                &imageIndex);

    // Set the semaphores and command buffer for the current frame buffer.
    RenderAPI::SubmitInfo submit_info;
    submit_info.wait_semaphores = &image_available_semaphores[current_frame];
    submit_info.wait_semaphores_count = 1;
    submit_info.command_buffers = &cmd_buffers[imageIndex];
    submit_info.command_buffers_count = 1;
    submit_info.signal_semaphores = &render_finished_semaphores[current_frame];
    submit_info.signal_semaphores_count = 1;
    RenderAPI::QueueSubmit(device, submit_info,
                           in_flight_fences[current_frame]);

    // Present.
    RenderAPI::PresentInfo present_info;
    present_info.wait_semaphores = &render_finished_semaphores[current_frame];
    present_info.wait_semaphores_count = 1;
    RenderAPI::QueuePresent(swapchain, imageIndex, present_info);
    current_frame = (current_frame + 1) % kMaxFramesInFlight;
  }

  // Shutdown: Destroy everything.
  RenderAPI::DeviceWaitIdle(device);
  RenderAPI::DestroyCommandPool(command_pool);
  for (auto it : framebuffers) {
    RenderAPI::DestroyFramebuffer(it);
  }

  for (auto& it : in_flight_fences) {
    RenderAPI::DestroyFence(it);
  }
  for (auto it : image_available_semaphores) {
    RenderAPI::DestroySemaphore(it);
  }
  for (auto it : render_finished_semaphores) {
    RenderAPI::DestroySemaphore(it);
  }
  RenderAPI::DestroyDescriptorSetPool(set_pool);
  RenderAPI::DestroyImageView(device, image_view);
  RenderAPI::DestroyImage(image);
  RenderAPI::DestroyBuffer(uniform_buffer_color);
  RenderAPI::DestroyBuffer(uniform_buffer_offset);
  RenderAPI::DestroyBuffer(index_buffer);
  RenderAPI::DestroyBuffer(vertex_buffer);
  RenderAPI::DestroySampler(device, sampler);
  RenderAPI::DestroyGraphicsPipeline(pipeline);
  RenderAPI::DestroyPipelineLayout(device, pipeline_layout);
  RenderAPI::DestroyDescriptorSetLayout(descriptor_layout);
  RenderAPI::DestroyRenderPass(pass);
  RenderAPI::DestroySwapChain(swapchain);
  RenderAPI::DestroyDevice(device);
  RenderAPI::Destroy(instance);
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
namespace RenderAPI {
extern VkInstance GetVkInstance(Instance instance);
void SetSurface(Instance instance, VkSurfaceKHR surface);
}  // namespace RenderAPI

void CreateVkSurfance(RenderAPI::Instance instance, GLFWwindow* window) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(RenderAPI::GetVkInstance(instance), window,
                              nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
  RenderAPI::SetSurface(instance, surface);
}

RenderAPI::GraphicsPipeline CreatePipeline(RenderAPI::Device device,
                                           RenderAPI::RenderPass pass,
                                           RenderAPI::PipelineLayout layout) {
  RenderAPI::GraphicsPipeline pipeline = RenderAPI::kInvalidHandle;

  RenderAPI::GraphicsPipelineCreateInfo info;
  auto vert = util::ReadFile("samples/quad/data/shader.vert.spv");
  auto frag = util::ReadFile("samples/quad/data/shader.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input.resize(1);
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec2, 0});
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec2, sizeof(float) * 2});
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec3, sizeof(float) * 4});
  info.layout = layout;

  info.states.viewport.viewports.emplace_back(
      RenderAPI::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));
  info.states.blend.attachments.resize(1);
  info.states.rasterization.front_face = RenderAPI::FrontFace::kClockwise;
  pipeline = RenderAPI::CreateGraphicsPipeline(device, pass, info);

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