#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/MaterialInstance.h>
#include "camera.h"

#include <RenderUtils/BufferedDescriptorSet.h>

struct View {
  Camera camera;
  RenderAPI::Viewport viewport;

  // Lights
  RenderAPI::Buffer lights_uniform;
  RenderUtils::BufferedDescriptorSet light_set;

  // Skybox
  MaterialInstance* skybox_material_instance = nullptr;

  // Objects
  RenderAPI::DescriptorSet objects_set;
  RenderAPI::Buffer objects_uniform;

  // Shadow pass
  RenderAPI::Buffer shadow_transforms_uniform;
};