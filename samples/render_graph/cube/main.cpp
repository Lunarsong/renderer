#include <chrono>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <renderer/renderer.h>
#include "render_graph/render_graph.h"
#include "samples/common/util.h"

void CreateVkSurfance(Renderer::Instance instance, GLFWwindow* window);
Renderer::GraphicsPipeline CreatePipeline(Renderer::Device device,
                                          Renderer::RenderPass pass,
                                          Renderer::PipelineLayout layout);
GLFWwindow* InitWindow();
void Shutdown(GLFWwindow* window);

struct CubePass {
  Renderer::Buffer vertex_buffer;
  Renderer::Buffer index_buffer;
  Renderer::Buffer uniform_buffer_mvp;
  Renderer::Buffer uniform_buffer_color;

  Renderer::Image image;
  Renderer::ImageView image_view;
  Renderer::Sampler sampler;

  Renderer::RenderPass render_pass;

  Renderer::DescriptorSetLayout descriptor_layout;
  Renderer::DescriptorSetPool descriptor_set_pool;
  Renderer::DescriptorSet descriptor_set;

  Renderer::PipelineLayout pipeline_layout;
  Renderer::GraphicsPipeline pipeline;
};

void RenderCube(RenderContext* context, CubePass& pass);
void DestroyCubePass(Renderer::Device device, CubePass& pass);
void CreateCubePass(CubePass& pass, Renderer::Device device,
                    Renderer::CommandPool command_pool,
                    const RenderGraph& graph);

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
  Renderer::CommandPool command_pool = Renderer::CreateCommandPool(device);

  RenderGraph render_graph_(device);
  render_graph_.BuildSwapChain(1980, 1200);

  CubePass cube_pass;
  CreateCubePass(cube_pass, device, command_pool, render_graph_);

  std::chrono::high_resolution_clock::time_point time =
      std::chrono::high_resolution_clock::now();
  float rotation = 0.0f;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Get the delta time.
    std::chrono::high_resolution_clock::time_point current_time =
        std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = current_time - time;
    time = current_time;
    double delta_seconds = elapsed_time.count();
    rotation += static_cast<float>(delta_seconds);

    render_graph_.BeginFrame();
    render_graph_.AddPass(
        "Cube",
        [&](RenderGraphBuilder& builder) {
          builder.UseRenderTarget(render_graph_.GetBackbufferFramebuffer());
        },
        [&](RenderContext* context, const RenderGraphCache* cache) {
          glm::mat4 perspective = glm::perspectiveFov(
              glm::radians(45.0f), 1980.0f, 1200.0f, 0.1f, 100.0f);
          perspective[1][1] *= -1.0f;
          glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 2.5f, -5.0f),
                                       glm::vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
          glm::mat4 mvp = perspective * view *
                          glm::mat4_cast(glm::angleAxis(
                              rotation, glm::vec3(0.0f, 1.0f, 0.0f)));
          memcpy(Renderer::MapBuffer(cube_pass.uniform_buffer_mvp), &mvp,
                 sizeof(mvp));
          Renderer::UnmapBuffer(cube_pass.uniform_buffer_mvp);
          RenderCube(context, cube_pass);
        });
    render_graph_.Render();
  }

  render_graph_.Destroy();
  DestroyCubePass(device, cube_pass);

  Renderer::DestroyCommandPool(command_pool);
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
void RenderCube(RenderContext* context, CubePass& pass) {
  Renderer::CommandBuffer cmd = context->cmd;

  Renderer::CmdBindPipeline(cmd, pass.pipeline);
  Renderer::CmdBindVertexBuffers(cmd, 0, 1, &pass.vertex_buffer);
  Renderer::CmdBindIndexBuffer(cmd, pass.index_buffer,
                               Renderer::IndexType::kUInt32);
  Renderer::CmdBindDescriptorSets(cmd, 0, pass.pipeline_layout, 0, 1,
                                  &pass.descriptor_set);
  Renderer::CmdDrawIndexed(cmd, 6 * 6);
}

void DestroyCubePass(Renderer::Device device, CubePass& pass) {
  Renderer::DestroyDescriptorSetPool(pass.descriptor_set_pool);
  Renderer::DestroyDescriptorSetLayout(pass.descriptor_layout);

  Renderer::DestroySampler(device, pass.sampler);
  Renderer::DestroyImageView(device, pass.image_view);
  Renderer::DestroyImage(pass.image);

  Renderer::DestroyBuffer(pass.vertex_buffer);
  Renderer::DestroyBuffer(pass.index_buffer);
  Renderer::DestroyBuffer(pass.uniform_buffer_color);
  Renderer::DestroyBuffer(pass.uniform_buffer_mvp);

  Renderer::DestroyPipelineLayout(device, pass.pipeline_layout);
  Renderer::DestroyGraphicsPipeline(pass.pipeline);

  Renderer::DestroyRenderPass(pass.render_pass);
}

