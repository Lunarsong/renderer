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

RendererPipeline CreateCubemapPipeline(RenderAPI::Device device,
                                       RenderAPI::RenderPass pass) {
  RendererPipeline result;
  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_layout_info(
      {{{RenderAPI::DescriptorType::kCombinedImageSampler, 1,
         RenderAPI::ShaderStageFlagBits::kFragmentBit}}});
  result.descriptor_layouts.emplace_back(
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info));

  RenderAPI::PipelineLayoutCreateInfo layout_info;
  layout_info.layouts = result.descriptor_layouts;
  layout_info.push_constants.push_back(
      {RenderAPI::ShaderStageFlagBits::kVertexBit, 0, sizeof(glm::mat4)});
  result.pipeline_layout = RenderAPI::CreatePipelineLayout(device, layout_info);

  RenderAPI::GraphicsPipelineCreateInfo info;
  auto vert = ReadFile("samples/render_graph/pbr/data/cubemap.vert.spv");
  auto frag = ReadFile("samples/render_graph/pbr/data/cubemap.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input.resize(1);
  info.vertex_input[0].layout.push_back(
      {RenderAPI::VertexAttributeType::kVec3, 0});
  info.layout = result.pipeline_layout;

  info.states.blend.attachments.resize(1);
  info.states.depth_stencil.depth_write_enable = false;
  info.states.depth_stencil.depth_test_enable = true;
  info.states.depth_stencil.depth_compare_op =
      RenderAPI::CompareOp::kLessOrEqual;
  info.states.rasterization.front_face = RenderAPI::FrontFace::kClockwise;

  info.states.viewport.viewports.emplace_back(
      RenderAPI::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));

  info.states.dynamic_states.states.push_back(
      RenderAPI::DynamicState::kViewport);
  info.states.dynamic_states.states.push_back(
      RenderAPI::DynamicState::kScissor);
  result.pipeline = RenderAPI::CreateGraphicsPipeline(device, pass, info);

  return result;
}

void DestroyRendererPipeline(RenderAPI::Device device,
                             RendererPipeline& pipeline) {
  if (pipeline.pipeline != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyGraphicsPipeline(pipeline.pipeline);
    pipeline.pipeline = RenderAPI::kInvalidHandle;
  }
  if (pipeline.pipeline_layout != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyPipelineLayout(device, pipeline.pipeline_layout);
    pipeline.pipeline_layout = RenderAPI::kInvalidHandle;
  }

  for (auto& it : pipeline.descriptor_layouts) {
    if (it != RenderAPI::kInvalidHandle) {
      RenderAPI::DestroyDescriptorSetLayout(it);
    }
  }
  pipeline.descriptor_layouts.clear();
}
}  // namespace

