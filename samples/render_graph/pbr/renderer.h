#pragma once

#include <RenderAPI/RenderAPI.h>
#include "scene.h"
#include "view.h"

struct RendererPipeline {
  std::vector<RenderAPI::DescriptorSetLayout> descriptor_layouts;
  RenderAPI::PipelineLayout pipeline_layout = RenderAPI::kInvalidHandle;
  RenderAPI::GraphicsPipeline pipeline = RenderAPI::kInvalidHandle;
};

class Renderer {
 public:
  Renderer(RenderAPI::Device device);
  ~Renderer();

  void Render(RenderAPI::CommandBuffer buffer, const View& view,
              const Scene& scene);

  void SetSkybox(Scene& scene, RenderAPI::ImageView sky);
  void SetIndirectLight(Scene& scene, RenderAPI::ImageView irradiance,
                        RenderAPI::ImageView reflections,
                        RenderAPI::ImageView brdf);

 private:
  RenderAPI::Device device_;

  RenderAPI::CommandPool command_pool_ = RenderAPI::kInvalidHandle;

  RenderAPI::Buffer objects_uniform_ = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer lights_uniform_ = RenderAPI::kInvalidHandle;

  RenderAPI::RenderPass render_pass_ = RenderAPI::kInvalidHandle;

  RenderAPI::DescriptorSetPool descriptor_set_pool_ = RenderAPI::kInvalidHandle;
  RenderAPI::DescriptorSet objects_descriptor_set_ = RenderAPI::kInvalidHandle;

  // PBR Pipeline.
  RendererPipeline pbr_pipeline_;

  // Cubemap Pipeline.
  RendererPipeline cubemap_pipeline_;
  RenderAPI::Sampler cubemap_sampler_ = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer cubemap_vertex_buffer_ = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer cubemap_index_buffer_ = RenderAPI::kInvalidHandle;
};
