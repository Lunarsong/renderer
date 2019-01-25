#include <chrono>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <renderer/renderer.h>
#include "render_graph/render_graph.h"
#include "samples/common/util.h"

void CreateVkSurfance(RenderAPI::Instance instance, GLFWwindow* window);
RenderAPI::GraphicsPipeline CreatePipeline(RenderAPI::Device device,
                                           RenderAPI::RenderPass pass,
                                           RenderAPI::PipelineLayout layout);
GLFWwindow* InitWindow();
void Shutdown(GLFWwindow* window);

struct QuadPass {
  RenderAPI::Buffer vertex_buffer;
  RenderAPI::Buffer index_buffer;
  RenderAPI::Buffer uniform_buffer_offset;
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

void CompileQuadPass(QuadPass& pass, RenderAPI::Device device,
                     RenderAPI::CommandPool command_pool,
                     const RenderGraph& graph) {
  // Create the quad.
  std::vector<float> quad = {-0.5, -0.5, 0.0, 0.0, 1.0, 0.0, 0.0,  //
                             0.5,  0.5,  1.0, 1.0, 0.0, 0.0, 1.0,  //
                             -0.5, 0.5,  0.0, 1.0, 0.0, 1.0, 0.0,  //
                             0.5,  -0.5, 1.0, 0.0, 1.0, 1.0, 1.0};
  pass.vertex_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kVertex, sizeof(float) * 4 * 7,
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, pass.vertex_buffer,
                                   quad.data(), sizeof(float) * 4 * 7);

  // Create the index buffer in GPU memory and copy the data.
  const uint32_t indices[] = {0, 1, 2, 3, 1, 0};
  pass.index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kIndex, sizeof(uint32_t) * 6,
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, pass.index_buffer, indices,
                                   sizeof(uint32_t) * 6);

  float offsets[] = {0.5f, 0.0f, 0.0f, 0.0f};
  pass.uniform_buffer_offset = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferType::kUniform, sizeof(float) * 4);
  memcpy(RenderAPI::MapBuffer(pass.uniform_buffer_offset), offsets,
         sizeof(float) * 4);
  RenderAPI::UnmapBuffer(pass.uniform_buffer_offset);

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
  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_layout_info = {
      {{RenderAPI::DescriptorType::kUniformBuffer, 1,
        RenderAPI::ShaderStageFlagBits::kVertexBit},
       {RenderAPI::DescriptorType::kUniformBuffer, 1,
        RenderAPI::ShaderStageFlagBits::kFragmentBit},
       {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
        RenderAPI::ShaderStageFlagBits::kFragmentBit}}};
  pass.descriptor_layout =
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info);

  RenderAPI::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(1);
  pass_info.attachments[0].final_layout =
      RenderAPI::ImageLayout::kPresentSrcKHR;
  pass_info.attachments[0].format =
      RenderAPI::GetSwapChainImageFormat(graph.GetSwapChain());
  static bool first = true;
  if (first) {
    first = false;
    pass_info.attachments[0].final_layout =
        RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
  }
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
  RenderAPI::DescriptorBufferInfo offset_buffer;
  offset_buffer.buffer = pass.uniform_buffer_offset;
  offset_buffer.range = sizeof(float) * 4;
  write[0].set = pass.descriptor_set;
  write[0].binding = 0;
  write[0].buffers = &offset_buffer;
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

void RenderQuad(RenderContext* context, QuadPass& pass) {
  RenderAPI::CommandBuffer cmd = context->cmd;

  RenderAPI::CmdBindPipeline(cmd, pass.pipeline);
  RenderAPI::CmdBindVertexBuffers(cmd, 0, 1, &pass.vertex_buffer);
  RenderAPI::CmdBindIndexBuffer(cmd, pass.index_buffer,
                                RenderAPI::IndexType::kUInt32);
  RenderAPI::CmdBindDescriptorSets(cmd, 0, pass.pipeline_layout, 0, 1,
                                   &pass.descriptor_set);
  RenderAPI::CmdDrawIndexed(cmd, 6);
}

