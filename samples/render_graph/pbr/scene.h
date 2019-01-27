#pragma once

#include <RenderAPI/RenderAPI.h>
#include <vector>
#include "model.h"

struct Scene {
  std::vector<Model> models;

  RenderAPI::DescriptorSet cubemap_descriptor = RenderAPI::kInvalidHandle;
  RenderAPI::Image cubemap_image;
  RenderAPI::ImageView cubemap_image_view;
};

inline void DestroyScene(RenderAPI::Device device, Scene& scene) {
  for (auto& model : scene.models) {
    DestroyModel(device, model);
  }
  scene.models.clear();
}