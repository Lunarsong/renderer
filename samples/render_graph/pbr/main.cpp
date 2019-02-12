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
#include <Renderer/Material.h>
#include "MaterialBits.h"
#include "MaterialCache.h"
#include "render_graph/render_graph.h"
#include "renderer.h"
#include "samples/common/camera_controller.h"
#include "samples/common/util.h"
#include "tonemap_pass.h"

#include "util.h"

namespace {
struct ObjectsData {
  glm::mat4 uMatWorldViewProjection;
  glm::mat4 uMatWorld;
  glm::mat4 uMatView;
  glm::mat4 uMatNormalsMatrix;
};

struct LightDataGPU {
  glm::mat4 uCascadeViewProjMatrices[4];
  glm::vec4 uCascadeSplits;
  alignas(16) glm::vec3 uCameraPosition;
  alignas(16) glm::vec3 uLightDirection;
};
}  // namespace

void CreateVkSurfance(RenderAPI::Instance instance, GLFWwindow* window);
RenderAPI::GraphicsPipeline CreatePipeline(RenderAPI::Device device,
                                           RenderAPI::RenderPass pass,
                                           RenderAPI::PipelineLayout layout);
GLFWwindow* InitWindow();
void Shutdown(GLFWwindow* window);

void CreateMaterials(RenderAPI::Device device, MaterialCache* cache) {
  // Default data.
  MetallicRoughnessMaterialGpuData default_material;
  default_material.uBaseColor =
      glm::vec4(253.0f, 181.0f, 21.0f, 255.0f) / glm::vec4(255.0f);
  default_material.uMetallicRoughness = glm::vec2(1.0f, 0.2f);
  default_material.uAmbientOcclusion = 0.25f;

  // PBR Pipeline
  auto vert = util::ReadFile("samples/render_graph/pbr/data/pbr.vert.spv");
  auto frag = util::ReadFile("samples/render_graph/pbr/data/pbr.frag.spv");
  Material::Builder builder(device);
  builder.VertexCode(reinterpret_cast<const uint32_t*>(vert.data()),
                     vert.size());
  builder.FragmentCode(reinterpret_cast<const uint32_t*>(frag.data()),
                       frag.size());
  builder.Specialization(RenderAPI::ShaderStageFlagBits::kFragmentBit, 0, true);
  builder.Specialization(RenderAPI::ShaderStageFlagBits::kFragmentBit, 1, true);
  builder.Specialization(RenderAPI::ShaderStageFlagBits::kFragmentBit, 2, true);
  builder.Specialization(RenderAPI::ShaderStageFlagBits::kFragmentBit, 3, true);
  builder.Specialization(RenderAPI::ShaderStageFlagBits::kFragmentBit, 4, true);
  builder.Sampler(
      "cubemap",
      RenderAPI::SamplerCreateInfo(
          RenderAPI::SamplerFilter::kLinear, RenderAPI::SamplerFilter::kLinear,
          RenderAPI::SamplerMipmapMode::kLinear,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge, 0.0f, 9.0f));
  builder.Sampler(
      "shadow",
      RenderAPI::SamplerCreateInfo(
          RenderAPI::SamplerFilter::kLinear, RenderAPI::SamplerFilter::kLinear,
          RenderAPI::SamplerMipmapMode::kLinear,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge, 0.0f, 9.0f, true,
          RenderAPI::CompareOp::kLessOrEqual));
  // Transform data.
  builder.Uniform(0, 0, RenderAPI::ShaderStageFlagBits::kVertexBit,
                  sizeof(ObjectsData));
  // Material data.
  builder.Texture(1, 0, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 1, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 2, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 3, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 4, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Uniform(1, 5, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  sizeof(MetallicRoughnessMaterialGpuData), &default_material);
  // Light data.
  builder.Uniform(2, 0, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  sizeof(LightDataGPU));
  builder.Texture(2, 1, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  "cubemap");
  builder.Texture(2, 2, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  "cubemap");
  builder.Texture(2, 3, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  "cubemap");
  builder.Texture(2, 4, RenderAPI::ShaderStageFlagBits::kFragmentBit, "shadow");
  builder.SetDescriptorFrequency(2, DescriptorFrequency::kUndefined);

  builder.Viewport(RenderAPI::Viewport(0.0f, 0.0f, 1920, 1200));
  builder.DynamicState(RenderAPI::DynamicState::kViewport);
  builder.DynamicState(RenderAPI::DynamicState::kScissor);
  builder.DepthTest(true);
  builder.DepthWrite(true);
  builder.CullMode(RenderAPI::CullModeFlagBits::kNone);
  builder.AddVertexAttribute(VertexAttribute::kPosition);
  builder.AddVertexAttribute(VertexAttribute::kTexCoords);
  builder.AddVertexAttribute(VertexAttribute::kColor);
  builder.AddVertexAttribute(VertexAttribute::kNormals);
  cache->Cache("Metallic Roughness",
               MetallicRoughnessBits::kHasMetallicRoughnessTexture |
                   MetallicRoughnessBits::kHasBaseColorTexture |
                   MetallicRoughnessBits::kHashEmissiveTexture |
                   MetallicRoughnessBits::kHasNormalsTexture |
                   MetallicRoughnessBits::kHasOcclusionTexture,
               builder.Build());

  // PBR Pipeline
  builder.VertexCode(reinterpret_cast<const uint32_t*>(vert.data()),
                     vert.size());
  builder.FragmentCode(reinterpret_cast<const uint32_t*>(frag.data()),
                       frag.size());
  builder.Sampler(
      "cubemap",
      RenderAPI::SamplerCreateInfo(
          RenderAPI::SamplerFilter::kLinear, RenderAPI::SamplerFilter::kLinear,
          RenderAPI::SamplerMipmapMode::kLinear,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge, 0.0f, 9.0f));
  builder.Sampler(
      "shadow",
      RenderAPI::SamplerCreateInfo(
          RenderAPI::SamplerFilter::kLinear, RenderAPI::SamplerFilter::kLinear,
          RenderAPI::SamplerMipmapMode::kLinear,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge,
          RenderAPI::SamplerAddressMode::kClampToEdge, 0.0f, 9.0f, true,
          RenderAPI::CompareOp::kLessOrEqual));
  // Transform data.
  builder.Uniform(0, 0, RenderAPI::ShaderStageFlagBits::kVertexBit,
                  sizeof(ObjectsData));
  // Material data.
  builder.Texture(1, 0, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 1, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 2, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 3, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Texture(1, 4, RenderAPI::ShaderStageFlagBits::kFragmentBit);
  builder.Uniform(1, 5, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  sizeof(MetallicRoughnessMaterialGpuData), &default_material);
  // Light data.
  builder.Uniform(2, 0, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  sizeof(LightDataGPU));
  builder.Texture(2, 1, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  "cubemap");
  builder.Texture(2, 2, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  "cubemap");
  builder.Texture(2, 3, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                  "cubemap");
  builder.Texture(2, 4, RenderAPI::ShaderStageFlagBits::kFragmentBit, "shadow");
  builder.SetDescriptorFrequency(2, DescriptorFrequency::kUndefined);

  builder.Viewport(RenderAPI::Viewport(0.0f, 0.0f, 1920, 1200));
  builder.DynamicState(RenderAPI::DynamicState::kViewport);
  builder.DynamicState(RenderAPI::DynamicState::kScissor);
  builder.DepthTest(true);
  builder.DepthWrite(true);
  builder.CullMode(RenderAPI::CullModeFlagBits::kNone);
  builder.AddVertexAttribute(VertexAttribute::kPosition);
  builder.AddVertexAttribute(VertexAttribute::kTexCoords);
  builder.AddVertexAttribute(VertexAttribute::kColor);
  builder.AddVertexAttribute(VertexAttribute::kNormals);
  cache->Cache("Metallic Roughness", 0, builder.Build());
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
  MaterialCache* materials = new MaterialCache();
  CreateMaterials(device, materials);
  renderer->SetPbrMaterial(materials->Get("Metallic Roughness", 0));
  Scene scene;
  /*scene.meshes.emplace_back(CreateSphereMesh(device, command_pool));
  scene.meshes.back().primitives[0].material =
      renderer->CreatePbrMaterialInstance();*/
  scene.meshes.emplace_back(CreatePlaneMesh(device, command_pool));
  scene.meshes.back().primitives[0].material =
      materials->Get("Metallic Roughness", 0)->CreateInstance();
  MetallicRoughnessMaterialGpuData mat;
  mat.uBaseColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
  mat.uMetallicRoughness = glm::vec2(0.0f, 0.5f);
  mat.uAmbientOcclusion = 0.2f;
  scene.meshes.back().primitives[0].material->SetParam(1, 5, mat);

  std::vector<RenderAPI::Image> image_cache;
  std::vector<RenderAPI::ImageView> image_views_cache;
  Mesh mesh;
  MeshFromGLTF(device, command_pool, materials, mesh, image_cache,
               image_views_cache);
  scene.meshes.push_back(std::move(mesh));

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

  scene.SetIndirectLight(irradiance_view, prefilter_view, brdf_view);
  scene.SetSkybox(cubemap_view);

  TonemapPass tonemap = CreateTonemapPass(device);

  CameraController camera;
  camera.position.z = -10.0f;
  camera.position.y = 2.5f;
  std::chrono::high_resolution_clock::time_point time =
      std::chrono::high_resolution_clock::now();
  float rotation = 0.0f;

  View* view = renderer->CreateView();
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

    // Update the camera view.
    view->viewport.width = render_graph_.GetSwapChainDescription().width;
    view->viewport.height = render_graph_.GetSwapChainDescription().height;
    view->camera.SetOrientation(camera.rotation);
    view->camera.SetPosition(camera.position);
    view->camera.SetPerspective(70.0f, view->viewport.width,
                                view->viewport.height, 0.25f, 250.0f);

    // Render.
    render_graph_.BeginFrame();
    RenderGraphResource output;
    output = renderer->Render(render_graph_, view, &scene);
    output = AddTonemapPass(device, render_graph_, &tonemap,
                            /* use previous output as input */ output);

    render_graph_.MoveSubresource(output,
                                  render_graph_.GetBackbufferResource());
    render_graph_.Render();
  }

  render_graph_.Destroy();
  DestroyScene(device, scene);
  delete renderer;
  delete materials;

  for (auto& it : image_views_cache) {
    RenderAPI::DestroyImageView(device, it);
  }
  for (auto& it : image_cache) {
    RenderAPI::DestroyImage(it);
  }

  renderer->DestroyView(&view);
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