void CreateCubePass(CubePass& pass, Renderer::Device device,
                    Renderer::CommandPool command_pool,
                    const RenderGraph& graph) {
  // Create the cube.
  std::vector<float> cube = {
      // Font face.
      -0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0,  //
      0.5, 0.5, -0.5f, 1.0, 1.0, 1.0, 1.0, 1.0,    //
      -0.5, 0.5, -0.5f, 0.0, 1.0, 1.0, 1.0, 0.0,   //
      0.5, -0.5, -0.5f, 1.0, 0.0, 1.0, 1.0, 1.0,

      // Back face.
      0.5, -0.5, 0.5f, 0.0, 0.0, 1.0, 1.0, 1.0,  //
      -0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0,  //
      0.5, 0.5, 0.5f, 0.0, 1.0, 1.0, 1.0, 1.0,   //
      -0.5, -0.5, 0.5f, 1.0, 0.0, 1.0, 1.0, 1.0,

      // Left face.
      -0.5, 0.5, -0.5f, 0.0, 1.0, 1.0, 1.0, 1.0,   //
      -0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0,  //
      -0.5, -0.5, 0.5f, 1.0, 0.0, 1.0, 1.0, 1.0,   //
      -0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0,

      // Right face.
      0.5, 0.5, -0.5f, 0.0, 1.0, 1.0, 1.0, 1.0,   //
      0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0,  //
      0.5, -0.5, 0.5f, 1.0, 0.0, 1.0, 1.0, 1.0,   //
      0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0,

      // Bottom face.
      -0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 0.0, 0.0,  //
      0.5, -0.5, -0.5f, 1.0, 0.0, 0.0, 0.0, 1.0,   //
      -0.5, -0.5, 0.5f, 0.0, 1.0, 0.0, 1.0, 0.0,   //
      0.5, -0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0,

      // Top face.
      -0.5, 0.5, -0.5f, 0.0, 0.0, 1.0, 0.0, 0.0,  //
      0.5, 0.5, -0.5f, 1.0, 0.0, 0.0, 0.0, 1.0,   //
      -0.5, 0.5, 0.5f, 0.0, 1.0, 0.0, 1.0, 0.0,   //
      0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0     //
  };
  pass.vertex_buffer = Renderer::CreateBuffer(
      device, Renderer::BufferType::kVertex, sizeof(float) * cube.size(),
      Renderer::MemoryUsage::kGpu);
  Renderer::StageCopyDataToBuffer(command_pool, pass.vertex_buffer, cube.data(),
                                  sizeof(float) * cube.size());

  // Create the index buffer in GPU memory and copy the data.
  const std::vector<uint32_t> indices = {
      0,  1,  2,  3,  1,  0,   // Front face
      4,  5,  6,  7,  5,  4,   // Back face
      10, 9,  8,  8,  11, 10,  // Left face
      12, 13, 14, 14, 15, 12,  // Right face
      19, 17, 16, 16, 18, 19,  // Bottom face
      20, 21, 23, 23, 22, 20,  // Top face

  };
  pass.index_buffer = Renderer::CreateBuffer(
      device, Renderer::BufferType::kIndex, sizeof(uint32_t) * indices.size(),
      Renderer::MemoryUsage::kGpu);
  Renderer::StageCopyDataToBuffer(command_pool, pass.index_buffer,
                                  indices.data(),
                                  sizeof(uint32_t) * indices.size());

  pass.uniform_buffer_mvp = Renderer::CreateBuffer(
      device, Renderer::BufferType::kUniform, sizeof(glm::mat4));

  float color[] = {0.5f, 0.5f, 0.5f, 1.0f};
  pass.uniform_buffer_color = Renderer::CreateBuffer(
      device, Renderer::BufferType::kUniform, sizeof(float) * 4);
  memcpy(Renderer::MapBuffer(pass.uniform_buffer_color), color,
         sizeof(float) * 4);
  Renderer::UnmapBuffer(pass.uniform_buffer_color);

  // Create the image and sampler.
  pass.image =
      Renderer::CreateImage(device, {Renderer::TextureType::Texture2D,
                                     Renderer::TextureFormat::kR8G8B8A8_UNORM,
                                     Renderer::Extent3D(2, 2, 1)});
  unsigned char pixels[] = {255, 255, 255, 255, 0,   0,   0,   255,
                            0,   0,   0,   255, 255, 255, 255, 255};
  Renderer::BufferImageCopy image_copy(0, 0, 0, 2, 2, 1);
  Renderer::StageCopyDataToImage(command_pool, pass.image, pixels, 2 * 2 * 4,
                                 image_copy);
  Renderer::ImageViewCreateInfo image_view_info(
      pass.image, Renderer::ImageViewType::Texture2D,
      Renderer::TextureFormat::kR8G8B8A8_UNORM,
      Renderer::ImageSubresourceRange(
          Renderer::ImageAspectFlagBits::kColorBit));
  pass.image_view = Renderer::CreateImageView(device, image_view_info);
  pass.sampler = Renderer::CreateSampler(device);

  // Create the pipeline.
  Renderer::DescriptorSetLayoutCreateInfo descriptor_layout_info(
      {{{Renderer::DescriptorType::kUniformBuffer, 1, Renderer::kVertexBit},
        {Renderer::DescriptorType::kUniformBuffer, 1, Renderer::kFragmentBit},
        {Renderer::DescriptorType::kCombinedImageSampler, 1,
         Renderer::kFragmentBit}}});
  pass.descriptor_layout =
      Renderer::CreateDescriptorSetLayout(device, descriptor_layout_info);

  Renderer::RenderPassCreateInfo pass_info;
  pass_info.color_attachments.resize(1);
  pass_info.color_attachments[0].final_layout =
      Renderer::ImageLayout::kPresentSrcKHR;
  pass_info.color_attachments[0].format =
      Renderer::GetSwapChainImageFormat(graph.GetSwapChain());
  pass.render_pass = Renderer::CreateRenderPass(device, pass_info);

  // Create a pipeline.
  pass.pipeline_layout =
      Renderer::CreatePipelineLayout(device, {{pass.descriptor_layout}});
  pass.pipeline =
      CreatePipeline(device, pass.render_pass, pass.pipeline_layout);

  // Create the descriptor sets.
  Renderer::CreateDescriptorSetPoolCreateInfo pool_info = {
      {{Renderer::DescriptorType::kUniformBuffer, 2},
       {Renderer::DescriptorType::kCombinedImageSampler, 1}},
      /*max_sets=*/1};
  pass.descriptor_set_pool =
      Renderer::CreateDescriptorSetPool(device, pool_info);

  Renderer::AllocateDescriptorSets(
      pass.descriptor_set_pool,
      std::vector<Renderer::DescriptorSetLayout>(1, pass.descriptor_layout),
      &pass.descriptor_set);

  Renderer::WriteDescriptorSet write[3];
  Renderer::DescriptorBufferInfo mvp_buffer;
  mvp_buffer.buffer = pass.uniform_buffer_mvp;
  mvp_buffer.range = sizeof(glm::mat4);
  write[0].set = pass.descriptor_set;
  write[0].binding = 0;
  write[0].buffers = &mvp_buffer;
  write[0].descriptor_count = 1;
  write[0].type = Renderer::DescriptorType::kUniformBuffer;

  Renderer::DescriptorBufferInfo color_buffer;
  color_buffer.buffer = pass.uniform_buffer_color;
  color_buffer.range = sizeof(float) * 4;
  write[1].set = pass.descriptor_set;
  write[1].binding = 1;
  write[1].buffers = &color_buffer;
  write[1].descriptor_count = 1;
  write[1].type = Renderer::DescriptorType::kUniformBuffer;

  Renderer::DescriptorImageInfo image_info;
  image_info.image_view = pass.image_view;
  image_info.sampler = pass.sampler;
  write[2].set = pass.descriptor_set;
  write[2].binding = 2;
  write[2].descriptor_count = 1;
  write[2].type = Renderer::DescriptorType::kCombinedImageSampler;
  write[2].images = &image_info;

  Renderer::UpdateDescriptorSets(device, 3, write);
}

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
  auto vert =
      util::ReadFile("samples/render_graph/cube/data/triangle.vert.spv");
  auto frag =
      util::ReadFile("samples/render_graph/cube/data/triangle.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input.resize(1);
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec3, 0});
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec2, sizeof(float) * 3});
  info.vertex_input[0].layout.push_back(
      {Renderer::VertexAttributeType::kVec3, sizeof(float) * 5});
  info.layout = layout;

  info.states.viewport.viewports.emplace_back(
      Renderer::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));
  info.states.blend.attachments.resize(1);
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