void DestroyQuadPass(RenderAPI::Device device, QuadPass& pass) {
  RenderAPI::DestroyDescriptorSetPool(pass.descriptor_set_pool);
  RenderAPI::DestroyDescriptorSetLayout(pass.descriptor_layout);

  RenderAPI::DestroySampler(device, pass.sampler);
  RenderAPI::DestroyImageView(device, pass.image_view);
  RenderAPI::DestroyImage(pass.image);

  RenderAPI::DestroyBuffer(pass.vertex_buffer);
  RenderAPI::DestroyBuffer(pass.index_buffer);
  RenderAPI::DestroyBuffer(pass.uniform_buffer_color);
  RenderAPI::DestroyBuffer(pass.uniform_buffer_offset);

  RenderAPI::DestroyPipelineLayout(device, pass.pipeline_layout);
  RenderAPI::DestroyGraphicsPipeline(pass.pipeline);

  RenderAPI::DestroyRenderPass(pass.render_pass);
}

void UpdateDescriptorSetTexture(RenderAPI::Device device, QuadPass& pass,
                                RenderAPI::ImageView image_view) {
  RenderAPI::WriteDescriptorSet write[3];
  RenderAPI::DescriptorBufferInfo offset_buffer;
  offset_buffer.buffer = pass.uniform_buffer_offset;
  offset_buffer.range = sizeof(float) * 4;
  write[0].set = pass.descriptor_set;
  write[0].binding = 0;
  write[0].buffers = &offset_buffer;
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
  image_info.image_view = image_view;
  image_info.sampler = pass.sampler;
  write[2].set = pass.descriptor_set;
  write[2].binding = 2;
  write[2].descriptor_count = 1;
  write[2].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[2].images = &image_info;

  RenderAPI::UpdateDescriptorSets(device, 3, write);
}

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

  QuadPass example_pass_1;
  QuadPass example_pass_2;

  CompileQuadPass(example_pass_1, device, command_pool, render_graph_);
  CompileQuadPass(example_pass_2, device, command_pool, render_graph_);
  float offsets[] = {-0.5f, 0.0f, 0.0f, 0.0f};
  memcpy(RenderAPI::MapBuffer(example_pass_2.uniform_buffer_offset), offsets,
         sizeof(float) * 4);
  RenderAPI::UnmapBuffer(example_pass_2.uniform_buffer_offset);

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

    render_graph_.BeginFrame();

    RenderGraphResource texture;
    render_graph_.AddPass(
        "right quad",
        [&](RenderGraphBuilder& builder) {
          auto desc = render_graph_.GetSwapChainDescription();
          desc.layout = RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
          texture = builder.CreateRenderTarget(desc).textures[0];
        },
        [&](RenderContext* context, const Scope& scope) {
          RenderQuad(context, example_pass_1);
        });

    RenderGraphResource output;
    render_graph_.AddPass(
        "left quad",
        [&](RenderGraphBuilder& builder) {
          builder.Read(texture);
          output =
              builder
                  .CreateRenderTarget(render_graph_.GetSwapChainDescription())
                  .textures[0];
        },
        [&](RenderContext* context, const Scope& scope) {
          UpdateDescriptorSetTexture(device, example_pass_2,
                                     scope.GetTexture(texture));
          RenderQuad(context, example_pass_2);
        });

    render_graph_.MoveSubresource(output,
                                  render_graph_.GetBackbufferResource());
    render_graph_.Render();
  }

  render_graph_.Destroy();
  DestroyQuadPass(device, example_pass_1);
  DestroyQuadPass(device, example_pass_2);

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
      util::ReadFile("samples/render_graph/simple/data/triangle.vert.spv");
  auto frag =
      util::ReadFile("samples/render_graph/simple/data/triangle.frag.spv");
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