Renderer::Renderer(RenderAPI::Device device) : device_(device) {
  command_pool_ = RenderAPI::CreateCommandPool(device_);

  CreateCubemap(device_, command_pool_, cubemap_vertex_buffer_,
                cubemap_index_buffer_);

  // Create the render pass.
  RenderAPI::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(2);
  pass_info.attachments[0].final_layout =
      RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
  pass_info.attachments[0].format =
      RenderAPI::TextureFormat::kR16G16B16A16_SFLOAT;
  pass_info.attachments[1].final_layout =
      RenderAPI::ImageLayout::kDepthStencilAttachmentOptimal;
  pass_info.attachments[1].format = RenderAPI::TextureFormat::kD32_SFLOAT;
  render_pass_ = RenderAPI::CreateRenderPass(device, pass_info);

  // Cubemap pipeline.
  cubemap_pipeline_ = CreateCubemapPipeline(device, render_pass_);
  RenderAPI::SamplerCreateInfo sampler_info;
  sampler_info.min_filter = RenderAPI::SamplerFilter::kLinear;
  sampler_info.mag_filter = RenderAPI::SamplerFilter::kLinear;
  sampler_info.max_lod = 9.0f;
  cubemap_sampler_ = RenderAPI::CreateSampler(device_, sampler_info);

  // Create the pipeline.
  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_layout_info(
      {{{RenderAPI::DescriptorType::kStorageBuffer, 1,
         RenderAPI::ShaderStageFlagBits::kVertexBit}}});
  pbr_pipeline_.descriptor_layouts.emplace_back(
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info));

  descriptor_layout_info.bindings.clear();
  descriptor_layout_info.bindings = {
      {RenderAPI::DescriptorType::kUniformBuffer, 1,
       RenderAPI::ShaderStageFlagBits::kFragmentBit},
      {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
       RenderAPI::ShaderStageFlagBits::kFragmentBit},
      {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
       RenderAPI::ShaderStageFlagBits::kFragmentBit},
      {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
       RenderAPI::ShaderStageFlagBits::kFragmentBit},
      {RenderAPI::DescriptorType::kCombinedImageSampler, 1,
       RenderAPI::ShaderStageFlagBits::kFragmentBit}};
  pbr_pipeline_.descriptor_layouts.emplace_back(
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info));

  // Create a pipeline.
  pbr_pipeline_.pipeline_layout = RenderAPI::CreatePipelineLayout(
      device, {{pbr_pipeline_.descriptor_layouts}});
  pbr_pipeline_.pipeline =
      CreatePipeline(device, render_pass_, pbr_pipeline_.pipeline_layout);

  // Create the descriptor sets.
  RenderAPI::CreateDescriptorSetPoolCreateInfo pool_info = {
      {{RenderAPI::DescriptorType::kStorageBuffer, 1},
       {RenderAPI::DescriptorType::kUniformBuffer, 3},
       {RenderAPI::DescriptorType::kCombinedImageSampler, 5 * 3}},
      /*max_sets=*/7};
  descriptor_set_pool_ = RenderAPI::CreateDescriptorSetPool(device, pool_info);

  // Shadow sampler.
  sampler_info.min_filter = RenderAPI::SamplerFilter::kLinear;
  sampler_info.mag_filter = RenderAPI::SamplerFilter::kLinear;
  sampler_info.address_mode_u = RenderAPI::SamplerAddressMode::kClampToBorder;
  sampler_info.address_mode_v = RenderAPI::SamplerAddressMode::kClampToBorder;
  sampler_info.address_mode_w = RenderAPI::SamplerAddressMode::kClampToBorder;
  sampler_info.max_lod = 1.0f;
  sampler_info.compare_enable = true;
  sampler_info.compare_op = RenderAPI::CompareOp::kLessOrEqual;
  shadow_sampler_ = RenderAPI::CreateSampler(device, sampler_info);

  shadow_pass_ = CascadeShadowsPass::Create(device);
}

Renderer::~Renderer() {
  CascadeShadowsPass::Destroy(device_, shadow_pass_);
  if (cubemap_sampler_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroySampler(device_, cubemap_sampler_);
    cubemap_sampler_ = RenderAPI::kInvalidHandle;
  }

  if (shadow_sampler_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroySampler(device_, shadow_sampler_);
    shadow_sampler_ = RenderAPI::kInvalidHandle;
  }

  if (descriptor_set_pool_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyDescriptorSetPool(descriptor_set_pool_);
    descriptor_set_pool_ = RenderAPI::kInvalidHandle;
  }

  DestroyRendererPipeline(device_, pbr_pipeline_);
  DestroyRendererPipeline(device_, cubemap_pipeline_);

  if (render_pass_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyRenderPass(render_pass_);
    render_pass_ = RenderAPI::kInvalidHandle;
  }

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
                                     const Scene* scene) {
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
        SetLightData(*view, scene->indirect_light, shadow_texture);
        Render(context->cmd, view, *scene);
      });

  return output;
}

