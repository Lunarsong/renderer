#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "renderer.h"
#include "samples/common/util.h"

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
                                          Renderer::RenderPass pass) {
  Renderer::GraphicsPipeline pipeline = Renderer::kInvalidHandle;

  Renderer::GraphicsPipelineCreateInfo info;
  auto vert = util::ReadFile("samples/triangle/data/vert.spv");
  auto frag = util::ReadFile("samples/triangle/data/frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input.resize(1);
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec2, 0});
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec3, sizeof(float) * 2});

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

void Run() {
  std::cout << "Hello Vulkan" << std::endl;

  GLFWwindow* window = InitWindow();

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  Renderer::Instance instance =
      Renderer::Create(glfwExtensions, glfwExtensionCount);
  CreateVkSurfance(instance, window);
  Renderer::Device device = Renderer::CreateDevice(instance);
  Renderer::SwapChain swapchain = Renderer::CreateSwapChain(device, 1980, 1200);
  Renderer::RenderPass pass = Renderer::CreateRenderPass(device, swapchain);
  const uint32_t swapchain_length = Renderer::GetSwapChainLength(swapchain);
  std::vector<Renderer::Framebuffer> framebuffers(swapchain_length);
  for (uint32_t i = 0; i < swapchain_length; ++i) {
    framebuffers[i] = Renderer::CreateSwapChainFramebuffer(swapchain, i, pass);
  }
  std::vector<float> triangle = {0.0,  -0.5, 1.0, 0.0, 0.0,  //
                                 0.5,  0.5,  0.0, 1.0, 0.0,  //
                                 -0.5, 0.5,  0.0, 0.0, 1.0};
  Renderer::Buffer vertex_buffer = Renderer::CreateBuffer(
      device, Renderer::BufferType::kVertex, sizeof(float) * 3 * 5);
  memcpy(Renderer::MapBuffer(vertex_buffer), triangle.data(),
         sizeof(float) * 3 * 5);
  Renderer::UnmapBuffer(vertex_buffer);

  const uint32_t indices[] = {0, 1, 2};
  Renderer::Buffer index_buffer = Renderer::CreateBuffer(
      device, Renderer::BufferType::kIndex, sizeof(uint32_t) * 3);
  memcpy(Renderer::MapBuffer(index_buffer), indices, sizeof(uint32_t) * 3);
  Renderer::UnmapBuffer(index_buffer);

  Renderer::GraphicsPipeline pipeline = CreatePipeline(device, pass);
  Renderer::CommandPool command_pool = Renderer::CreateCommandPool(device);

  std::vector<Renderer::CommandBuffer> cmd_buffers(swapchain_length);
  int i = 0;
  for (auto& it : cmd_buffers) {
    it = Renderer::CreateCommandBuffer(command_pool);
    Renderer::CmdBegin(it);
    Renderer::CmdBeginRenderPass(it, pass, framebuffers[i]);
    Renderer::CmdBindPipeline(it, pipeline);
    Renderer::CmdBindVertexBuffers(it, 0, 1, &vertex_buffer);
    Renderer::CmdBindIndexBuffer(it, index_buffer,
                                 Renderer::IndexType::kUInt32);
    Renderer::CmdDrawIndexed(it, 3);
    Renderer::CmdEndRenderPass(it);
    Renderer::CmdEnd(it);
    ++i;
  }

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

  size_t current_frame = 0;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    Renderer::WaitForFences(&in_flight_fences[current_frame], 1, true,
                            std::numeric_limits<uint64_t>::max());
    Renderer::ResetFences(&in_flight_fences[current_frame], 1);

    uint32_t imageIndex;
    Renderer::AcquireNextImage(swapchain, std::numeric_limits<uint64_t>::max(),
                               image_available_semaphores[current_frame],
                               &imageIndex);

    Renderer::SubmitInfo submit_info;
    submit_info.wait_semaphores = &image_available_semaphores[current_frame];
    submit_info.wait_semaphores_count = 1;
    submit_info.command_buffers = &cmd_buffers[imageIndex];
    submit_info.command_buffers_count = 1;
    submit_info.signal_semaphores = &render_finished_semaphores[current_frame];
    submit_info.signal_semaphores_count = 1;
    Renderer::QueueSubmit(device, submit_info, in_flight_fences[current_frame]);

    Renderer::PresentInfo present_info;
    present_info.wait_semaphores = &render_finished_semaphores[current_frame];
    present_info.wait_semaphores_count = 1;
    Renderer::QueuePresent(swapchain, imageIndex, present_info);
    current_frame = (current_frame + 1) % kMaxFramesInFlight;
  }

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
  Renderer::DestroyBuffer(index_buffer);
  Renderer::DestroyBuffer(vertex_buffer);
  Renderer::DestroyGraphicsPipeline(pipeline);
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
