#pragma once

#include <RenderAPI/RenderAPI.h>
#include "camera.h"

#include <RenderUtils/buffered_descriptor_set.h>

struct View {
  Camera camera;
  RenderAPI::Viewport viewport;

  // Lights
  RenderAPI::Buffer lights_uniform;
  RenderUtils::BufferedDescriptorSet light_set;

  // Skybox
  RenderUtils::BufferedDescriptorSet skybox_set;

  // Objects
  RenderAPI::DescriptorSet objects_set;
  RenderAPI::Buffer objects_uniform;

  // Shadow pass
  RenderAPI::Buffer shadow_transforms_uniform;
};