#include "shadow_pass.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "samples/common/util.h"
#include "vertex.h"

ShadowPass ShadowPass::Create(RenderAPI::Device device) {
  ShadowPass pass;

  // Pass.
  RenderAPI::RenderPassCreateInfo pass_info;
  pass_info.attachments.resize(1);
  pass_info.attachments[0].final_layout =
      RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
  pass_info.attachments[0].format = RenderAPI::TextureFormat::kD32_SFLOAT;
  pass.pass = RenderAPI::CreateRenderPass(device, pass_info);

  // Pipeline layout.
  RenderAPI::PipelineLayoutCreateInfo layout_info;
  layout_info.push_constants.push_back(
      {RenderAPI::ShaderStageFlagBits::kVertexBit, 0, sizeof(glm::mat4)});
  pass.pipeline_layout = RenderAPI::CreatePipelineLayout(device, layout_info);

  // Graphics pipeline.
  RenderAPI::GraphicsPipelineCreateInfo info;
  auto vert =
      util::ReadFile("samples/render_graph/pbr/data/shadow_depth.vert.spv");
  auto frag =
      util::ReadFile("samples/render_graph/pbr/data/shadow_depth.frag.spv");
  info.vertex.code = reinterpret_cast<const uint32_t*>(vert.data());
  info.vertex.code_size = vert.size();
  info.fragment.code = reinterpret_cast<const uint32_t*>(frag.data());
  info.fragment.code_size = frag.size();
  info.vertex_input = {{Vertex::layout}};
  info.layout = pass.pipeline_layout;

  info.states.blend.attachments.resize(1);
  info.states.depth_stencil.depth_write_enable = true;
  info.states.depth_stencil.depth_test_enable = true;

  // info.states.rasterization.front_face = RenderAPI::FrontFace::kClockwise;

  info.states.viewport.viewports.emplace_back(
      RenderAPI::Viewport(0.0f, 0.0f, 1024.0f, 1024.0f));

  pass.pipeline = RenderAPI::CreateGraphicsPipeline(device, pass.pass, info);

  return pass;
}

void ShadowPass::Destroy(RenderAPI::Device device, ShadowPass& shadow) {
  RenderAPI::DestroyGraphicsPipeline(shadow.pipeline);
  RenderAPI::DestroyPipelineLayout(device, shadow.pipeline_layout);
  RenderAPI::DestroyRenderPass(shadow.pass);
}

RenderGraphResource ShadowPass::AddPass(ShadowPass* shadow,
                                        RenderAPI::Device device,
                                        RenderGraph& render_graph, View* view,
                                        const Scene* scene) {
  RenderGraphResource output;
  render_graph.AddPass(
      "Shadow Map",
      [&](RenderGraphBuilder& builder) {
        RenderGraphTextureDesc depth_desc;
        depth_desc.format = RenderAPI::TextureFormat::kD32_SFLOAT;
        depth_desc.width = 1024;
        depth_desc.height = 1024;
        depth_desc.load_op = RenderAPI::AttachmentLoadOp::kClear;
        depth_desc.layout = RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
        depth_desc.clear_values.depth_stencil.depth = 1.0f;
        output = builder.CreateRenderTarget(depth_desc).textures[0];
      },
      [shadow, view, scene](RenderContext* context, const Scope& scope) {
        RenderAPI::CommandBuffer cmd = context->cmd;
        glm::mat4 shadow_projection =
            glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 30.0f);
        const glm::mat4 shadow_view =
            glm::lookAt(-scene->directional_light.direction * 20.0f,
                        glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        const glm::mat4 shadow_view_projection =
            shadow_projection * shadow_view;

        // Draw the scene.
        RenderAPI::CmdBindPipeline(cmd, shadow->pipeline);

        RenderAPI::CmdPushConstants(cmd, shadow->pipeline_layout,
                                    RenderAPI::ShaderStageFlagBits::kVertexBit,
                                    0, sizeof(glm::mat4),
                                    &shadow_view_projection);
        /*LightDataGPU* lights_buffer = reinterpret_cast<LightDataGPU*>(
            RenderAPI::MapBuffer(view->lights_uniform));
        lights_buffer->uCameraPosition = view->camera.position;
        lights_buffer->uLightDirection = scene.directional_light.direction;
        RenderAPI::UnmapBuffer(view->lights_uniform);*/

        size_t instance_id = 0;
        for (const auto& model : scene->models) {
          for (const auto& primitive : model.primitives) {
            RenderAPI::CmdBindVertexBuffers(cmd, 0, 1,
                                            &primitive.vertex_buffer);
            RenderAPI::CmdBindIndexBuffer(cmd, primitive.index_buffer,
                                          RenderAPI::IndexType::kUInt32);
            RenderAPI::CmdDrawIndexed(cmd, primitive.num_primitives, 1,
                                      primitive.first_index,
                                      primitive.vertex_offset, instance_id++);
          }
        }
      });

  return output;
}