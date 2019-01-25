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

void CreateVkSurfance(RenderAPI::Instance instance, GLFWwindow* window);
RenderAPI::GraphicsPipeline CreatePipeline(RenderAPI::Device device,
                                           RenderAPI::RenderPass pass,
                                           RenderAPI::PipelineLayout layout);
GLFWwindow* InitWindow();
void Shutdown(GLFWwindow* window);

struct CubePass {
  RenderAPI::Buffer vertex_buffer;
  RenderAPI::Buffer index_buffer;
  RenderAPI::Buffer uniform_buffer_mvp;
  RenderAPI::Buffer uniform_buffer_color;

  RenderAPI::Image image;
  RenderAPI::ImageView image_view;
  RenderAPI::Sampler sampler;

  RenderAPI::RenderPass render_pass;

  RenderAPI::DescriptorSetLayout descriptor_layout;
  RenderAPI::DescriptorSetPool descriptor_set_pool;
  RenderAPI::DescriptorSet descriptor_set;

  RenderAPI::PipelineLayout pipeline_layout;
  RenderAPI::GraphicsPipeline pipeline;
};

void RenderCube(RenderContext* context, CubePass& pass);
void DestroyCubePass(RenderAPI::Device device, CubePass& pass);
void CreateCubePass(CubePass& pass, RenderAPI::Device device,
                    RenderAPI::CommandPool command_pool,
                    const RenderGraph& graph);

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
  RenderAPI::CommandPool command_pool = RenderAPI::CreateCommandPool(device);

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
    RenderGraphResource output;
    render_graph_.AddPass(
        "Cube",
        [&](RenderGraphBuilder& builder) {
          RenderGraphFramebufferDesc desc;
          desc.textures.push_back(render_graph_.GetSwapChainDescription());
          RenderGraphTextureDesc depth_desc;
          depth_desc.format = RenderAPI::TextureFormat::kD32_SFLOAT;
          depth_desc.width = desc.textures[0].width;
          depth_desc.height = desc.textures[0].height;
          depth_desc.load_op = RenderAPI::AttachmentLoadOp::kClear;
          depth_desc.layout =
              RenderAPI::ImageLayout::kDepthStencilAttachmentOptimal;
          depth_desc.clear_values.depth_stencil.depth = 1.0f;
          desc.textures.push_back(std::move(depth_desc));
          output = builder.CreateRenderTarget(desc).textures[0];
        },
        [&](RenderContext* context, const Scope& scope) {
          glm::mat4 perspective = glm::perspectiveFov(
              glm::radians(45.0f), 1980.0f, 1200.0f, 0.1f, 100.0f);
          perspective[1][1] *= -1.0f;
          glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 2.5f, -5.0f),
                                       glm::vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
          glm::mat4 mvp = perspective * view *
                          glm::mat4_cast(glm::angleAxis(
                              rotation, glm::vec3(0.0f, 1.0f, 0.0f)));
          memcpy(RenderAPI::MapBuffer(cube_pass.uniform_buffer_mvp), &mvp,
                 sizeof(mvp));
          RenderAPI::UnmapBuffer(cube_pass.uniform_buffer_mvp);
          RenderCube(context, cube_pass);
        });
    render_graph_.MoveSubresource(output,
                                  render_graph_.GetBackbufferResource());
    render_graph_.Render();
  }

  render_graph_.Destroy();
  DestroyCubePass(device, cube_pass);

  RenderAPI::DestroyCommandPool(command_pool);
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
void RenderCube(RenderContext* context, CubePass& pass) {
  RenderAPI::CommandBuffer cmd = context->cmd;

  RenderAPI::CmdBindPipeline(cmd, pass.pipeline);
  RenderAPI::CmdBindVertexBuffers(cmd, 0, 1, &pass.vertex_buffer);
  RenderAPI::CmdBindIndexBuffer(cmd, pass.index_buffer,
                                RenderAPI::IndexType::kUInt32);
  RenderAPI::CmdBindDescriptorSets(cmd, 0, pass.pipeline_layout, 0, 1,
                                   &pass.descriptor_set);
  RenderAPI::CmdDrawIndexed(cmd, 6 * 6);
}

void DestroyCubePass(RenderAPI::Device device, CubePass& pass) {
  RenderAPI::DestroyDescriptorSetPool(pass.descriptor_set_pool);
  RenderAPI::DestroyDescriptorSetLayout(pass.descriptor_layout);

  RenderAPI::DestroySampler(device, pass.sampler);
  RenderAPI::DestroyImageView(device, pass.image_view);
  RenderAPI::DestroyImage(pass.image);

  RenderAPI::DestroyBuffer(pass.vertex_buffer);
  RenderAPI::DestroyBuffer(pass.index_buffer);
  RenderAPI::DestroyBuffer(pass.uniform_buffer_color);
  RenderAPI::DestroyBuffer(pass.uniform_buffer_mvp);

  RenderAPI::DestroyPipelineLayout(device, pass.pipeline_layout);
  RenderAPI::DestroyGraphicsPipeline(pass.pipeline);

  RenderAPI::DestroyRenderPass(pass.render_pass);
}

