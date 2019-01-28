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

#include <RenderAPI/RenderAPI.h>
#include "render_graph/render_graph.h"
#include "renderer.h"
#include "samples/common/camera_controller.h"
#include "samples/common/util.h"
#include "tonemap_pass.h"

#include "util.h"

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

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  // Create a device and swapchain.
  RenderAPI::Device device = RenderAPI::CreateDevice(instance);
  RenderAPI::CommandPool command_pool = RenderAPI::CreateCommandPool(device);

  RenderGraph render_graph_(device);
  render_graph_.BuildSwapChain(width, height);

  RenderAPI::Image cubemap_image;
  RenderAPI::ImageView cubemap_view;
  util::LoadTexture("samples/render_graph/pbr/data/cubemap.ktx", device,
                    command_pool, cubemap_image, cubemap_view);

  Renderer* renderer = new Renderer(device);
  Scene scene;
  scene.models.emplace_back(CreateSphereModel(device, command_pool));

  RenderAPI::Image irradiance_image;
  RenderAPI::ImageView irradiance_view;
  util::LoadTexture("samples/render_graph/pbr/data/irradiance.ktx", device,
                    command_pool, irradiance_image, irradiance_view);
  RenderAPI::Image prefilter_image;
  RenderAPI::ImageView prefilter_view;
  util::LoadTexture("samples/render_graph/pbr/data/prefilter.ktx", device,
                    command_pool, prefilter_image, prefilter_view);
  RenderAPI::Image brdf_image;
  RenderAPI::ImageView brdf_view;
  util::LoadTexture("samples/render_graph/pbr/data/brdf.ktx", device,
                    command_pool, brdf_image, brdf_view);
  renderer->SetIndirectLight(scene, irradiance_view, prefilter_view, brdf_view);
  renderer->SetSkybox(scene, cubemap_view);

  TonemapPass tonemap = CreateTonemapPass(device);

  CameraController camera;
  camera.position.z = -10.0f;
  camera.position.y = 2.5f;
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
    UpdateCameraController(camera, window, delta_seconds);

    render_graph_.BeginFrame();
    RenderGraphResource output;
    View view;
    render_graph_.AddPass(
        "Scene",
        [&](RenderGraphBuilder& builder) {
          RenderGraphFramebufferDesc desc;
          desc.textures.push_back(render_graph_.GetSwapChainDescription());
          desc.textures[0].format =
              RenderAPI::TextureFormat::kR16G16B16A16_SFLOAT;
          desc.textures[0].layout =
              RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
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

          view.viewport.height = depth_desc.height;
          view.viewport.width = depth_desc.width;
          view.camera.projection =
              glm::perspectiveFov(glm::radians(45.0f), view.viewport.width,
                                  view.viewport.height, 0.1f, 100.0f);

          view.camera.projection[1][1] *= -1.0f;
          view.camera.view = camera.view;
          view.camera.position = camera.position;
        },
        [&](RenderContext* context, const Scope& scope) {
          renderer->Render(context->cmd, view, scene);
        });

    output = AddTonemapPass(device, render_graph_, tonemap,
                            /* use previous output as input */ output);

    render_graph_.MoveSubresource(output,
                                  render_graph_.GetBackbufferResource());
    render_graph_.Render();
  }

  render_graph_.Destroy();
  DestroyScene(device, scene);
  delete renderer;

  DestroyTonemapPass(device, tonemap);
  RenderAPI::DestroyImageView(device, cubemap_view);
  RenderAPI::DestroyImage(cubemap_image);
  RenderAPI::DestroyImageView(device, irradiance_view);
  RenderAPI::DestroyImage(irradiance_image);
  RenderAPI::DestroyImageView(device, prefilter_view);
  RenderAPI::DestroyImage(prefilter_image);
  RenderAPI::DestroyImageView(device, brdf_view);
  RenderAPI::DestroyImage(brdf_image);
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
  auto vert = util::ReadFile("samples/render_graph/pbr/data/pbr.vert.spv");
  auto frag = util::ReadFile("samples/render_graph/pbr/data/pbr.frag.spv");
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