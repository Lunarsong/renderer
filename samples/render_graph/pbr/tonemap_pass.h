#pragma once

#include <RenderAPI/RenderAPI.h>
#include <RenderUtils/BufferedDescriptorSet.h>

#include "render_graph/render_graph.h"

struct TonemapPass {
  RenderAPI::Device device;

  RenderAPI::GraphicsPipeline pipeline;
  RenderAPI::PipelineLayout pipeline_layout;
  RenderAPI::RenderPass pass;

  RenderAPI::DescriptorSetLayout descriptor_layout;
  RenderAPI::DescriptorSetPool descriptor_set_pool;
  RenderUtils::BufferedDescriptorSet descriptor_set;

  RenderAPI::Sampler sampler;
};

TonemapPass CreateTonemapPass(RenderAPI::Device device);
void DestroyTonemapPass(RenderAPI::Device device, TonemapPass& tonemap);
RenderGraphResource AddTonemapPass(RenderAPI::Device device,
                                   RenderGraph& render_graph,
                                   TonemapPass* tonemap,
                                   RenderGraphResource texture);