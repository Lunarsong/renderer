#pragma once

#include <RenderAPI/RenderAPI.h>
#include <RenderUtils/buffered_descriptor_set.h>
#include <glm/glm.hpp>
#include "render_graph/render_graph.h"
#include "scene.h"
#include "view.h"

struct ShadowMapCascadeInfo {
  float min_distance;
  float max_distance;
  glm::vec3 center;
  glm::vec3 extents;
  glm::mat4 view;
  glm::mat4 projection;
};

struct CascadeShadowsPass {
  RenderAPI::Device device;

  RenderAPI::GraphicsPipeline pipeline;
  RenderAPI::PipelineLayout pipeline_layout;
  RenderAPI::RenderPass pass;

  uint32_t num_cascades;
  uint32_t cascade_size;
  ShadowMapCascadeInfo cascades[4];
  RenderAPI::Image depth_image;
  RenderAPI::ImageView depth_array_view;
  std::vector<RenderAPI::ImageView> cascade_views;

  static CascadeShadowsPass Create(RenderAPI::Device device);
  static void Destroy(RenderAPI::Device device, CascadeShadowsPass& shadow);
  static std::vector<RenderGraphResource> AddPass(CascadeShadowsPass* shadow,
                                                  RenderAPI::Device device,
                                                  RenderGraph& render_graph,
                                                  View* view,
                                                  const Scene* scene);
};
