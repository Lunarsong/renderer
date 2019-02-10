#include "renderer.h"

#include <fstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <string>
#include "vertex.h"

namespace {
constexpr size_t kMaxInstances = 1024;

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

std::vector<char> ReadFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file " + filename);
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

RenderAPI::GraphicsPipeline CreatePipeline(RenderAPI::Device device,
                                           RenderAPI::RenderPass pass,
                                           RenderAPI::PipelineLayout layout) {
  RenderAPI::GraphicsPipeline pipeline = RenderAPI::kInvalidHandle;

  RenderAPI::GraphicsPipelineCreateInfo info;
  auto vert = ReadFile("samples/render_graph/pbr/data/pbr.vert.spv");
  auto frag = ReadFile("samples/render_graph/pbr/data/pbr.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input = {{Vertex::layout}};
  info.layout = layout;

  info.states.blend.attachments.resize(1);
  info.states.depth_stencil.depth_write_enable = true;
  info.states.depth_stencil.depth_test_enable = true;

  // info.states.rasterization.front_face = RenderAPI::FrontFace::kClockwise;

  info.states.viewport.viewports.emplace_back(
      RenderAPI::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));

  info.states.dynamic_states.states.push_back(
      RenderAPI::DynamicState::kViewport);
  info.states.dynamic_states.states.push_back(
      RenderAPI::DynamicState::kScissor);
  pipeline = RenderAPI::CreateGraphicsPipeline(device, pass, info);

  return pipeline;
}

void CreateCubemap(RenderAPI::Device device,
                   RenderAPI::CommandPool command_pool, RenderAPI::Buffer& vb,
                   RenderAPI::Buffer& ib) {
  std::vector<float> cube = {
      // Font face.
      -1.0, -1.0, -1.0f,  //
      1.0, 1.0, -1.0f,    //
      -1.0, 1.0, -1.0f,   //
      1.0, -1.0, -1.0f,

      // Back face.
      1.0, -1.0, 1.0f,  //
      -1.0, 1.0, 1.0f,  //
      1.0, 1.0, 1.0f,   //
      -1.0, -1.0, 1.0f,

      // Left face.
      -1.0, 1.0, -1.0f,   //
      -1.0, -1.0, -1.0f,  //
      -1.0, -1.0, 1.0f,   //
      -1.0, 1.0, 1.0f,

      // Right face.
      1.0, 1.0, -1.0f,   //
      1.0, -1.0, -1.0f,  //
      1.0, -1.0, 1.0f,   //
      1.0, 1.0, 1.0f,

      // Bottom face.
      -1.0, -1.0, -1.0f,  //
      1.0, -1.0, -1.0f,   //
      -1.0, -1.0, 1.0f,   //
      1.0, -1.0, 1.0f,

      // Top face.
      -1.0, 1.0, -1.0f,  //
      1.0, 1.0, -1.0f,   //
      -1.0, 1.0, 1.0f,   //
      1.0, 1.0, 1.0f,    //
  };
  vb = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kVertexBuffer,
      sizeof(float) * cube.size(), RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, vb, cube.data(),
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
  ib = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kIndexBuffer,
      sizeof(uint32_t) * indices.size(), RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, ib, indices.data(),
                                   sizeof(uint32_t) * indices.size());
}
}  // namespace

