#include "cascade_shadow_pass.h"

#include <glm/gtc/matrix_transform.hpp>
#include "samples/common/util.h"
#include "vertex.h"

void CreateCascadeImagesAndViews(RenderAPI::Device device,
                                 CascadeShadowsPass& pass) {
  constexpr RenderAPI::TextureFormat kDepthFormat =
      RenderAPI::TextureFormat::kD32_SFLOAT;
  constexpr RenderAPI::ImageAspectFlags kAspectBits =
      RenderAPI::ImageAspectFlagBits::kDepthBit;

  RenderAPI::ImageCreateInfo image_create_info(
      RenderAPI::TextureType::Texture2D, kDepthFormat,
      RenderAPI::Extent3D(pass.cascade_size, pass.cascade_size, 1),
      RenderAPI::ImageUsageFlagBits::kSampledBit |
          RenderAPI::ImageUsageFlagBits::kDepthStencilAttachmentBit,
      /*mips=*/1, /*layers=*/pass.num_cascades);
  pass.depth_image = RenderAPI::CreateImage(device, image_create_info);

  // Create the array view.
  RenderAPI::ImageViewCreateInfo array_image_view_info(
      pass.depth_image, RenderAPI::ImageViewType::Array2D, kDepthFormat,
      RenderAPI::ImageSubresourceRange(kAspectBits));
  pass.depth_array_view =
      RenderAPI::CreateImageView(device, array_image_view_info);

  pass.cascade_views.resize(pass.num_cascades);
  for (uint32_t i = 0; i < pass.num_cascades; ++i) {
    RenderAPI::ImageViewCreateInfo image_view_info(
        pass.depth_image, RenderAPI::ImageViewType::Array2D, kDepthFormat,
        RenderAPI::ImageSubresourceRange(kAspectBits, 0, 1, i, 1));
    pass.cascade_views[i] = RenderAPI::CreateImageView(device, image_view_info);
  }
}

void DestroyCascadeImagesAndViews(RenderAPI::Device device,
                                  CascadeShadowsPass& pass) {
  for (auto it : pass.cascade_views) {
    RenderAPI::DestroyImageView(device, it);
  }
  RenderAPI::DestroyImageView(device, pass.depth_array_view);
  RenderAPI::DestroyImage(pass.depth_image);
}

CascadeShadowsPass CascadeShadowsPass::Create(RenderAPI::Device device) {
  CascadeShadowsPass pass;

  pass.num_cascades = 4;
  pass.cascade_size = 2048;

  CreateCascadeImagesAndViews(device, pass);

  // Create the pipeline.
  auto vert =
      util::ReadFile("samples/render_graph/pbr/data/shadow_depth.vert.spv");
  auto frag =
      util::ReadFile("samples/render_graph/pbr/data/shadow_depth.frag.spv");

  Material::Builder builder(device);
  builder.PushConstant(RenderAPI::ShaderStageFlagBits::kVertexBit,
                       sizeof(glm::mat4));
  builder.VertexCode(reinterpret_cast<const uint32_t*>(vert.data()),
                     vert.size());
  builder.FragmentCode(reinterpret_cast<const uint32_t*>(frag.data()),
                       frag.size());
  builder.Viewport(
      RenderAPI::Viewport(0.0f, 0.0f, pass.cascade_size, pass.cascade_size));
  builder.DepthWrite(true);
  builder.DepthTest(true);
  builder.DepthClamp(true);
  builder.VertexAttribute(0, 0, RenderAPI::TextureFormat::kR32G32B32_SFLOAT, 0);
  builder.VertexAttribute(1, 0, RenderAPI::TextureFormat::kR32G32_SFLOAT,
                          sizeof(float) * 3);
  builder.VertexAttribute(2, 0, RenderAPI::TextureFormat::kR32G32B32_SFLOAT,
                          sizeof(float) * 5);
  builder.VertexAttribute(3, 0, RenderAPI::TextureFormat::kR32G32B32_SFLOAT,
                          sizeof(float) * 8);
  builder.VertexBinding(0, sizeof(float) * 11,
                        RenderAPI::VertexInputRate::kVertex);
  pass.material = builder.Build();

  return pass;
}

