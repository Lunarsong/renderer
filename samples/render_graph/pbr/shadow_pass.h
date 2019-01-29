#pragma once

#include <RenderAPI/RenderAPI.h>
#include <RenderUtils/buffered_descriptor_set.h>
#include "render_graph/render_graph.h"
#include "scene.h"
#include "view.h"

struct ShadowPass {
  RenderAPI::Device device;

  RenderAPI::GraphicsPipeline pipeline;
  RenderAPI::PipelineLayout pipeline_layout;
  RenderAPI::RenderPass pass;

  static ShadowPass Create(RenderAPI::Device device);
  static void Destroy(RenderAPI::Device device, ShadowPass& shadow);
  static RenderGraphResource AddPass(ShadowPass* shadow,
                                     RenderAPI::Device device,
                                     RenderGraph& render_graph, View* view,
                                     const Scene* scene);
};