Renderer::Renderer(RenderAPI::Device device) : device_(device) {
  command_pool_ = RenderAPI::CreateCommandPool(device_);

  CreateCubemap(device_, command_pool_, cubemap_vertex_buffer_,
                cubemap_index_buffer_);

  // Cubemap pipeline.
  auto vert = ReadFile("samples/render_graph/pbr/data/cubemap.vert.spv");
  auto frag = ReadFile("samples/render_graph/pbr/data/cubemap.frag.spv");

  Material::Builder skybox_builder(device);
  skybox_builder.VertexCode(reinterpret_cast<const uint32_t*>(vert.data()),
                            vert.size());
  skybox_builder.FragmentCode(reinterpret_cast<const uint32_t*>(frag.data()),
                              frag.size());
  skybox_builder.PushConstant(RenderAPI::ShaderStageFlagBits::kVertexBit,
                              sizeof(glm::mat4));
  skybox_builder.Sampler("cubemap", RenderAPI::SamplerCreateInfo(
                                        RenderAPI::SamplerFilter::kLinear,
                                        RenderAPI::SamplerFilter::kLinear));
  skybox_builder.Texture(0, 0, RenderAPI::ShaderStageFlagBits::kFragmentBit,
                         "cubemap");
  skybox_builder.Viewport(RenderAPI::Viewport(0.0f, 0.0f, 1920, 1200));
  skybox_builder.DynamicState(RenderAPI::DynamicState::kViewport);
  skybox_builder.DynamicState(RenderAPI::DynamicState::kScissor);
  skybox_builder.DepthTest(true);
  skybox_builder.DepthCompareOp(RenderAPI::CompareOp::kLessOrEqual);
  skybox_builder.CullMode(RenderAPI::CullModeFlagBits::kFront);
  skybox_builder.AddVertexAttribute(VertexAttribute::kPosition);
  skybox_material_ = skybox_builder.Build();

  // Create the shadow pass.
  shadow_pass_ = CascadeShadowsPass::Create(device);
}

Renderer::~Renderer() {
  Material::Destroy(skybox_material_);
  CascadeShadowsPass::Destroy(device_, shadow_pass_);

  if (cubemap_vertex_buffer_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyBuffer(cubemap_vertex_buffer_);
    cubemap_vertex_buffer_ = RenderAPI::kInvalidHandle;
  }
  if (cubemap_index_buffer_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyBuffer(cubemap_index_buffer_);
    cubemap_index_buffer_ = RenderAPI::kInvalidHandle;
  }

  if (command_pool_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyCommandPool(command_pool_);
    command_pool_ = RenderAPI::kInvalidHandle;
  }
}

RenderGraphResource Renderer::Render(RenderGraph& render_graph, View* view,
                                     Scene* scene) {
  RenderAPI::ImageView shadow_texture = shadow_pass_.depth_array_view;
  std::vector<RenderGraphResource> render_graph_resources =
      CascadeShadowsPass::AddPass(&shadow_pass_, device_, render_graph, view,
                                  scene);

  RenderGraphResource output;
  render_graph.AddPass(
      "Scene",
      [&](RenderGraphBuilder& builder) {
        for (auto it : render_graph_resources) {
          builder.Read(it);
        }

        RenderGraphFramebufferDesc desc;
        desc.textures.push_back(render_graph.GetSwapChainDescription());
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
      },
      [this, view, scene, shadow_texture](RenderContext* context,
                                          const Scope& scope) {
        SetLightData(*view, scene, shadow_texture);
        Render(context, view, *scene);
      });

  return output;
}

