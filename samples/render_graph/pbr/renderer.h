#pragma once

#include <RenderAPI/RenderAPI.h>
#include <glm/glm.hpp>
#include "cascade_shadow_pass.h"
#include "render_graph/render_graph.h"
#include "scene.h"
#include "view.h"

class Renderer {
 public:
  Renderer(RenderAPI::Device device);
  ~Renderer();

  RenderGraphResource Render(RenderGraph& render_graph, View* view,
                             Scene* scene);

  View* CreateView();
  void DestroyView(View** view);

  MaterialInstance* CreatePbrMaterialInstance();

 private:
  RenderAPI::Device device_;

  RenderAPI::CommandPool command_pool_ = RenderAPI::kInvalidHandle;

  // PBR Pipeline.
  Material* pbr_material_;

  // Skybox Material.
  Material* skybox_material_;
  RenderAPI::Buffer cubemap_vertex_buffer_ = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer cubemap_index_buffer_ = RenderAPI::kInvalidHandle;

  // Shadow mapping.
  CascadeShadowsPass shadow_pass_;

  void SetSkybox(View& view, const Skybox& skybox);
  void SetLightData(View& view, const Scene* scene,
                    RenderAPI::ImageView shadow_map_texture);

  void Render(RenderContext* context, View* view, Scene& scene);
};