void CreateCubePass(CubePass& pass, RenderAPI::Device device,
                    RenderAPI::CommandPool command_pool,
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
  pass.vertex_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kVertex, sizeof(float) * cube.size(),
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, pass.vertex_buffer,
                                   cube.data(), sizeof(float) * cube.size());

  // Create the index buffer in GPU memory and copy the data.
  const std::vector<uint32_t> indices = {
      0,  1,  2,  3,  1,  0,   // Front face
      4,  5,  6,  7,  5,  4,   // Back face
      10, 9,  8,  8,  11, 10,  // Left face
      12, 13, 14, 14, 15, 12,  // Right face
      19, 17, 16, 16, 18, 19,  // Bottom face
      20, 21, 23, 23, 22, 20,  // Top face

  };
  pass.index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kIndex, sizeof(uint32_t) * indices.size(),
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, pass.index_buffer,
                                   indices.data(),
                                   sizeof(uint32_t) * indices.size());

  pass.uniform_buffer_mvp = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kUniform, sizeof(glm::mat4));

  float color[] = {0.5f, 0.5f, 0.5f, 1.0f};
  pass.uniform_buffer_color = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kUniform, sizeof(float) * 4);
  memcpy(RenderAPI::MapBuffer(pass.uniform_buffer_color), color,
         sizeof(float) * 4);
  RenderAPI::UnmapBuffer(pass.uniform_buffer_color);

  // Create the image and sampler.
  pass.image =
      RenderAPI::CreateImage(device, {RenderAPI::TextureType::Texture2D,
                                      RenderAPI::TextureFormat::kR8G8B8A8_UNORM,
                                      RenderAPI::Extent3D(2, 2, 1)});
  unsigned char pixels[] = {255, 255, 255, 255, 0,   0,   0,   255,
                            0,   0,   0,   255, 255, 255, 255, 255};
  RenderAPI::BufferImageCopy image_copy(0, 0, 0, 2, 2, 1);
  RenderAPI::StageCopyDataToImage(command_pool, pass.image, pixels, 2 * 2 * 4,
                                  image_copy);
  RenderAPI::ImageViewCreateInfo image_view_info(
      pass.image, RenderAPI::ImageViewType::Texture2D,
      RenderAPI::TextureFormat::kR8G8B8A8_UNORM,
      RenderAPI::ImageSubresourceRange(
          RenderAPI::ImageAspectFlagBits::kColorBit));
  pass.image_view = RenderAPI::CreateImageView(device, image_view_info);
  pass.sampler = RenderAPI::CreateSampler(device);

  // Create the pipeline.
  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_layout_info(
      {{{RenderAPI::DescriptorType::kUniformBuffer, 1,
         RenderAPI::ShaderStageFlagBits::kVertexBit},
        {RenderAPI::DescriptorType::kUniformBuffer, 1,
         RenderAPI::ShaderStageFlagBits::kFragmentBit},
        {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
         RenderAPI::ShaderStageFlagBits::kFragmentBit}}});
  pass.descriptor_layout =
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info);

  RenderAPI::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(2);
  pass_info.attachments[0].final_layout =
      RenderAPI::ImageLayout::kPresentSrcKHR;
  pass_info.attachments[0].format =
      RenderAPI::GetSwapChainImageFormat(graph.GetSwapChain());
  pass_info.attachments[1].final_layout =
      RenderAPI::ImageLayout::kDepthStencilAttachmentOptimal;
  pass_info.attachments[1].format = RenderAPI::TextureFormat::kD32_SFLOAT;
  pass.render_pass = RenderAPI::CreateRenderPass(device, pass_info);

  // Create a pipeline.
  pass.pipeline_layout =
      RenderAPI::CreatePipelineLayout(device, {{pass.descriptor_layout}});
  pass.pipeline =
      CreatePipeline(device, pass.render_pass, pass.pipeline_layout);

  // Create the descriptor sets.
  RenderAPI::CreateDescriptorSetPoolCreateInfo pool_info = {
      {{RenderAPI::DescriptorType::kUniformBuffer, 2},
       {RenderAPI::DescriptorType::kCombinedImageSampler, 1}},
      /*max_sets=*/1};
  pass.descriptor_set_pool =
      RenderAPI::CreateDescriptorSetPool(device, pool_info);

  RenderAPI::AllocateDescriptorSets(
      pass.descriptor_set_pool,
      std::vector<RenderAPI::DescriptorSetLayout>(1, pass.descriptor_layout),
      &pass.descriptor_set);

  RenderAPI::WriteDescriptorSet write[3];
  RenderAPI::DescriptorBufferInfo mvp_buffer;
  mvp_buffer.buffer = pass.uniform_buffer_mvp;
  mvp_buffer.range = sizeof(glm::mat4);
  write[0].set = pass.descriptor_set;
  write[0].binding = 0;
  write[0].buffers = &mvp_buffer;
  write[0].descriptor_count = 1;
  write[0].type = RenderAPI::DescriptorType::kUniformBuffer;

  RenderAPI::DescriptorBufferInfo color_buffer;
  color_buffer.buffer = pass.uniform_buffer_color;
  color_buffer.range = sizeof(float) * 4;
  write[1].set = pass.descriptor_set;
  write[1].binding = 1;
  write[1].buffers = &color_buffer;
  write[1].descriptor_count = 1;
  write[1].type = RenderAPI::DescriptorType::kUniformBuffer;

  RenderAPI::DescriptorImageInfo image_info;
  image_info.image_view = pass.image_view;
  image_info.sampler = pass.sampler;
  write[2].set = pass.descriptor_set;
  write[2].binding = 2;
  write[2].descriptor_count = 1;
  write[2].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[2].images = &image_info;

  RenderAPI::UpdateDescriptorSets(device, 3, write);
}

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
  auto vert =
      util::ReadFile("samples/render_graph/gltf/data/triangle.vert.spv");
  auto frag =
      util::ReadFile("samples/render_graph/gltf/data/triangle.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input.resize(1);
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec3, 0});
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec2, sizeof(float) * 3});
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec3, sizeof(float) * 5});
  info.layout = layout;

  info.states.viewport.viewports.emplace_back(
      RenderAPI::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));
  info.states.blend.attachments.resize(1);
  info.states.depth_stencil.depth_write_enable = true;
  info.states.depth_stencil.depth_test_enable = true;
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