void CascadeShadowsPass::Destroy(RenderAPI::Device device,
                                 CascadeShadowsPass& shadow) {
  DestroyCascadeImagesAndViews(device, shadow);
  Material::Destroy(shadow.material);
}

void AddCascadePass(CascadeShadowsPass* shadow, RenderAPI::Device device,
                    RenderGraph& render_graph, RenderGraphResource target,
                    const Scene* scene,
                    const glm::mat4& shadow_view_projection) {
  render_graph.AddPass(
      "Cascade Shadow Map",
      [&](RenderGraphBuilder& builder) { builder.UseRenderTarget(target); },
      [shadow, scene, shadow_view_projection](RenderContext* context,
                                              const Scope& scope) {
        RenderAPI::CommandBuffer cmd = context->cmd;

        // Draw the scene.
        RenderAPI::CmdBindPipeline(
            cmd, shadow->material->GetPipeline(context->pass));

        size_t instance_id = 0;
        for (const auto& mesh : scene->meshes) {
          glm::mat4 mat_world_view_projection =
              shadow_view_projection * mesh.mat_world;
          RenderAPI::CmdPushConstants(
              cmd, shadow->material->GetPipelineLayout(),
              RenderAPI::ShaderStageFlagBits::kVertexBit, 0, sizeof(glm::mat4),
              &mat_world_view_projection);
          for (const auto& primitive : mesh.primitives) {
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
}

// Calculate cascade splits based on view camera furstum.
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
void CalculateCascadeSplits(float camera_near_clip, float camera_far_clip,
                            uint32_t num_cascades, float* cascade_splits) {
  // Logarithmic split.
  float camera_clip_range = camera_far_clip - camera_near_clip;

  constexpr float kMinShadowDistance = 0.0f;
  constexpr float kMaxShadowDistance = 1.0f;
  const float cascades_min_z =
      camera_near_clip + kMinShadowDistance * camera_clip_range;
  const float cascades_max_z =
      camera_near_clip + kMaxShadowDistance * camera_clip_range;

  const float cascades_range = cascades_max_z - cascades_min_z;
  const float cascades_ratio = cascades_max_z / cascades_min_z;

  for (uint32_t i = 0; i < num_cascades; ++i) {
    float range_percentage = (i + 1) / static_cast<float>(num_cascades);
    float log = cascades_min_z * std::pow(cascades_ratio, range_percentage);
    float uniform = cascades_min_z + cascades_range * range_percentage;
    float d = (log - uniform) + uniform;
    cascade_splits[i] = (d - camera_near_clip) / camera_clip_range;
  }
}

void CreateShadowmapCascades(ShadowMapCascadeInfo* cascades,
                             const Camera& camera, uint32_t num_cascades,
                             uint32_t shadow_map_size) {
  assert(num_cascades <= 4);

  // Get the cascade splits.
  float cascade_splits[4];
  CalculateCascadeSplits(camera.NearClip(), camera.FarClip(), num_cascades,
                         cascade_splits);

  // Copy the info for each cascade.
  for (uint32_t i = 0; i < num_cascades; ++i) {
    cascades[i].max_distance = cascade_splits[i];
    if (i == 0) {
      cascades[i].min_distance = 0.0f;
    } else {
      cascades[i].min_distance = cascade_splits[i - 1];
    }
  }

  // Retrieve the camera's inverse view projection.
  const glm::mat4 inverse_camera_view_projection =
      glm::inverse(camera.GetProjection() * camera.GetView());

  // Project the frustum corners into world space.
  glm::vec3 frustumCornersWS[8] = {
      glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f),
      glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f),
      glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f),
  };

  for (uint32_t i = 0; i < 8; ++i) {
    glm::vec4 frustum_corner_world_space =
        inverse_camera_view_projection * glm::vec4(frustumCornersWS[i], 1.0f);
    frustumCornersWS[i] =
        frustum_corner_world_space / frustum_corner_world_space.w;
  }

  for (uint32_t cascade_index = 0; cascade_index < num_cascades;
       ++cascade_index) {
    // Get the corners of the current cascade slice of the view frustum
    glm::vec3 frustum_corners[8];
    for (uint32_t i = 0; i < 4; ++i) {
      glm::vec3 ray = frustumCornersWS[i + 4] - frustumCornersWS[i];
      glm::vec3 near_corner_ray = ray * cascades[cascade_index].min_distance;
      glm::vec3 far_corner_ray = ray * cascades[cascade_index].max_distance;

      frustum_corners[i + 4] = frustumCornersWS[i] + far_corner_ray;
      frustum_corners[i] = frustumCornersWS[i] + near_corner_ray;
    }

    // Calculate the center point of the cascade.
    cascades[cascade_index].center = glm::vec3(0.0f);
    for (uint32_t i = 0; i < 8; ++i) {
      cascades[cascade_index].center += frustum_corners[i];
    }
    cascades[cascade_index].center *= 1.0f / 8.0f;

    // Calculate the radius of a bounding sphere surrounding the frustum corners
    float sphere_radius = 0.0f;
    for (uint32_t i = 0; i < 8; ++i) {
      float corner_distance_to_center =
          glm::length(frustum_corners[i] - cascades[cascade_index].center);
      sphere_radius = std::max(sphere_radius, corner_distance_to_center);
    }
    sphere_radius = std::ceil(sphere_radius * 16.0f) / 16.0f;

    // Calculate the extents for the orthographic projection.
    const glm::vec3 max_extents = glm::vec3(sphere_radius);
    const glm::vec3 min_extents = -max_extents;
    cascades[cascade_index].extents = max_extents - min_extents;

    // Get position of the shadow camera.
    static const glm::vec3 kLightDirection =
        glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
    const glm::vec3 shadow_camera_pos =
        cascades[cascade_index].center - kLightDirection * -min_extents.z;

    // Create the view and projection matrices.
    // [Note: Should probably change the LookAt matrix to avoid incorrect result
    // when the light is pointing directly down.]
    cascades[cascade_index].view =
        glm::lookAt(shadow_camera_pos, cascades[cascade_index].center,
                    glm::vec3(0.0f, 1.0f, 0.0f));
    cascades[cascade_index].projection =
        glm::ortho(min_extents.x, max_extents.x, min_extents.y, max_extents.y,
                   0.0f, cascades[cascade_index].extents.z);

    // Stabilize the shadows by snapping the projection to texel size
    // increments. Create the rounding matrix, by projecting the world-space
    // origin and determining the fractional offset in texel space.
    const glm::mat4 shadow_matrix =
        cascades[cascade_index].projection * cascades[cascade_index].view;
    glm::vec4 shadow_origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    shadow_origin = shadow_matrix * shadow_origin;
    shadow_origin = shadow_origin * static_cast<float>(shadow_map_size) / 2.0f;

    const glm::vec4 rounded_origin = glm::round(shadow_origin);
    glm::vec4 round_offset = rounded_origin - shadow_origin;
    round_offset = round_offset * 2.0f / static_cast<float>(shadow_map_size);
    round_offset.z = 0.0f;
    round_offset.w = 0.0f;
    cascades[cascade_index].projection[3] += round_offset;
  }
}

std::vector<RenderGraphResource> CascadeShadowsPass::AddPass(
    CascadeShadowsPass* shadow, RenderAPI::Device device,
    RenderGraph& render_graph, View* view, const Scene* scene) {
  std::vector<RenderGraphResource> render_graph_resources;

  ShadowMapCascadeInfo* cascades = shadow->cascades;
  CreateShadowmapCascades(cascades, view->camera, shadow->num_cascades,
                          shadow->cascade_size);

  for (uint32_t i = 0; i < shadow->num_cascades; ++i) {
    RenderGraphTextureDesc depth_desc;
    depth_desc.format = RenderAPI::TextureFormat::kD32_SFLOAT;
    depth_desc.width = shadow->cascade_size;
    depth_desc.height = shadow->cascade_size;
    depth_desc.load_op = RenderAPI::AttachmentLoadOp::kClear;
    depth_desc.layout = RenderAPI::ImageLayout::kShaderReadOnlyOptimal;
    depth_desc.clear_values.depth_stencil.depth = 1.0f;
    RenderGraphResource target =
        render_graph.ImportTexture(depth_desc, shadow->cascade_views[i]);
    render_graph_resources.push_back(target);

    glm::mat4 shadow_view_projection =
        cascades[i].projection * cascades[i].view;
    AddCascadePass(shadow, device, render_graph, target, scene,
                   shadow_view_projection);
  }

  return render_graph_resources;
}
