#pragma once

#include <RenderAPI/RenderAPI.h>
#include "camera.h"

#include <RenderUtils/buffered_descriptor_set.h>

struct View {
  Camera camera;
  RenderAPI::Viewport viewport;

  // Lights
  RenderAPI::Buffer lights_uniform;
  RenderUtils::BufferedDescriptorSet indirect_light_set;

  // Skybox
  RenderUtils::BufferedDescriptorSet skybox_set;

  // Objects
  RenderAPI::DescriptorSet objects_set = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer objects_uniform;
};