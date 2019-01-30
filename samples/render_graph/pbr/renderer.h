#pragma once

#include <RenderAPI/RenderAPI.h>
#include <glm/glm.hpp>
#include "render_graph/render_graph.h"
#include "scene.h"
#include "shadow_pass.h"
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

  RenderGraphResource Render(RenderGraph& render_graph, View* view,
                             const Scene* scene);

  View* CreateView();
  void DestroyView(View** view);

 private:
  RenderAPI::Device device_;

  RenderAPI::CommandPool command_pool_ = RenderAPI::kInvalidHandle;
  RenderAPI::RenderPass render_pass_ = RenderAPI::kInvalidHandle;
  RenderAPI::DescriptorSetPool descriptor_set_pool_ = RenderAPI::kInvalidHandle;

  // PBR Pipeline.
  RendererPipeline pbr_pipeline_;

  // Cubemap Pipeline.
  RendererPipeline cubemap_pipeline_;
  RenderAPI::Sampler cubemap_sampler_ = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer cubemap_vertex_buffer_ = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer cubemap_index_buffer_ = RenderAPI::kInvalidHandle;

  // Shadow mapping.
  RenderAPI::Sampler shadow_sampler_ = RenderAPI::kInvalidHandle;
  ShadowPass shadow_pass_;

  void SetSkybox(View& view, const Skybox& skybox);
  void SetLightData(View& view, const IndirectLight& light,
                    RenderAPI::ImageView shadow_map_texture);

  void Render(RenderAPI::CommandBuffer buffer, View* view, const Scene& scene,
              const glm::mat4& shadow_view_projection);
};