void Renderer::Render(RenderContext* context, View* view, Scene& scene) {
  RenderAPI::CommandBuffer cmd = context->cmd;
  const glm::mat4 camera_view = view->camera.GetView();

  glm::mat4 cubemap_mvp = camera_view;
  cubemap_mvp[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  cubemap_mvp = view->camera.GetProjection() * cubemap_mvp;
  RenderAPI::Rect2D scissor = {
      RenderAPI::Offset2D(0, 0),
      RenderAPI::Extent2D(view->viewport.width, view->viewport.height)};

  // Draw the Skybox.
  SetSkybox(*view, scene.skybox);
  RenderAPI::CmdBindPipeline(cmd, skybox_material_->GetPipeline(context->pass));
  RenderAPI::CmdSetScissor(cmd, 0, 1, &scissor);
  RenderAPI::CmdSetViewport(cmd, 0, 1, &view->viewport);
  RenderAPI::CmdBindDescriptorSets(
      cmd, 0, skybox_material_->GetPipelineLayout(), 0, 1,
      view->skybox_material_instance->DescriptorSet(0));
  RenderAPI::CmdPushConstants(cmd, skybox_material_->GetPipelineLayout(),
                              RenderAPI::ShaderStageFlagBits::kVertexBit, 0,
                              sizeof(glm::mat4), &cubemap_mvp);
  RenderAPI::CmdBindVertexBuffers(cmd, 0, 1, &cubemap_vertex_buffer_);
  RenderAPI::CmdBindIndexBuffer(cmd, cubemap_index_buffer_,
                                RenderAPI::IndexType::kUInt32);
  RenderAPI::CmdDrawIndexed(cmd, 36, 1, 0, 0, 0);

  // Draw the scene.
  ObjectsData objects_data;
  objects_data.uMatView = camera_view;
  size_t instance_id = 0;
  for (const auto& mesh : scene.meshes) {
    // Update the uniform data for the mesh.
    objects_data.uMatWorld = mesh.mat_world;
    objects_data.uMatWorldViewProjection =
        view->camera.GetProjection() * camera_view * objects_data.uMatWorld;
    objects_data.uMatNormalsMatrix =
        glm::transpose(glm::inverse(objects_data.uMatWorld));
    for (const auto& primitive : mesh.primitives) {
      assert(primitive.material);
      Material* material = primitive.material->GetMaterial();
      MaterialInstance* instance = primitive.material;

      RenderAPI::CmdBindPipeline(cmd, material->GetPipeline(context->pass));
      RenderAPI::CmdSetScissor(cmd, 0, 1, &scissor);
      RenderAPI::CmdSetViewport(cmd, 0, 1, &view->viewport);
      RenderAPI::CmdBindDescriptorSets(cmd, 0, material->GetPipelineLayout(), 2,
                                       1, view->light_params->DescriptorSet());

      instance->SetParam(0, 0, objects_data);
      instance->Commit();

      RenderAPI::CmdBindDescriptorSets(cmd, 0, material->GetPipelineLayout(), 0,
                                       1, instance->DescriptorSet(0));
      RenderAPI::CmdBindDescriptorSets(cmd, 0, material->GetPipelineLayout(), 1,
                                       1, instance->DescriptorSet(1));

      RenderAPI::CmdBindVertexBuffers(cmd, 0, 1, &primitive.vertex_buffer);
      RenderAPI::CmdBindIndexBuffer(cmd, primitive.index_buffer,
                                    RenderAPI::IndexType::kUInt32);
      RenderAPI::CmdDrawIndexed(cmd, primitive.num_primitives, 1,
                                primitive.first_index, primitive.vertex_offset,
                                instance_id++);
    }
  }
}

void Renderer::SetSkybox(View& view, const Skybox& skybox) {
  view.skybox_material_instance->SetTexture(0, 0, skybox.sky);
  view.skybox_material_instance->Commit();
}

void Renderer::SetLightData(View& view, const Scene* scene,
                            RenderAPI::ImageView shadow_map_texture) {
  const IndirectLight& light = scene->indirect_light;
  view.light_params->SetTexture(1, light.irradiance);
  view.light_params->SetTexture(2, light.reflections);
  view.light_params->SetTexture(3, light.brdf);
  view.light_params->SetTexture(4, shadow_map_texture);

  const glm::mat4 kShadowBiasMatrix(0.5f, 0.0f, 0.0f, 0.0f,  //
                                    0.0f, 0.5f, 0.0f, 0.0f,  //
                                    0.0f, 0.0f, 1.0f, 0.0f,  //
                                    0.5f, 0.5f, 0.0f, 1.0f);
  LightDataGPU lights_data;
  for (uint32_t i = 0; i < shadow_pass_.num_cascades; ++i) {
    lights_data.uCascadeSplits[i] =
        view.camera.NearClip() +
        shadow_pass_.cascades[i].max_distance *
            (view.camera.FarClip() - view.camera.NearClip());
    lights_data.uCascadeViewProjMatrices[i] =
        kShadowBiasMatrix *
        (shadow_pass_.cascades[i].projection * shadow_pass_.cascades[i].view);
  }

  lights_data.uCameraPosition = view.camera.GetPosition();
  lights_data.uLightDirection = scene->directional_light.direction;
  view.light_params->SetParam(0, lights_data);

  view.light_params->Commit();
}

View* Renderer::CreateView() {
  View* view = new View();

  view->skybox_material_instance = skybox_material_->CreateInstance();
  view->light_params = pbr_material_->CreateParams(2);

  return view;
}

void Renderer::DestroyView(View** view) {
  MaterialInstance::Destroy((*view)->skybox_material_instance);
  MaterialParams::Destroy((*view)->light_params);

  delete *view;
  *view = nullptr;
}

void Renderer::SetPbrMaterial(Material* material) { pbr_material_ = material; }