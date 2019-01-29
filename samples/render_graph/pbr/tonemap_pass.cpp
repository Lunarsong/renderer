#include "tonemap_pass.h"

#include "samples/common/util.h"

TonemapPass CreateTonemapPass(RenderAPI::Device device) {
  TonemapPass tonemap;

  // Create the render pass.
  RenderAPI::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(1);
  pass_info.attachments[0].final_layout =
      RenderAPI::ImageLayout::kPresentSrcKHR;
  pass_info.attachments[0].format = RenderAPI::TextureFormat::kB8G8R8A8_UNORM;
  tonemap.pass = RenderAPI::CreateRenderPass(device, pass_info);

  // Create a sampler.
  RenderAPI::SamplerCreateInfo sampler_info;
  sampler_info.mag_filter = RenderAPI::SamplerFilter::kLinear;
  sampler_info.min_filter = RenderAPI::SamplerFilter::kLinear;
  tonemap.sampler = RenderAPI::CreateSampler(device, sampler_info);

  // Create the descriptor layout.
  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_layout_info;
  descriptor_layout_info.bindings.push_back(
      RenderAPI::DescriptorSetLayoutBinding(
          RenderAPI::DescriptorType::kCombinedImageSampler, 1,
          RenderAPI::ShaderStageFlagBits::kFragmentBit));
  tonemap.descriptor_layout =
      RenderAPI::CreateDescriptorSetLayout(device, descriptor_layout_info);

  // Create the pipeline layout.
  RenderAPI::PipelineLayoutCreateInfo layout_info;
  layout_info.layouts.push_back(tonemap.descriptor_layout);
  tonemap.pipeline_layout =
      RenderAPI::CreatePipelineLayout(device, layout_info);

  // Create a graphics pipeline.
  RenderAPI::GraphicsPipelineCreateInfo info;
  auto vert = util::ReadFile("samples/render_graph/pbr/data/tonemap.vert.spv");
  auto frag = util::ReadFile("samples/render_graph/pbr/data/tonemap.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.layout = tonemap.pipeline_layout;

  info.states.blend.attachments.resize(1);
  info.states.rasterization.front_face = RenderAPI::FrontFace::kClockwise;
  info.states.viewport.viewports.emplace_back(
      RenderAPI::Viewport(0.0f, 0.0f, 1920.0f, 1200.0f));

  info.states.dynamic_states.states.push_back(
      RenderAPI::DynamicState::kViewport);
  info.states.dynamic_states.states.push_back(
      RenderAPI::DynamicState::kScissor);
  tonemap.pipeline =
      RenderAPI::CreateGraphicsPipeline(device, tonemap.pass, info);

  // Create the descriptor set pool.
  RenderAPI::CreateDescriptorSetPoolCreateInfo pool_info = {
      {{RenderAPI::DescriptorType::kCombinedImageSampler, 3}},
      /*max_sets=*/3};
  tonemap.descriptor_set_pool =
      RenderAPI::CreateDescriptorSetPool(device, pool_info);

  tonemap.descriptor_set = RenderUtils::BufferedDescriptorSet::Create(
      device, tonemap.descriptor_set_pool, tonemap.descriptor_layout);

  tonemap.device = device;
  return tonemap;
}

void DestroyTonemapPass(RenderAPI::Device device, TonemapPass& tonemap) {
  RenderAPI::DestroyGraphicsPipeline(tonemap.pipeline);
  RenderAPI::DestroyPipelineLayout(device, tonemap.pipeline_layout);
  RenderAPI::DestroyRenderPass(tonemap.pass);
  RenderAPI::DestroyDescriptorSetPool(tonemap.descriptor_set_pool);
  RenderAPI::DestroyDescriptorSetLayout(tonemap.descriptor_layout);
  RenderAPI::DestroySampler(device, tonemap.sampler);
}

RenderGraphResource AddTonemapPass(RenderAPI::Device device,
                                   RenderGraph& render_graph,
                                   TonemapPass* tonemap,
                                   RenderGraphResource texture) {
  auto swapchain_desc = render_graph.GetSwapChainDescription();
  RenderGraphResource output;
  render_graph.AddPass(
      "Tonemap",
      [&](RenderGraphBuilder& builder) {
        builder.Read(texture);
        output = builder.CreateRenderTarget(swapchain_desc).textures[0];
      },
      [=](RenderContext* context, const Scope& scope) {
        RenderAPI::ImageView hdr_texture = scope.GetTexture(texture);

        // Update the descriptor set.
        RenderAPI::WriteDescriptorSet write[1];
        RenderAPI::DescriptorImageInfo image_info;
        image_info.image_view = hdr_texture;
        image_info.sampler = tonemap->sampler;
        write[0].set = ++tonemap->descriptor_set;
        write[0].binding = 0;
        write[0].descriptor_count = 1;
        write[0].type = RenderAPI::DescriptorType::kCombinedImageSampler;
        write[0].images = &image_info;
        RenderAPI::UpdateDescriptorSets(tonemap->device, 1, write);

        RenderAPI::CmdBindPipeline(context->cmd, tonemap->pipeline);

        RenderAPI::Rect2D scissor = {
            RenderAPI::Offset2D(0, 0),
            RenderAPI::Extent2D(swapchain_desc.width, swapchain_desc.height)};
        RenderAPI::Viewport viewport(0, 0, swapchain_desc.width,
                                     swapchain_desc.height);

        RenderAPI::CmdSetScissor(context->cmd, 0, 1, &scissor);
        RenderAPI::CmdSetViewport(context->cmd, 0, 1, &viewport);

        RenderAPI::CmdBindDescriptorSets(context->cmd, 0,
                                         tonemap->pipeline_layout, 0, 1,
                                         tonemap->descriptor_set);
        RenderAPI::CmdDraw(context->cmd, 3);
      });
  return output;
}
