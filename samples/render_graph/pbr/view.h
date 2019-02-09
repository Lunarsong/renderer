#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/MaterialInstance.h>
#include "camera.h"

#include <RenderUtils/BufferedDescriptorSet.h>

struct View {
  Camera camera;
  RenderAPI::Viewport viewport;

  // Materials
  MaterialInstance* skybox_material_instance = nullptr;
  MaterialInstance* pbr_material_instance = nullptr;
  MaterialParams* light_params;
};