void Renderer::Render(RenderAPI::CommandBuffer cmd, View* view,
                      const Scene& scene) {
  glm::mat4 cubemap_mvp = view->camera.view;
  cubemap_mvp[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  cubemap_mvp = view->camera.projection * cubemap_mvp;
  RenderAPI::Rect2D scissor = {
      RenderAPI::Offset2D(0, 0),
      RenderAPI::Extent2D(view->viewport.width, view->viewport.height)};

  // Draw the Skybox.
  SetSkybox(*view, scene.skybox);
  RenderAPI::CmdBindPipeline(cmd, cubemap_pipeline_.pipeline);
  RenderAPI::CmdSetScissor(cmd, 0, 1, &scissor);
  RenderAPI::CmdSetViewport(cmd, 0, 1, &view->viewport);
  RenderAPI::CmdBindDescriptorSets(cmd, 0, cubemap_pipeline_.pipeline_layout, 0,
                                   1, view->skybox_set);
  RenderAPI::CmdPushConstants(cmd, cubemap_pipeline_.pipeline_layout,
                              RenderAPI::ShaderStageFlagBits::kVertexBit, 0,
                              sizeof(glm::mat4), &cubemap_mvp);
  RenderAPI::CmdBindVertexBuffers(cmd, 0, 1, &cubemap_vertex_buffer_);
  RenderAPI::CmdBindIndexBuffer(cmd, cubemap_index_buffer_,
                                RenderAPI::IndexType::kUInt32);
  RenderAPI::CmdDrawIndexed(cmd, 36, 1, 0, 0, 0);

  // Draw the scene.
  RenderAPI::CmdBindPipeline(cmd, pbr_pipeline_.pipeline);
  RenderAPI::CmdSetScissor(cmd, 0, 1, &scissor);
  RenderAPI::CmdSetViewport(cmd, 0, 1, &view->viewport);
  RenderAPI::CmdBindDescriptorSets(cmd, 0, pbr_pipeline_.pipeline_layout, 0, 1,
                                   &view->objects_set);
  RenderAPI::CmdBindDescriptorSets(cmd, 0, pbr_pipeline_.pipeline_layout, 1, 1,
                                   view->light_set);

  const glm::mat4 kShadowBiasMatrix(0.5f, 0.0f, 0.0f, 0.0f,  //
                                    0.0f, 0.5f, 0.0f, 0.0f,  //
                                    0.0f, 0.0f, 1.0f, 0.0f,  //
                                    0.5f, 0.5f, 0.0f, 1.0f);
  LightDataGPU* lights_buffer = reinterpret_cast<LightDataGPU*>(
      RenderAPI::MapBuffer(view->lights_uniform));
  for (uint32_t i = 0; i < shadow_pass_.num_cascades; ++i) {
    lights_buffer->uCascadeSplits[i] =
        view->camera.near_clip +
        shadow_pass_.cascades[i].max_distance *
            (view->camera.far_clip - view->camera.near_clip);
    lights_buffer->uCascadeViewProjMatrices[i] =
        kShadowBiasMatrix *
        (shadow_pass_.cascades[i].projection * shadow_pass_.cascades[i].view);
  }

  lights_buffer->uCameraPosition = view->camera.position;
  lights_buffer->uLightDirection = scene.directional_light.direction;
  RenderAPI::UnmapBuffer(view->lights_uniform);

  ObjectsData* objects_buffer = reinterpret_cast<ObjectsData*>(
      RenderAPI::MapBuffer(view->objects_uniform));
  size_t instance_id = 0;
  for (const auto& model : scene.models) {
    for (const auto& primitive : model.primitives) {
      static float rotation = 0.0f;
      // rotation += 1 / 60.0f;
      objects_buffer[instance_id].uMatWorld =  // glm::identity<glm::mat4>();
          glm::mat4_cast(glm::angleAxis(rotation, glm::vec3(0.0f, 1.0f, 0.0f)));
      objects_buffer[instance_id].uMatWorldViewProjection =
          view->camera.projection * view->camera.view *
          objects_buffer[instance_id].uMatWorld;
      objects_buffer[instance_id].uMatView = view->camera.view;
      objects_buffer[instance_id].uMatNormalsMatrix =
          glm::transpose(glm::inverse(objects_buffer[instance_id].uMatWorld));

      RenderAPI::CmdBindVertexBuffers(cmd, 0, 1, &primitive.vertex_buffer);
      RenderAPI::CmdBindIndexBuffer(cmd, primitive.index_buffer,
                                    RenderAPI::IndexType::kUInt32);
      RenderAPI::CmdDrawIndexed(cmd, primitive.num_primitives, 1,
                                primitive.first_index, primitive.vertex_offset,
                                instance_id++);
    }
  }
  RenderAPI::UnmapBuffer(view->objects_uniform);
}

void Renderer::SetSkybox(View& view, const Skybox& skybox) {
  ++view.skybox_set;
  RenderAPI::WriteDescriptorSet write[1];
  RenderAPI::DescriptorImageInfo image_info;
  image_info.image_view = skybox.sky;
  image_info.sampler = cubemap_sampler_;
  write[0].set = view.skybox_set;
  write[0].binding = 0;
  write[0].descriptor_count = 1;
  write[0].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[0].images = &image_info;
  RenderAPI::UpdateDescriptorSets(device_, 1, write);
}

void Renderer::SetLightData(View& view, const IndirectLight& light,
                            RenderAPI::ImageView shadow_map_texture) {
  ++view.light_set;

  RenderAPI::WriteDescriptorSet write[5];
  RenderAPI::DescriptorBufferInfo buffer_info;
  buffer_info.buffer = view.lights_uniform;
  buffer_info.range = sizeof(LightDataGPU);
  write[0].set = view.light_set;
  write[0].binding = 0;
  write[0].buffers = &buffer_info;
  write[0].descriptor_count = 1;
  write[0].type = RenderAPI::DescriptorType::kUniformBuffer;

  RenderAPI::DescriptorImageInfo irradiance_info;
  irradiance_info.image_view = light.irradiance;
  irradiance_info.sampler = cubemap_sampler_;
  write[1].set = view.light_set;
  write[1].binding = 1;
  write[1].descriptor_count = 1;
  write[1].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[1].images = &irradiance_info;

  RenderAPI::DescriptorImageInfo reflections_info;
  reflections_info.image_view = light.reflections;
  reflections_info.sampler = cubemap_sampler_;
  write[2].set = view.light_set;
  write[2].binding = 2;
  write[2].descriptor_count = 1;
  write[2].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[2].images = &reflections_info;

  RenderAPI::DescriptorImageInfo brdf_info;
  brdf_info.image_view = light.brdf;
  brdf_info.sampler = cubemap_sampler_;
  write[3].set = view.light_set;
  write[3].binding = 3;
  write[3].descriptor_count = 1;
  write[3].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[3].images = &brdf_info;

  RenderAPI::DescriptorImageInfo shadowmap_info;
  shadowmap_info.image_view = shadow_map_texture;
  shadowmap_info.sampler = shadow_sampler_;
  write[4].set = view.light_set;
  write[4].binding = 4;
  write[4].descriptor_count = 1;
  write[4].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  write[4].images = &shadowmap_info;

  RenderAPI::UpdateDescriptorSets(device_, 5, write);
}

View* Renderer::CreateView() {
  View* view = new View();

  view->objects_uniform = RenderAPI::CreateBuffer(
      device_, RenderAPI::BufferUsageFlagBits::kStorageBuffer,
      sizeof(ObjectsData) * kMaxInstances);

  view->lights_uniform = RenderAPI::CreateBuffer(
      device_, RenderAPI::BufferUsageFlagBits::kUniformBuffer,
      sizeof(LightDataGPU));

  view->skybox_set = RenderUtils::BufferedDescriptorSet::Create(
      device_, descriptor_set_pool_, cubemap_pipeline_.descriptor_layouts[0]);
  view->light_set = RenderUtils::BufferedDescriptorSet::Create(
      device_, descriptor_set_pool_, pbr_pipeline_.descriptor_layouts[1]);

  RenderAPI::AllocateDescriptorSets(descriptor_set_pool_,
                                    std::vector<RenderAPI::DescriptorSetLayout>(
                                        1, pbr_pipeline_.descriptor_layouts[0]),
                                    &view->objects_set);

  RenderAPI::WriteDescriptorSet write[1];
  RenderAPI::DescriptorBufferInfo instance_data_buffer_info;
  instance_data_buffer_info.buffer = view->objects_uniform;
  instance_data_buffer_info.range = sizeof(ObjectsData) * kMaxInstances;
  write[0].set = view->objects_set;
  write[0].binding = 0;
  write[0].buffers = &instance_data_buffer_info;
  write[0].descriptor_count = 1;
  write[0].type = RenderAPI::DescriptorType::kStorageBuffer;
  RenderAPI::UpdateDescriptorSets(device_, 1, write);

  return view;
}

void Renderer::DestroyView(View** view) {
  RenderAPI::DestroyBuffer((*view)->objects_uniform);
  RenderAPI::DestroyBuffer((*view)->lights_uniform);

  delete *view;
  *view = nullptr;
}